/* 
 * Copyright (c) 2018 Tschokko. All rights reserved.
 */

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <strings.h>

#include "ovpn_client_config.h"

struct ovpn_client_network {
    address_family_t vpncn_family;
    union {
        struct in_addr vpncn_ipv4_addr;
        struct in6_addr vpncn_ipv6_addr;
    };
    size_t vpncn_prefix;
};

struct ovpn_client_route {
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
};

struct ovpn_client_config {
    struct in_addr vpncc_ipv4_addr;
    struct in_addr vpncc_ipv4_remote_addr;
    bool vpncc_has_ipv6_addr;
    struct in6_addr vpncc_ipv6_addr;
    size_t vpncc_ipv6_prefix;
    struct in6_addr vpncc_ipv6_remote_addr;
    vector_t *vpncc_networks;
    vector_t *vpncc_routes;
};

int
ovpn_client_config_alloc(ovpn_client_config_t **vpnccp, const char *ipv4_addr, 
                        const char *ipv4_remote_addr)
{
    int err = 0;

    if (vpnccp == NULL || ipv4_addr == NULL || ipv4_remote_addr == NULL) {
        return (EINVAL);
    }

    if ((*vpnccp = calloc(1, sizeof(ovpn_client_config_t))) == NULL) {
        return (ENOMEM);
    }

    /* Convert IPv4 addresses strings */
    if ((err = inetx_str_to_ipv4_addr(ipv4_addr, 
         &((*vpnccp)->vpncc_ipv4_addr))) != 0 ||
        (err = inetx_str_to_ipv4_addr(ipv4_remote_addr, 
         &((*vpnccp)->vpncc_ipv4_remote_addr))) != 0) {
        free(*vpnccp);
        return (err);
    }

    (*vpnccp)->vpncc_has_ipv6_addr = false;

    if ((err = vector_alloc(&((*vpnccp)->vpncc_networks), 
         sizeof(struct ovpn_client_network))) != 0) {
        free(*vpnccp);
        return (err);
    }

    if ((err = vector_alloc(&((*vpnccp)->vpncc_routes), 
         sizeof(struct ovpn_client_route))) != 0) {
        /* Important! Free previously allocated vector for networks */
        vector_free((*vpnccp)->vpncc_networks); 
        free(*vpnccp);
        return (err);
    }

    return (0);
}

void
ovpn_client_config_free(ovpn_client_config_t *vpncc)
{
    if (vpncc == NULL) {
        return;
    }

    vector_free(vpncc->vpncc_networks);
    vector_free(vpncc->vpncc_routes);

    free(vpncc);
}

static int
i_fprintf_vpncc_ifconfig_push(FILE *stream, const ovpn_client_config_t *vpncc)
{
    char ipv4_addr_str[INET_ADDRSTRLEN] = {0};
    int err = 0;

    assert(stream != NULL);
    assert(vpncc != NULL);

    /* Write the ifconfig-push option */
    fprintf(stream, "ifconfig-push");

    if ((err = inetx_ipv4_addr_to_str(&(vpncc->vpncc_ipv4_addr),
         ipv4_addr_str, sizeof(ipv4_addr_str))) != 0) {
        return (err);
    }
    fprintf(stream, " %s", ipv4_addr_str);

    if ((err = inetx_ipv4_addr_to_str(&(vpncc->vpncc_ipv4_remote_addr), 
         ipv4_addr_str, sizeof(ipv4_addr_str))) != 0) {
        return (err);
    }
    fprintf(stream, " %s", ipv4_addr_str);

    fprintf(stream, "\n");

    return (0);
}

static int
i_fprintf_vpncc_ifconfig_ipv6_push(FILE *stream, 
    const ovpn_client_config_t *vpncc)
{
    char ipv6_addr_str[INET6_ADDRSTRLEN] = {0};
    int err = 0;

    assert(stream != NULL);
    assert(vpncc != NULL);
    assert(vpncc->vpncc_has_ipv6_addr == true);

    /* Write the ifconfig-ipv6-push option */
    fprintf(stream, "ifconfig-ipv6-push");

    if ((err = inetx_ipv6_addr_to_str(&(vpncc->vpncc_ipv6_addr),
         ipv6_addr_str, sizeof(ipv6_addr_str))) != 0) {
        return (err);
    }
    fprintf(stream, " %s", ipv6_addr_str);

    fprintf(stream, "/%ld", vpncc->vpncc_ipv6_prefix);

    if (!IN6_IS_ADDR_UNSPECIFIED(&(vpncc->vpncc_ipv6_remote_addr))) {
        if ((err = inetx_ipv6_addr_to_str(&(vpncc->vpncc_ipv6_remote_addr), 
             ipv6_addr_str, sizeof(ipv6_addr_str))) != 0) {
            return (err);
        }
        fprintf(stream, " %s", ipv6_addr_str);
    } 
    

    fprintf(stream, "\n");

    return (0);
}

