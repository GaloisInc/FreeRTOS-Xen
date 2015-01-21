
FreeRTOS for Xen on ARM
-----------------------

This FreeRTOS tree provides a port of FreeRTOS for Xen on ARM systems.

 * Author: Jonathan Daugherty (jtd AT galois.com), Galois, Inc.
 * Xen version: 4.4+
 * FreeRTOS version: 8.2.0
 * Supported systems: any ARM system with virtualization extensions

Selected elements of the source tree layout are as follows:

  * Demo/
    * CORTEX_A15_Xen_GCC/: The root of the build system, including the C library, drivers, linker script, and Makefile.
      * platform/: Code responsible for integrating with the hypervisor and paravirtualized services.
      * xen/: Hypervisor services: grant tables, event channels, hypercalls, Xenbus.
      * console/: The xen virtual console drivers: the hypercall-based debug console and the xenbus-based console.
      * asm/: Assembly relevant to interacting with the hypervisor.
      * minlibc/: The minimal C library used by the Xen service drivers.
  * Source/: The FreeRTOS core, including task scheduler and other data structure.
  * portable/GCC/: The ARM-specific elements of the port for the GNU compiler toolchain.
    * ARM7_CA15_Xen/: The ARMv7 Cortex A15 Xen port implementation.
    * asm/: All assembly for setting up the hardware, including cache setup, FPU setup, interrupt handlers, page table management and iMMU setup, and virtual timer setup.

Requirements
------------

To make use of this distribution, you will need:

 * An installed version of Xen, including development headers (e.g. `libxen-dev`
   on Ubuntu)
 * An ARM cross compiler toolchain (e.g. installable from your distribution as
   `arm-none-eabi-gcc`)
 * A standard Linux development system (i.e. with GNU `make`)
 * An ARM system capable of running Xen:
   http://www.xenproject.org/developers/teams/arm-hypervisor.html

Step 1: Building the core
-------------------------

Applications are built with this distribution in two steps:

 * Build the core of FreeRTOS as a library (`.a`)
 * Build your application codebase and link it against the library

The first step requires your cross compiler to be in the PATH and requires the
presence of Xen development headers to be installed somewhere on your system.
Given both of those details, the build process for building the core FreeRTOS
library is as follows:

```
  $ cd FreeRTOS/Demo/CORTEX_A15_Xen_GCC
  $ make CROSS_COMPILE=... XEN_PREFIX=...
```

The two settings specified above are

 * `CROSS_COMPILE`: the string prefix of the cross compiler toolchain's program
   names.  DEFAULT: "`arm-none-eabi-`".

 * `XEN_PREFIX`: the autoconf-style "prefix" for the location of the Xen headers.
   DEFAULT: "`/usr`".

The result of the build process will be `FreeRTOS.a`, the library against which
your application will be built in step 2.

Step 2: Building an application
-------------------------------

To build an application with `FreeRTOS.a`, compile it and link it against the
library.  The details of this process are somewhat tedious, so this
distribution includes a sample Makefile for this purpose which you can
customize for your application.  See `Makefile.example` in the root of the
distribution.

Running a FreeRTOS VM
---------------------

To create a virtual machine from an application image, create a Xen domain
configuration for your application, e.g.,

```
  kernel = ".../path/to/application.bin"
  memory = 32
  name = "your_application"
  vcpus = 1
  disk = []
```

and then create a domain with `xl create`, e.g.,

```
  xl create -f .../path/to/domain.cfg
```

Debugging
---------

Many modules in the distribution provide extra Xen console output for debugging
purposes.  The convention for enabling such debugging output is to edit
relevant source files and set a `#define`'d "`DEBUG`" constant to a non-zero value.
Look for e.g.

```
  #define DEBUG 0
  #define dprintk if (DEBUG) printk
```

Contributing
------------

If you'd like to contribute to this effort, please submit issues or pull
requests via the GitHub repository page at

  https://github.com/GaloisInc/FreeRTOS-Xen

See Also
--------

There are many ports of FreeRTOS which are not included in this distribution.
For more information on the upstream FreeRTOS distribution, please see:

 * http://www.freertos.org/FreeRTOS-quick-start-guide.html
 * http://www.freertos.org/FAQHelp.html
