# Skylight
## A bright opening, a clean window.

### **Etymology**
According to [dictionary.com](https://www.dictionary.com/browse/skylight#), a "skylight" is:
> an opening in a roof or ceiling, fitted with glass, for admitting daylight.

Skylight is an operating system that is modeled after Windows, and aims to be cleaner and more efficient.

### **License**
I, rizet, have waived my copyrights on this entire repository, under the Creative Commons Zero license.

In layman's terms, I have released my source code to the public domain, and you may use the contents of this repository however you see fit.

See [license](LICENSE).

### **Building**
Skylight uses Linux software to compile, therefore any build process must take place under native Linux or Windows' subsystem for Linux.

Additionally, you require these packages:
```
util-linux 
dosfstools 
mtools 
nasm 
clang 
build-essential 
wget
```

To build, simply enter the root of the source tree, and run `make image`. A FAT32-formatted hard drive image will be generated.

### Testing
In order to test, you need to ensure that your testing emulator/hardware supports:
 - x86-64
 - PAE/NX
 - SSE2
 - ACPI
 - UEFI v2.0
 - 128MiB RAM (min.)

To test in an emulator, enter the source tree after building an image, and type `make run`.

To test on real hardware, meet the requirements above, and write the image to a drive.
Boot into it through your UEFI boot menu.