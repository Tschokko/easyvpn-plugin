/* 
 * Copyright (c) 2018 Tschokko. All rights reserved.
 */

#ifndef EASYVPN_PLUGIN_OVPN_CLIENT_CONFIG_H_
#define EASYVPN_PLUGIN_OVPN_CLIENT_CONFIG_H_

#include <stdbool.h>
#include <stdint.h>
#include <netinet/in.h>

#include "inetx.h"
#include "vector.h"

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct ovpn_client_config ovpn_client_config_t;

int ovpn_client_config_alloc(ovpn_client_config_t **, const char *, const char *);
void ovpn_client_config_free(ovpn_client_config_t *);
int ovpn_client_config_build(ovpn_client_config_t *, FILE *);
int ovpn_client_config_set_ipv6_addr(ovpn_client_config_t *, const char *, 
    const char *);

int ovpn_client_config_add_ipv4_network(ovpn_client_config_t *, const char *);
int ovpn_client_config_add_ipv6_network(ovpn_client_config_t *, const char *);
int ovpn_client_config_add_network(ovpn_client_config_t *, const char *);

int ovpn_client_config_add_ipv4_route(ovpn_client_config_t *, const char *, 
    const char *, short);
int ovpn_client_config_add_ipv6_route(ovpn_client_config_t *, const char *, 
    const char *, short);
int ovpn_client_config_add_route(ovpn_client_config_t *, const char *, 
    const char *, short);

#ifdef	__cplusplus
}
#endif

#endif  /* EASYVPN_PLUGIN_OVPN_CLIENT_CONFIG_H_ */
