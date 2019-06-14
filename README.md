
FreeRTOS for Xen on ARM
-----------------------

This FreeRTOS tree provides a port of FreeRTOS for Xen on ARM systems.

**This project is not currently maintained. Use at your own risk.**

 * Author: Jonathan Daugherty (jtd AT galois.com), Galois, Inc.
 * Xen version: 4.7
 * FreeRTOS version: 7.6.0
 * Supported systems: any ARM system with virtualization extensions
 * Stability: suggested for exploratory/research use only, see Future
   Work section

Selected elements of the source tree layout are as follows:

  * `Demo/`
    * `CORTEX_A15_Xen_GCC/`: The root of the build system, including the
      C library, drivers, linker script, and Makefile.
      * `platform/`: Code responsible for integrating with the
        hypervisor and paravirtualized services.
      * `xen/`: Hypervisor services: grant tables, event channels,
        hypercalls, Xenbus.
      * `console/`: The xen virtual console drivers: the hypercall-based
        debug console and the xenbus-based console.
      * `asm/`: Assembly relevant to interacting with the hypervisor.
      * `minlibc/`: The minimal C library used by the Xen service
        drivers.
  * `Source/`: The FreeRTOS core, including task scheduler and other data
    structure.
  * `portable/GCC/`: The ARM-specific elements of the port for the GNU
    compiler toolchain.
    * `ARM7_CA15_Xen/`: The ARMv7 Cortex A15 Xen port implementation.
    * `asm/`: All assembly for setting up the hardware, including cache
      setup, FPU setup, interrupt handlers, page table management and MMU
      setup, and virtual timer setup.

Requirements
------------

To make use of this distribution, you will need:

 * An installed version of Xen, including development headers (e.g.
   `libxen-dev` on Ubuntu)
 * An ARM cross compiler toolchain (e.g. installable from your
   distribution as `arm-none-eabi-gcc`)
 * A standard Linux development system (i.e. with GNU `make`)
 * An ARM system capable of running Xen:
   http://wiki.xen.org/wiki/Xen_ARM_with_Virtualization_Extensions#Hardware

Step 1: Building the core
-------------------------

Applications are built with this distribution in two steps:

 * Build the core of FreeRTOS as a library (`.a`)
 * Build your application codebase and link it against the library

The first step requires your cross compiler to be in the PATH and
requires the presence of Xen development headers to be installed
somewhere on your system. Given both of those details, the build process
for building the core FreeRTOS library is as follows:

Step 1: fetch dependencies:
```
  $ cd FreeRTOS
  $ bash setup.sh
```

Step 2: build the core library:
```
  $ cd FreeRTOS/Demo/CORTEX_A15_Xen_GCC
  $ make CROSS_COMPILE=... XEN_PREFIX=...
```

The two settings specified above are

 * `CROSS_COMPILE`: the string prefix of the cross compiler toolchain's
   program names. DEFAULT: "`arm-none-eabi-`".

 * `XEN_PREFIX`: the autoconf-style "prefix" for the location of the
   Xen headers. DEFAULT: "`/usr`".

The result of the build process will be `FreeRTOS.a`, the library
against which your application will be built in step 2.

Step 2: Building an application
-------------------------------

To build an application with `FreeRTOS.a`, compile it and link it
against the library. The details of this process are somewhat tedious,
but this distribution includes an example application and Makefile
in the Example/ directory. Once you've built the FreeRTOS library as
described in Step 1:

```
  $ cd Example/
  $ make
```

The resulting `Example.bin` kernel image can be used to create a virtual
machine as described in the next section.

Running a FreeRTOS VM
---------------------

To create a virtual machine from an application image, create a Xen
domain configuration for your application, e.g.,

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

Using Xen Services
------------------

This port of FreeRTOS borrows Xenbus, console, grant table, and event
channel implementations from Mini-OS, modified to work with the FreeRTOS
scheduler.

 * General setup:
   * `Demo/CORTEX_A15_Xen_GCC/platform/xen/xen_setup.c`
 * Xenbus:
   * `Demo/CORTEX_A15_Xen_GCC/platform/xen/xenbus.c`
   * `Demo/CORTEX_A15_Xen_GCC/platform/xen/xenbus-arm.c`
 * Console:
   * Everything in `Demo/CORTEX_A15_Xen_GCC/platform/console/`
 * Grant tables:
   * `Demo/CORTEX_A15_Xen_GCC/platform/xen/gnttab.c`
   * `map_frames()` in `mmu.c`
 * Event channels:
   * `Demo/CORTEX_A15_Xen_GCC/platform/xen/events_setup.c`
   * `Demo/CORTEX_A15_Xen_GCC/platform/xen/events.c`

Debugging
---------

Many modules in the distribution provide extra Xen emergency console
output for debugging purposes. To enable such debugging output so that
it bypasses the Xen paravirtualized console and makes direct hypercalls
instead, declare a `DEBUG` constant at compile time:

```
  $ make -DDEBUG=1 ...
```

If you need to debug system state in assembly regions, some macros are
available:

 * `_dumpregs` (`dumpregs.s`) may be used to dump the register states
   for `R0-R12`, `LR`, `SPSR`, `CPSR`, and other useful system
   registers.
 * `_dumpstack` (`dumpregs.s`) may be used to dump the 8 most recent
   words on the current stack.

Relevant Configuration Settings
-------------------------------

The following settings in

```
Demo/CORTEX_A15_Xen_GCC/include/FreeRTOSConfig.h
```

