#include "../acpi/tables/madt.h"
#include "../8259/pic.h"
#include "../uart/serial.h"
#include "cpu/interrupts/idt.h"
#include "cpu/tss/tss.h"
#include "debug.h"
#include "lapic.h"
#include "ioapic.h"
#include <cpuid.h>
#include <stdbool.h>

static void detect_apic() {
    uint32_t erax, erbx, ercx, erdx;
    __cpuid(1, erax, erbx, ercx, erdx);

    bool apic = (erdx >> 9) & 1;

    if (!apic) {
        serial_terminal()->puts("ERROR: Hardware unsupported (cpuid.01h.edx.9): APIC missing; get a new computer");
        __asm__ volatile ("cli; hlt");
    }
}


static void configure_local_apic(acpi_madt_header_t* madt) {
    serial_terminal()->puts("\n\nlocal apic configuration:\n\n");
    apic_local_set_base((void *)(uintptr_t)madt->lapic_address);
    serial_terminal()->puts("base: ")->putul(madt->lapic_address)->putc('\n');

    apic_local_write(APIC_LOCAL_REGISTER_SPURIOUS_INT_VECTOR, 0x100 | apic_local_read(APIC_LOCAL_REGISTER_SPURIOUS_INT_VECTOR));
    serial_terminal()->puts("spiv: ")->putul(apic_local_read(APIC_LOCAL_REGISTER_SPURIOUS_INT_VECTOR))->putc('\n');

    uint32_t lapic_id = apic_local_read(APIC_LOCAL_REGISTER_ID);
    serial_terminal()->puts("id: ")->putul(lapic_id)->putc('\n');

    const uint8_t nmi = 0x04 & 0x07;

    for (acpi_madt_record_t* madt_record = (acpi_madt_record_t *)((uintptr_t)madt + sizeof(acpi_madt_header_t));;) {
        if (((uintptr_t)madt_record - (uintptr_t)madt) >= madt->common.length)
            break;

        madt_record = (acpi_madt_record_t *)((uintptr_t)madt_record + madt_record->length);

        if (madt_record->type == ACPI_MADT_RECORD_TYPE_LOCAL_NMI) {
            acpi_madt_record_nmi_t* lint01_record = (acpi_madt_record_nmi_t *)madt_record;
            if (lint01_record->processor_id != lapic_id && lint01_record->processor_id != 0xff)
                continue;
            
            apic_local_register_t lvt = lint01_record->lint_no == 0 ? APIC_LOCAL_REGISTER_LVT_LINT0 : APIC_LOCAL_REGISTER_LVT_LINT1;

            uint32_t entry = 0x00000000;
            entry |= (nmi & 0x07) << 8;

            bool level_triggered = !!(lint01_record->flags & ACPI_MADT_RECORD_ISO_NMI_FLAG_LEVEL_TRIGGERED);
            bool active_low = !!(lint01_record->flags & ACPI_MADT_RECORD_ISO_NMI_FLAG_ACTIVE_LOW);

            entry |= (uint32_t)level_triggered << 13;
            entry |= (uint32_t)active_low << 15;

            apic_local_write(lvt, entry);
        }
    }
    serial_terminal()->puts("lint0: ")->putul(apic_local_read(APIC_LOCAL_REGISTER_LVT_LINT0))->putc('\n');
    serial_terminal()->puts("lint1: ")->putul(apic_local_read(APIC_LOCAL_REGISTER_LVT_LINT1))->putc('\n');
}

static void configure_io_apic(acpi_madt_header_t* madt) {
    for (acpi_madt_record_t* madt_record = (acpi_madt_record_t *)((uintptr_t)madt + sizeof(acpi_madt_header_t));;) {
        if (((uintptr_t)madt_record - (uintptr_t)madt) >= madt->common.length)
            break;

        madt_record = (acpi_madt_record_t *)((uintptr_t)madt_record + madt_record->length);

        if (madt_record->type == ACPI_MADT_RECORD_TYPE_IOAPIC) {
            acpi_madt_record_ioapic_t* ioapic = (acpi_madt_record_ioapic_t *)madt_record;
            apic_io_register_controller(*ioapic);
        }
    }

    serial_terminal()->puts("\n\nioapic overrides:\n\n");

    for (acpi_madt_record_t* madt_record = (acpi_madt_record_t *)((uintptr_t)madt + sizeof(acpi_madt_header_t));;) {
        if (((uintptr_t)madt_record - (uintptr_t)madt) >= madt->common.length)
            break;

        madt_record = (acpi_madt_record_t *)((uintptr_t)madt_record + madt_record->length);

        if (madt_record->type == ACPI_MADT_RECORD_TYPE_ISO) {
            acpi_madt_record_iso_t* override = (acpi_madt_record_iso_t *)madt_record;
            apic_io_redirect_irq(override->irq_source, 
                                (uint8_t)override->gsi, 
                                !!(override->flags & ACPI_MADT_RECORD_ISO_NMI_FLAG_ACTIVE_LOW), 
                                !!(override->flags & ACPI_MADT_RECORD_ISO_NMI_FLAG_LEVEL_TRIGGERED));

            serial_terminal()->puts("irq: ")->putul(override->irq_source)->puts(", vec: ")->putul(override->gsi)->putc('\n');
        }
    }
}

void apic_initialize(void);
void apic_initialize() {
    acpi_madt_header_t* madt = ACPI_MADT_GET();

    pic_disable();

    detect_apic();

    configure_local_apic(madt);

    configure_io_apic(madt);
}
