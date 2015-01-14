
#include <platform/xen_setup.h>

/* Called by boot.s to set up platform-specific services. */
void platform_setup()
{
    xen_setup();
}
