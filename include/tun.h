/*
 * SimpleRT: Reverse tethering utility for Android
 * Copyright (C) 2016 Konstantin Menyaev
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _TUN_H_
#define _TUN_H_

#include <stdint.h>
#include <stdbool.h>

bool is_tun_present(void);
int tun_alloc(char *dev_name, size_t dev_name_size);
ssize_t tun_read_ip_packet(int fd, uint8_t *packet, size_t size);
ssize_t tun_write_ip_packet(int fd, const uint8_t *packet, size_t size);

#endif
