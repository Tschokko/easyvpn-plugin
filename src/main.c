/* 
 * Copyright (c) 2018 Tschokko. All rights reserved.
 */

#include <errno.h>
#include <msgpack.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <strings.h>
#include <arpa/inet.h>

#include "ovpn_client_config.h"
#include "dao.h"
#include "vector.h"
#include "inetx.h"
#include "model.h"

int
main(int argc, char **argv)
{
    vector_t *vec1 = NULL;
    struct in6_addr addr = {}, *elem = NULL;
    char str[INET6_ADDRSTRLEN] = {0};

    vector_alloc(&vec1, sizeof(struct in6_addr));

    inetx_str_to_ipv6_addr("2001:db8:10::1", &addr);
    vector_push_back(vec1, &addr);

    inetx_str_to_ipv6_addr("2001:db8:20::1", &addr);
    vector_push_back(vec1, &addr);

    for (elem = vector_begin(vec1); elem != vector_end(vec1); 
         elem = vector_next(vec1, elem)) {
        inetx_ipv6_addr_to_str(elem, str, INET6_ADDRSTRLEN);
        printf("elem: %s\n", str);
    }

    vector_free(vec1);

    /* ====================================================================== */

    /* ovpn_client_config_t *client1;
    ovpn_client_config_alloc(&client1, "192.168.10.1", "255.255.255.0");

    ovpn_client_config_add_network(client1, "192.168.20.0/24");
    ovpn_client_config_add_network(client1, "192.168.30.0/24");
    ovpn_client_config_add_network(client1, "192.168.40.0/32");
    ovpn_client_config_add_network(client1, "0.0.0.0/0");
    ovpn_client_config_add_network(client1, "2001:db8:20::/64");
    ovpn_client_config_add_network(client1, "2001:db8:30::/64");
    ovpn_client_config_add_network(client1, "2001:db8:40::");
    ovpn_client_config_add_network(client1, "::/0");

    ovpn_client_config_add_route(client1, "192.168.90.1", NULL, 0);
    ovpn_client_config_add_route(client1, "192.168.90.1/32", NULL, 0);
    ovpn_client_config_add_route(client1, "192.168.90.1/0", NULL, 0);
    ovpn_client_config_add_route(client1, "0.0.0.0/0", NULL, 0);
    ovpn_client_config_add_route(client1, "192.168.30.0/24", NULL, 0);
    ovpn_client_config_add_route(client1, "192.168.40.0/24", "192.168.254.1", 0);
    ovpn_client_config_add_route(client1, "192.168.50.0/24", "192.168.254.1", 10);
    ovpn_client_config_add_route(client1, "DEAD:BEEF:7654:3210:FEDC:3210:7654:BA98", NULL, 0);
    ovpn_client_config_add_route(client1, "2001:db8:85a3::/56", "2001:db8:0:0:1::1", 0);
    ovpn_client_config_add_route(client1, "2001:db8:85a4::/56", "2001:db8:0:0:1::2", 10);
    ovpn_client_config_add_route(client1, "::/0", "2001:db8:0:0:1::2", 10);

    ovpn_client_config_build(client1, stdout); */

    /* ====================================================================== */

    /* dao_config_t *dao1;
    struct vpn_client vpn_client1;
    struct vpn_client_network *vpn_client_network_elem;
    vector_t *vpn_client1_networks;
    ovpn_client_config_t *ovpn_client1;

    vector_alloc(&vpn_client1_networks, sizeof(struct vpn_client_network));

    dao_alloc(&dao1, "./easyvpn.db");
    dao_db_open(dao1);
    dao_vpn_client_find_by_cn(dao1, "client1", &vpn_client1);
    dao_vpn_client_network_find_by_client_id(dao1, vpn_client1.id, 
        vpn_client1_networks);
    dao_free(dao1);

    ovpn_client_config_alloc(&ovpn_client1, vpn_client1.ipv4_addr, 
        vpn_client1.ipv4_remote_addr);
    for (vpn_client_network_elem = vector_begin(vpn_client1_networks);
        vpn_client_network_elem != vector_end(vpn_client1_networks);
        vpn_client_network_elem = vector_next(vpn_client1_networks, vpn_client_network_elem)) {
        ovpn_client_config_add_network(ovpn_client1, vpn_client_network_elem->network_addr);   
    }
    vector_free(vpn_client1_networks);

    ovpn_client_config_build(ovpn_client1, stdout); 
    ovpn_client_config_free(ovpn_client1); */

    msgpack_sbuffer* buffer = msgpack_sbuffer_new();
    msgpack_packer* pk = msgpack_packer_new(buffer, msgpack_sbuffer_write);

    /* msgpack_pack_array(pk, 2);
    msgpack_pack_bin(pk, 5);
    msgpack_pack_bin_body(pk, "Hello", 5);
    msgpack_pack_bin(pk, 11);
    msgpack_pack_bin_body(pk, "MessagePack", 11); */
    
    msgpack_pack_array(pk, 7);

    // cn
    msgpack_pack_str(pk, 7);
    msgpack_pack_str_body(pk, "client1", 7);

    // is_active
    msgpack_pack_true(pk);

    // ipv4_addr
    msgpack_pack_str(pk, 12);
    msgpack_pack_str_body(pk, "192.168.10.1", 12);

    // ipv4_remote_addr
    msgpack_pack_str(pk, 13);
    msgpack_pack_str_body(pk, "255.255.255.0", 13);

    // ipv6_addr
    msgpack_pack_nil(pk);

    // ipv6_remote_addr
    msgpack_pack_nil(pk);

    // networks
    msgpack_pack_array(pk, 2);
    // network #1
    msgpack_pack_str(pk, 15);
    msgpack_pack_str_body(pk, "192.168.20.0/24", 15);
    // network #2
    msgpack_pack_str(pk, 16);
    msgpack_pack_str_body(pk, "2001:db8:20::/64", 16);

    msgpack_unpacked msg;
    msgpack_unpacked_init(&msg);
    msgpack_unpack_return ret = msgpack_unpack_next(&msg, buffer->data, buffer->size, NULL);

    msgpack_object obj = msg.data;
    msgpack_object_print(stdout, obj);
    printf("\n");

    msgpack_sbuffer_free(buffer);
    msgpack_packer_free(pk);

    return (0);
}
