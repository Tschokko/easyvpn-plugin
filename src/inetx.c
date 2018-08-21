/* 
 * Copyright (c) 2018 Tschokko. All rights reserved.
 */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <strings.h>
#include <arpa/inet.h>

#include "inetx.h"

/* 
 * i_str_to_ip_addr is a helper function to convert a string to an in_addr or
 * in6_addr address struct.  
 */ 
static int
i_str_to_ip_addr(int af, const char *str, void *addr, size_t addr_sz)
{
    if (af != AF_INET && af != AF_INET6) {
        return (ENOTSUP);
    }

    if (str == NULL || addr == NULL || addr_sz == 0) {
        return (EINVAL);
    }

    /* Zero in_addr, sometimes the system network functions aren't safe! */
    memset(addr, 0, addr_sz);

    /* Convert string to address */
    if (inet_pton(af, str, addr) != 1) {
        return (EINVAL);
    }

    return (0);
}

/* 
 * inetx_str_to_ipv4_addr converts a string to an in_addr address struct.
 */ 
int
inetx_str_to_ipv4_addr(const char *str, struct in_addr *addr)
{
    return (i_str_to_ip_addr(AF_INET, str, addr, sizeof(struct in_addr)));
}

/* 
 * inetx_str_to_ipv4_addr converts a string to an in6_addr address struct.
 */ 
int
inetx_str_to_ipv6_addr(const char *str, struct in6_addr *addr)
{
    return (i_str_to_ip_addr(AF_INET6, str, addr, sizeof(struct in6_addr)));
}

/* 
 * i_ip_addr_to_str is a helper function to convert an in_addr or in6_addr
 * address struct to a string.
 */ 
static int
i_ip_addr_to_str(int af, const void *addr, char *str, size_t str_sz)
{
    int err = 0;

    if (af != AF_INET && af != AF_INET6) {
        return (ENOTSUP);
    }

    if (addr == NULL || str == NULL || str_sz == 0 ||
        (af == AF_INET && str_sz < INET_ADDRSTRLEN) || 
        (af == AF_INET6 && str_sz < INET6_ADDRSTRLEN)) {
        return (EINVAL);
    }

    /* Convert address to string */
    if (inet_ntop(af, addr, str, str_sz) == NULL) {
        return (errno);
    }

    return (0);
}

/* 
 * inetx_ipv4_addr_to_str converts an in_addr address struct to a string.
 */
int
inetx_ipv4_addr_to_str(const struct in_addr *addr, char *str, size_t str_sz)
{
    return (i_ip_addr_to_str(AF_INET, addr, str, str_sz));
}

/* 
 * inetx_ipv6_addr_to_str converts an in6_addr address struct to a string.
 */
int
inetx_ipv6_addr_to_str(const struct in6_addr *addr, char *str, size_t str_sz)
{
    return (i_ip_addr_to_str(AF_INET6, addr, str, str_sz));
}

/*
 * i_parse_cidr_fallback parses an IP CIDR string the naive way with strtok.
 */
static int
i_parse_cidr_fallback(int af, const char *cidr, void *addr, size_t addr_sz, 
                      size_t *prefix)
{
    char *cidr_dup = NULL, *cidr_addr = NULL, *cidr_prefix = NULL, 
         *cidr_prefix_ptr = NULL;
    char delim[] = "/";
    int err = 0;  /* Set to 0 otherwise on success function returns an error. */

    if (af != AF_INET && af != AF_INET6) {
        return (ENOTSUP);
    }

    if (cidr == NULL || addr == NULL || prefix == NULL) {
        return (EINVAL);
    }

    /* Make a writeable copy for strtok */
    cidr_dup = strdup(cidr);

    /* Fetch the IP address of the CIDR */
    if ((cidr_addr = strtok(cidr_dup, delim)) == NULL) {
        err = EINVAL;
        goto out_free_buffer;
    }

    /* Fetch the prefix of the CIDR */
    if ((cidr_prefix = strtok(NULL, delim)) == NULL) {
        err = EINVAL;
        goto out_free_buffer;
    }

    /* Next token should be NULL */
    if (strtok(NULL, delim) != NULL) {
        err = EINVAL;
        goto out_free_buffer;
    }

    /* Convert IP string to address */
    if ((err = i_str_to_ip_addr(af, cidr_addr, addr, addr_sz)) != 0) {
        goto out_free_buffer;
    }

    /* Convert and check the prefix */
    *prefix = strtoul(cidr_prefix, &cidr_prefix_ptr, 10);
    if ((cidr_prefix == cidr_prefix_ptr) || /* No conversion happend */
        (af == AF_INET && *prefix > 32) ||
        (af == AF_INET6 && *prefix > 128)) {
        err = EINVAL;
        goto out_free_buffer;
    }

out_free_buffer:
    free(cidr_dup);
    return (err);
}