static int
i_fprintf_vpncc_ipv4_iroute(FILE *stream, const struct ovpn_client_network *network) 
{
    char addr_str[INET_ADDRSTRLEN] = {0};
    struct in_addr netmask = {0};
    int err = 0;

    assert(stream != NULL);
    assert(network != NULL);
    assert(network->vpncn_family == ADDRESS_FAMILY_IPV4);

    /* Write the iroute option */
    fprintf(stream, "iroute");

    if ((err = inetx_ipv4_addr_to_str(&(network->vpncn_ipv4_addr), addr_str, 
         INET_ADDRSTRLEN)) != 0) {
        return (err);
    }
    fprintf(stream, " %s", addr_str);

    /* Convert prefix to address */
    if ((err = inetx_ipv4_prefix_to_netmask(network->vpncn_prefix, &netmask)) 
        != 0) {
        return (err);
    }

    /* Write netmask only if it's not default 0xFFFFFFFF. */
    if (netmask.s_addr != INADDR_BROADCAST) {
        if ((err = inetx_ipv4_addr_to_str(&netmask, addr_str, INET_ADDRSTRLEN)) 
            != 0) {
            return (err);
        }
        fprintf(stream, " %s", addr_str);
    }

    fprintf(stream, "\n");

    return (0);
}

static int
i_fprintf_vpncc_ipv6_iroute(FILE *stream, const struct ovpn_client_network *network) 
{
    char ipv6_addr_str[INET6_ADDRSTRLEN] = {0};
    int err = 0;

    assert(stream != NULL);
    assert(network != NULL);
    assert(network->vpncn_family == ADDRESS_FAMILY_IPV6);

    /* Write the iroute-ipv6 option */
    fprintf(stream, "iroute-ipv6");

    if ((err = inetx_ipv6_addr_to_str(&(network->vpncn_ipv6_addr), 
         ipv6_addr_str, sizeof(ipv6_addr_str))) != 0) {
        return (err);
    }
    fprintf(stream, " %s", ipv6_addr_str);

    fprintf(stream, "/%ld", network->vpncn_prefix);

    fprintf(stream, "\n");

    return (0);
}

static int
i_fprintf_vpncc_iroute(FILE *stream, const struct ovpn_client_network *network)
{
    int err = 0;

    assert(stream != NULL);
    assert(network != NULL);

    if (network->vpncn_family == ADDRESS_FAMILY_IPV4 &&
        (err = i_fprintf_vpncc_ipv4_iroute(stream, network)) != 0) {
        return (err);
    }

    if (network->vpncn_family == ADDRESS_FAMILY_IPV6 &&
        (err = i_fprintf_vpncc_ipv6_iroute(stream, network)) != 0) {
        return (err);
    }

    return (0);
}

static int
i_fprintf_vpncc_iroutes(FILE *stream, const ovpn_client_config_t *vpncc)
{
    struct ovpn_client_network *elem;
    int err = 0;

    assert(stream != NULL);
    assert(vpncc != NULL);

    /* for (int i = 0; i < vpncc->vpncc_networks_size; i++) {
        if ((err = i_fprintf_vpncc_iroute(stream, &(vpncc->vpncc_networks[i])))
            != 0) {
            return (err);
        }
    } */
    for (elem = vector_begin(vpncc->vpncc_networks); 
         elem != vector_end(vpncc->vpncc_networks); 
         elem = vector_next(vpncc->vpncc_networks, elem)) {
        if ((err = i_fprintf_vpncc_iroute(stream, elem))
            != 0) {
            return (err);
        }
    }

    return (0);
}