may be relevant to your application:

   * `configUSE_XEN_CONSOLE`: if `1`, console output emitted with
     `printk` goes to the Xen paravirtualized console once it is up. If
     `0`, the output goes to the Xen "emergency" console instead, via the
     `HYPERVISOR_console_io` hypercall. Stable systems should set this to
     `1`; systems under development will be easier to debug if this is set to
     `0`, in case the console service is not coming up for some reason.
   * `configINTERRUPT_NESTING`: if `1`, enables interrupts of
     sufficiently high priority to preempt the execution of lower-priority
     interrupts. Defaults to `0`.
   * `configMAX_API_CALL_INTERRUPT_PRIORITY`: the highest interrupt
     priority from which interrupt safe FreeRTOS API functions can be called.
     For more information, please see http://www.freertos.org/a00110.html.
   * `configTICK_PRIORITY`: the priority of the tick interrupt raised by
     the ARM virtual timer.
   * `configXENBUS_TASK_PRIORITY`: the priority of the FreeRTOS task
     responsible for processing Xenbus responses.
   * `configEVENT_IRQ_PRIORITY`: the interrupt priority of the Xen event
     notification interrupt.
   * `configTICK_RATE_HZ_ASM`: the rate in ticks per second of the tick
     used to drive the FreeRTOS scheduler. This determines the timer interval
     used to set the virtual timer's interrupt.
   * `configTOTAL_HEAP_SIZE`: the size of the heap in bytes.

Memory Layout
-------------

The in-memory layout of the FreeRTOS system is as follows. For
information, see the linker script, `linker.lds`, and the various
assembly sources used to boot the system (in particular, `boot.s`).
The following table lists addresses with increasingly higher values,
indicating symbolic or numeric addresses where possible. This layout is
not intended to be exhaustive, but to provide a high-level view of the
organization of the program.

```
 Address         Segment     Description
 ------------------------------------------------------------------------
 (start)         .start      Start of kernel and execution entry point
 l1_page_table               Level 1 page table region (see boot.s)
 l2_page_table               Level 2 page table region (see boot.s)
                 .text       Text segments of compiled object code
                 .rodata     Read-only data
                 .data       Read-write data
 _start_stacks               Start of stack region (top of SVC stack)
 svc_stack                   SVC stack: _start_stacks + 16384
 irq_stack                   IRQ stack: _start_stacks + 32768
 firq_stack                  FIQ stack: _start_stacks + 49152
 abt_stack                   ABT stack: _start_stacks + 65536
 und_stack                   UND stack: _start_stacks + 81920
                 .bss        BSS region
```

All addresses in first three gigabytes of virtual memory address space
are mapped to the equivalent physical memory addresses (PA == VA). This
mapping is set up by mmu.c in the function `setup_direct_map`. The only
exceptions to this rule are:

 * The physical addresses of the 1MB regions containing the kernel
   binary
 * The virtual addresses of the 1MB regions containing the kernel
   binary

The virtual address of the kernel is determined by the start address
set by the linker script. Although the specific start address is not
important, what is important is that the kernel is aware that it is
probably not running at the desired virtual address at startup. We don't
know where Xen will place the kernel in physical memory. We only know
that it will be placed near the start of RAM. To deal with this, the
kernel determines the difference between its actual start address and
its virtual address, and sets up the MMU with mappings so that both
its physical and virtual start addresses map to the physical memory
containing the kernel. This technique was borrowed from Mini-OS since it
has the same requirement.

The fourth gigabyte of virtual address space is initially unmapped and
is reserved for mapping pages from grant tables and libIVC. For more
information, please see

 * `Demo/CORTEX_A15_Xen_GCC/include/freertos/mmu.h`
 * `Source/portable/GCC/ARM7_CA15_Xen/mmu.c`

Future Work & Caveats
---------------------

The following are known issues, missing features we'd like to implement,
or security issues we'd like to resolve. If you have the time and
inclination, feel free to work on these and submit pull requests!

 * Testing with real-time Xen: while FreeRTOS itself is suitable for
   real-time use, running FreeRTOS on Xen requires more investigation
   because the Xen scheduler will interfere with the real-time behavior of
   the guest. In particular, testing with Xen's real-time scheduler needs
   to be done to determine how reliable the guest performs real-time work.
   In the mean time, near-real-time behavior can be obtained by using CPU
   pinning to pin the FreeRTOS guest to a single CPU free of other Xen
   domains.

 * Configurable memory usage: the heap size is currently configurable
   using the `FreeRTOSConfig.h` setting `configTOTAL_HEAP_SIZE`, but the
   total available guest domain memory is not known to the guest. As with
   GIC configuration, this information can be obtained from the ARM device
   tree, but that work has not been done.

 * Per-task virtual address spaces: at present, only one virtual address
   space is used for the entire system, and each virtual address is mapped
   to its equivalent corresponding physical address. This was done to allow
   use of the caches and MMU for mapping grant table entries, but further
   use of virtual memory could be employed to provide per-task virtual
   address spaces.

 * Memory protection for relevant segments: `text`, `rodata`, and
   `start` segments are not protected in the MMU.

 * Optimizations are turned off: on some ARM cross compilers, enabling
   optimizations causes incorrect code generation for some C library
   routines. Optimizations are therefore disabled by default.

Contributing
------------

If you'd like to contribute to this effort, please submit issues or pull
requests via the GitHub repository page at

  https://github.com/GaloisInc/FreeRTOS-Xen

See Also
--------

There are many ports of FreeRTOS which are not included in this
distribution. For more information on the upstream FreeRTOS
distribution, please see:

 * http://www.freertos.org/FreeRTOS-quick-start-guide.html
 * http://www.freertos.org/FAQHelp.html