/*
 * i_parse_cidr parses an IP CIDR string with the provided system function 
 * inet_net_pton. On error it fallbacks to i_parse_cidr_fallback function. This  
 * is often the case if a host address, and not a network address, with a prefix 
 * is parsed instead.
 */ 
static int
i_parse_cidr(int af, const char *cidr, void *addr, size_t addr_sz, 
             size_t *prefix)
{
    int err = 0;

    if (af != AF_INET && af != AF_INET6) {
        return (ENOTSUP);
    }

    if (cidr == NULL || addr == NULL || prefix == NULL) {
        return (EINVAL);
    }

    /* Zero in_addr, sometimes the system network functions aren't safe! */
    memset(addr, 0, addr_sz);

    if((err = inet_net_pton(af, cidr, addr, addr_sz)) == -1) {
        return (i_parse_cidr_fallback(af, cidr, addr, addr_sz, prefix));
    }

    /* The prefix is stored in return value of inet_net_pton. */
    *prefix = err;

    return (0);
}

/* 
 * inetx_parse_ipv4_cidr parses an IPv4 CIDR string and converts it to an
 * in_addr address struct and a prefix.
 */
int
inetx_parse_ipv4_cidr(const char *cidr, struct in_addr *addr, size_t *prefix)
{
    return (i_parse_cidr(AF_INET, cidr, addr, sizeof(struct in_addr), prefix));
}

/* 
 * inetx_parse_ipv6_cidr parses an IPv6 CIDR string and converts it to an
 * in6_addr address struct and a prefix.
 */
int
inetx_parse_ipv6_cidr(const char *cidr, struct in6_addr *addr, size_t *prefix)
{
    return (i_parse_cidr(AF_INET6, cidr, addr, sizeof(struct in6_addr), 
        prefix));
}

/* 
 * inetx_ipv4_prefix_to_netmask converts an IPv4 prefix to an in_addr address
 * struct, which is used as an IP netmask address.
 */
int 
inetx_ipv4_prefix_to_netmask(size_t prefix, struct in_addr *netmask)
{
    if (netmask == NULL) {
        return (EINVAL);
    }

    if (prefix > 32) {
        return (EINVAL);
    }

    /* Zero in_addr, sometimes the system network functions aren't safe! */
    memset(netmask, 0, sizeof(struct in_addr));

    if (prefix) {
        netmask->s_addr = htonl(~((1 << (32 - prefix)) - 1));
    }
    else {
        netmask->s_addr = htonl(0);
    }

    return (0);
}

/* 
 * inetx_predict_address_family analyzes an string for it's possible address
 * family. Currently only IPv4 or IPv6 address strings are supported.
 */
int
inetx_predict_address_family(const char *str, int *af)
{
    char *found = NULL;
    int c = 0;

    if (af == NULL) {
        return (EINVAL);
    }

    /* Watch for a colon otherwise for a dot */
    if ((found = strchr(str, ':')) != NULL) {
        /* Check for an IPv6 address. It should contain at least two colons. */
        c = 1;
        while ((found = strchr(found, ':')) != NULL) {
            c++; /* Increment colon counter */
            found++;
        }

        if (c < 2) {
            return (EINVAL);
        }

        *af = AF_INET6;
    } else if ((found = strchr(str, '.')) != NULL) {
        /* Check for an IPv4 address. It should contain exact four dots. */
        c = 1;
        while ((found = strchr(found, '.')) != NULL) {
            c++; /* Increment colon counter */
            found++;
        }

        if (c != 4) {
            return (EINVAL);
        }

        *af = AF_INET;
    } else {
        /* Wether a colon nor a dot was found. Not a supported address. */
        return (ENOTSUP);
    }

    return (0);
}