static int
i_fprintf_vpncc_push_ipv4_route(FILE *stream, 
                                const struct ovpn_client_route *route)
{
    char addr_str[INET_ADDRSTRLEN] = {0};
    struct in_addr netmask = {0};
    int err = 0;

    assert(stream != NULL);
    assert(route != NULL);
    assert(route->vpncr_family == ADDRESS_FAMILY_IPV4);

    /* Write the push route option */
    fprintf(stream, "push \"route");

    if ((err = inetx_ipv4_addr_to_str(&(route->vpncr_ipv4_addr), addr_str, 
         INET_ADDRSTRLEN)) != 0) {
        return (err);
    }
    fprintf(stream, " %s", addr_str);


    /* Convert prefix to netmask. */
    if ((err = inetx_ipv4_prefix_to_netmask(route->vpncr_prefix, &netmask)) 
        != 0) {
        return (err);
    }

    /* Write netmask if netmask or gateway is not default. */
    if (netmask.s_addr != INADDR_BROADCAST || 
        route->vpncr_ipv4_gateway_addr.s_addr != INADDR_ANY) {
        if ((err = inetx_ipv4_addr_to_str(&netmask, addr_str, INET_ADDRSTRLEN)) 
            != 0) {
            return (err);
        }
        fprintf(stream, " %s", addr_str);
    }

    if (route->vpncr_ipv4_gateway_addr.s_addr != INADDR_ANY) {
        if ((err = inetx_ipv4_addr_to_str(&(route->vpncr_ipv4_gateway_addr), 
             addr_str, INET_ADDRSTRLEN)) != 0) {
            return (err);
        }
        fprintf(stream, " %s", addr_str);
    }

    if (route->vpncr_ipv4_gateway_addr.s_addr != INADDR_ANY && 
        route->vpncr_metric > 0) { 
        fprintf(stream, " %d", route->vpncr_metric);
    }

    fprintf(stream, "\"\n");

    return (0);
}

static int
i_fprintf_vpncc_push_ipv6_route(FILE *stream, 
                                const struct ovpn_client_route *route)
{
    char addr_str[INET6_ADDRSTRLEN] = {0};
    int err = 0;

    assert(stream != NULL);
    assert(route != NULL);
    assert(route->vpncr_family == ADDRESS_FAMILY_IPV6);

    /* Write the push route-ipv6 option */
    fprintf(stream, "push \"route-ipv6");

    if ((err = inetx_ipv6_addr_to_str(&(route->vpncr_ipv6_addr), addr_str, 
         INET6_ADDRSTRLEN)) != 0) {
        return (err);
    }
    fprintf(stream, " %s", addr_str);

    fprintf(stream, "/%ld", route->vpncr_prefix);

    if (!IN6_IS_ADDR_UNSPECIFIED(&(route->vpncr_ipv6_gateway_addr))) {
        if ((err = inetx_ipv6_addr_to_str(&(route->vpncr_ipv6_gateway_addr), 
             addr_str, INET6_ADDRSTRLEN)) != 0) {
            return (err);
        }
        fprintf(stream, " %s", addr_str);
    }

    if (!IN6_IS_ADDR_UNSPECIFIED(&(route->vpncr_ipv6_gateway_addr)) && 
        route->vpncr_metric > 0) {
        fprintf(stream, " %d", route->vpncr_metric);
    }
    fprintf(stream, "\"\n");

    return (0);
}

static int
i_fprintf_vpncc_push_route(FILE *stream, 
                           const struct ovpn_client_route *route)
{
    int err = 0;

    assert(stream != NULL);
    assert(route != NULL);

    if (route->vpncr_family == ADDRESS_FAMILY_IPV4 &&
        (err = i_fprintf_vpncc_push_ipv4_route(stream, route)) != 0) {
        return (err);
    }

    if (route->vpncr_family == ADDRESS_FAMILY_IPV6 &&
        (err = i_fprintf_vpncc_push_ipv6_route(stream, route)) != 0) {
        return (err);
    }

    return (0);
}

static int
i_fprintf_vpncc_push_routes(FILE *stream, const ovpn_client_config_t *vpncc)
{
    struct ovpn_client_route *elem = NULL;
    int err = 0;

    assert(stream != NULL);
    assert(vpncc != NULL);

    /* for (int i = 0; i < vpncc->vpncc_routes_size; i++) {
        if ((err = i_fprintf_vpncc_push_route(stream, 
             &(vpncc->vpncc_routes[i]))) != 0) { 
            return (err);
        }
    } */
    for (elem = vector_begin(vpncc->vpncc_routes);
        elem != vector_end(vpncc->vpncc_routes);
        elem = vector_next(vpncc->vpncc_routes, elem)) {
        if ((err = i_fprintf_vpncc_push_route(stream, elem)) != 0) { 
            return (err);
        }
    }

    return (0);
}

