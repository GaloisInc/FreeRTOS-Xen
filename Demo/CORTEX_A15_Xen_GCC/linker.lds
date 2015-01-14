OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS 
{
  os_start = 0x80008000;

  . = os_start;

  .text : {
    *(.start);
    *(.text);
  }

  . = ALIGN(4);

  .rodata : { *(.rodata); }
  .data : { *(.data); }

  . = ALIGN(4);

  /* NOTE: The space specified in the zImage header (_end - _start) MUST be no
   * bigger than the binary image size of the entire kernel. */
  _end = .;

  . = ALIGN(8);
  _start_stacks = .;
  svc_stack = _start_stacks + 16384;
  irq_stack = _start_stacks + 32768;
  firq_stack = _start_stacks + 49152;
  abt_stack = _start_stacks + 65536;
  und_stack = _start_stacks + 81920;

  . = und_stack;

  . = ALIGN(4);
  _bss_start = .;
  .bss : { *(.bss); }
  . = ALIGN(4);
  _bss_end = .;
}
