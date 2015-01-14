
# print msg prints the string referred to by msg to the Xen console.  Example:
#
# .section ".data"
# foo:
#   .asciz "bar\n"
# 
#   print foo
#
# Note that the string MUST be declared using 'asciz', not 'ascii', to be
# null-terminated; otherwise the call to printk will overrun the end of the
# string.
.macro print msg
    push   {r0, r1, lr}
    ldr    r0, =\msg
    bl     printk
    pop    {r0, r1, lr}
.endm
