#include <stdint.h>

#ifndef _NETWORKMANAGER_H_
#define _NETWORKMANAGER_H_

ssize_t wrapIPpackageInEthernet(uint8_t *acc_buf, size_t size, uint8_t *sendbuf);

#endif