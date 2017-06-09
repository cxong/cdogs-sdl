// MinGW is missing inet_pton(), so provide it here
// TODO: once MinGW implements it, remove this
# pragma once

int inet_pton_mingw(int af, const char *src, struct in_addr *dst);