/*
 * Writes the OpenVPN client config to a stream
 */
int
ovpn_client_config_build(ovpn_client_config_t *vpncc, FILE *stream)
{
    FILE *local_stream = NULL;
    char *local_buf = NULL;
    size_t local_sz = 0;
    int err = 0;  /* Set to 0 otherwise on success function returns an error. */

    if (vpncc == NULL || stream == NULL) {
        return (EINVAL);
    }
    
    /* Create a local stream to buffer the configuration output */
    if (((local_stream = open_memstream(&local_buf, &local_sz))) == NULL) {
        return (errno);
    }

    /* Write the ifconfig-push option */
    if ((err = i_fprintf_vpncc_ifconfig_push(local_stream, vpncc)) != 0) {
        goto out_free_buffer;
    }

    if (vpncc->vpncc_has_ipv6_addr == true &&
        (err = i_fprintf_vpncc_ifconfig_ipv6_push(local_stream, vpncc)) != 0) {
        goto out_free_buffer;
    }

    /* Write the iroute entries */
    if ((err = i_fprintf_vpncc_iroutes(local_stream, vpncc)) != 0) {
        goto out_free_buffer;
    }

    /* Write the push route entries */
    if ((err = i_fprintf_vpncc_push_routes(local_stream, vpncc)) != 0) {
        goto out_free_buffer;
    }

    /* Flush the local stream and write local buffer to given stream */
    fflush(local_stream);
    fprintf(stream, "%s", local_buf);

out_free_buffer:
    fclose(local_stream);
    free(local_buf);
    return (err);
}

/*
 * Set the IPv6 address of the client.
 */
int
ovpn_client_config_set_ipv6_addr(ovpn_client_config_t *vpncc, 
    const char *ipv6_addr, const char *ipv6_remote_addr)
{
    int err = 0;

    if (vpncc == NULL || ipv6_addr == NULL)
        return (EINVAL);

    if ((err = inetx_parse_ipv6_cidr(ipv6_addr, &(vpncc->vpncc_ipv6_addr), 
         &(vpncc->vpncc_ipv6_prefix))) != 0) {
        return (err); 
    }

    if (ipv6_remote_addr != NULL &&
        (err = inetx_str_to_ipv6_addr(ipv6_remote_addr, 
         &(vpncc->vpncc_ipv6_remote_addr))) != 0) {
        return (err);
    }

    vpncc->vpncc_has_ipv6_addr = true;

    return (0);
}

int
ovpn_client_config_add_ipv4_network(ovpn_client_config_t *vpncc, 
                                    const char *str)
{
    struct ovpn_client_network entry = {0};
    int err = 0;

    if (vpncc == NULL || str == NULL) {
        return (EINVAL);
    }

    /* Set the corresponding address family. */
    entry.vpncn_family = ADDRESS_FAMILY_IPV4;

    /* Parse the network CIDR to get IP and prefix */
    if ((err = inetx_parse_ipv4_cidr(str, &(entry.vpncn_ipv4_addr), 
         &(entry.vpncn_prefix))) != 0) {
        return (err);
    }

    /* Finally add the new entry to the vector and return the result. */
    return (vector_push_back(vpncc->vpncc_networks, &entry));
}

int
ovpn_client_config_add_ipv6_network(ovpn_client_config_t *vpncc, 
                                    const char *str)
{
    struct ovpn_client_network entry = {0};
    int err = 0;

    if (vpncc == NULL || str == NULL) {
        return (EINVAL);
    }

    /* Set the corresponding address family. */
    entry.vpncn_family = ADDRESS_FAMILY_IPV6;

    /* Parse address and prefix. */
    if ((err = inetx_parse_ipv6_cidr(str, &(entry.vpncn_ipv6_addr), 
         &(entry.vpncn_prefix))) != 0) {
        return (err);
    }

    /* Finally add the new entry to the vector and return the result. */
    return (vector_push_back(vpncc->vpncc_networks, &entry));
}

