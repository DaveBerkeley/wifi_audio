
#pragma once

// sockets / network
#if defined(ARCH_LINUX)
#include <arpa/inet.h>
#else
// esp32
#include "lwip/sockets.h"
#endif

//  FIN
