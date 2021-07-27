#ifndef __WIFI_H
#define __WIFI_H

#include <esp_err.h>
#include <lwip/ip4_addr.h>
#include <stdbool.h>

esp_err_t wifi_init();

/**
 * Checks if the specified peer is on the same network as the device.
 * 
 * @param peer_ip IP4 address of the peer
 */
bool is_peer_local(ip4_addr_t peer_ip);

#endif