// emu shim: the device sysroot exposes libuuid as <uuid.h>; on a normal host
// distro it lives at <uuid/uuid.h>. Redirect so unmodified zero-sdk sources
// (#include <uuid.h>) resolve against the host's libuuid.
#include <uuid/uuid.h>
