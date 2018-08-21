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

typedef struct ovpn_client_network_s {
    address_family_t vpncn_family;
    union {
        struct in_addr vpncn_ipv4_addr;
        struct in6_addr vpncn_ipv6_addr;
    };
    size_t vpncn_prefix;
} ovpn_client_network_t;

typedef struct ovpn_client_route_s {
    address_family_t vpncr_family;
    union {
        struct {
            struct in_addr vpncr_ipv4_addr;
            struct in_addr vpncr_ipv4_gateway_addr;
        };
        struct {
            struct in6_addr vpncr_ipv6_addr;
            struct in6_addr vpncr_ipv6_gateway_addr;
        };
    };
    size_t vpncr_prefix;
    short vpncr_metric;
} ovpn_client_route_t;

typedef struct ovpn_client_config_s {
    struct in_addr vpncc_ipv4_addr;
    struct in_addr vpncc_ipv4_remote_addr;
    bool vpncc_has_ipv6_addr;
    struct in6_addr vpncc_ipv6_addr;
    size_t vpncc_ipv6_prefix;
    struct in6_addr vpncc_ipv6_remote_addr;
    vector_t *vpncc_networks;
    vector_t *vpncc_routes;
} ovpn_client_config_t;

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
