/* 
 * Copyright (c) 2018 Tschokko. All rights reserved.
 */

#ifndef EASYVPN_PLUGIN_INETX_H_
#define EASYVPN_PLUGIN_INETX_H_

#include <stdint.h>
#include <arpa/inet.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef enum {
    ADDRESS_FAMILY_IPV4 = AF_INET,
    ADDRESS_FAMILY_IPV6 = AF_INET6
} address_family_t;

int inetx_parse_ipv4_cidr(const char *, struct in_addr *, size_t *);
int inetx_parse_ipv6_cidr(const char *, struct in6_addr *, size_t *);
int inetx_str_to_ipv4_addr(const char *, struct in_addr *);
int inetx_str_to_ipv6_addr(const char *, struct in6_addr *);
int inetx_ipv4_addr_to_str(const struct in_addr *, char *, size_t);
int inetx_ipv6_addr_to_str(const struct in6_addr *, char *, size_t);
int inetx_ipv4_prefix_to_netmask(size_t, struct in_addr *);
int inetx_predict_address_family(const char *, int *);

#ifdef	__cplusplus
}
#endif

#endif  /* EASYVPN_PLUGIN_INETX_H_ */
