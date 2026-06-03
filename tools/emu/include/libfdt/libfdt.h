// emu shim: device sysroot exposes libfdt under <libfdt/libfdt.h>; the host
// dtc package installs libfdt.h at the top level.
#include <libfdt.h>
