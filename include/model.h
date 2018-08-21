/* 
 * Copyright (c) 2018 TSCHOKKO. All rights reserved.
 */

#ifndef EASYVPN_PLUGIN_MODEL_H_
#define EASYVPN_PLUGIN_MODEL_H_

#include <netinet/in.h>

#ifdef	__cplusplus
extern "C" {
#endif

#define RFC5280_CN_MAX_LENGTH     64
#define INET_ADDRSTRLEN_W_PREFIX  19
#define INET6_ADDRSTRLEN_W_PREFIX 50

struct vpn_client {
    int id;
    char cn[RFC5280_CN_MAX_LENGTH];
    int is_active; /* Boolean: 0 || 1 */
    char ipv4_addr[INET_ADDRSTRLEN];
    char ipv4_remote_addr[INET_ADDRSTRLEN];
    char ipv6_addr[INET6_ADDRSTRLEN_W_PREFIX];
    char ipv6_remote_addr[INET6_ADDRSTRLEN];
    int prefix;
};

struct vpn_client_network {
    int id;
    int client_id;
    int family;
    char network_addr[INET6_ADDRSTRLEN_W_PREFIX];
};

#ifdef	__cplusplus
}
#endif

#endif  /* EASYVPN_PLUGIN_MODEL_H_ */