int
ovpn_client_config_add_network(ovpn_client_config_t *vpncc, const char *str)
{
    int err = 0, af = 0;

    if (vpncc == NULL || str == NULL) {
        return (EINVAL);
    }

    /* Predict address familiy */
    if ((err = inetx_predict_address_family(str, &af)) != 0) {
        return (err);
    }

    /* Add new network based on family. */
    switch (af) {
    case AF_INET:
        err  = ovpn_client_config_add_ipv4_network(vpncc, str);
        break;
    case AF_INET6:
        err = ovpn_client_config_add_ipv6_network(vpncc, str);
        break;
    default:
        err = ENOTSUP;
        break;
    }

    return (err);
}

int
ovpn_client_config_add_ipv4_route(ovpn_client_config_t *vpncc, const char *str, 
                                 const char *gateway_str, short metric)
{
    struct ovpn_client_route entry = {0};
    int err = 0;

    if (vpncc == NULL || str == NULL) {
        return (EINVAL);
    }

    /* Set the corresponding address family */
    entry.vpncr_family = ADDRESS_FAMILY_IPV4;

    if ((err = inetx_parse_ipv4_cidr(str, &(entry.vpncr_ipv4_addr), 
         &(entry.vpncr_prefix))) != 0) {
        return (err);
    }

    if (gateway_str != NULL && (err = inetx_str_to_ipv4_addr(gateway_str, 
         &(entry.vpncr_ipv4_gateway_addr))) != 0) {
        return (err);
    }

    if (gateway_str == NULL) {
        entry.vpncr_ipv4_gateway_addr.s_addr = INADDR_ANY;
    }

    /* If invalid metric or metric without a gateway is set, then error. */
    if (metric < 0 || (metric > 0 && 
         entry.vpncr_ipv4_gateway_addr.s_addr == INADDR_ANY)) {
        return (EINVAL);
    }

    entry.vpncr_metric = metric;

    /* Finally add the new entry to the vector and return the result. */
    return (vector_push_back(vpncc->vpncc_routes, &entry));
}

int 
ovpn_client_config_add_ipv6_route(ovpn_client_config_t *vpncc, const char *str, 
                                 const char *gateway_str, short metric)
{
    struct ovpn_client_route entry = {0};
    int err = 0;

    if (vpncc == NULL || str == NULL) {
        return (EINVAL);
    }

    /* Set the corresponding address family */
    entry.vpncr_family = ADDRESS_FAMILY_IPV6;

    if ((err = inetx_parse_ipv6_cidr(str, &(entry.vpncr_ipv6_addr), 
         &(entry.vpncr_prefix))) != 0) {
        return (err);
    }

    if (gateway_str != NULL &&
        (err = inetx_str_to_ipv6_addr(gateway_str, 
         &(entry.vpncr_ipv6_gateway_addr))) != 0) {
        return (err);
    }

    if (gateway_str == NULL) {
        /* TODO: Check if assignment is done properly */
        entry.vpncr_ipv6_gateway_addr = in6addr_any;
    }

    /* If invalid metric or metric without a gateway is set, then error. */
    if (metric < 0 || (metric > 0 && 
         IN6_IS_ADDR_UNSPECIFIED(&(entry.vpncr_ipv6_gateway_addr)))) {
        return (EINVAL);
    }

    entry.vpncr_metric = metric;

    /* Finally add the new entry to the vector and return the result. */
    return (vector_push_back(vpncc->vpncc_routes, &entry));
}

int
ovpn_client_config_add_route(ovpn_client_config_t *vpncc, const char *str,
                            const char *gateway_str, short metric)
{
    int err = 0, af = 0, af_gateway = 0;

    if (vpncc == NULL || str == NULL) {
        return (EINVAL);
    }

    /* Predict address familiy */
    if ((err = inetx_predict_address_family(str, &af)) != 0) {
        return (err);
    }

    /* Predict address familiy of gateway, if gateway is set. */
    if (gateway_str != NULL && 
        (err = inetx_predict_address_family(gateway_str, &af_gateway)) != 0) {
        return (err);
    }

    /* The address family of addr and gateway_addr has to be the same! */
    if (gateway_str != NULL && (af != af_gateway)) {
        return (EINVAL);
    }

    /* Add new route based on family. */
    switch (af) {
    case AF_INET:
        err = ovpn_client_config_add_ipv4_route(vpncc, str, gateway_str, 
            metric);
        break;
    case AF_INET6:
        err = ovpn_client_config_add_ipv6_route(vpncc, str, gateway_str, 
            metric);
        break;
    default:
        err = ENOTSUP;
        break;
    }

    return (err);
}
