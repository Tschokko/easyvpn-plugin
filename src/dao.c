/* 
 * Copyright (c) 2018 Tschokko. All rights reserved.
 */

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>

#include "dao.h"

/* 
 * dao_config contains all attributes to connect the SQLite database and it's 
 * opaque to prevent accidental access or unexpected behavior. 
*/
struct dao_config {
    char *db_filename;
    sqlite3 *db;
};

/* 
 * dao_alloc allocates a new dao_config and stores the SQLite database filename. 
 */ 
int
dao_alloc(dao_config_t **daocfgp, const char *db_filename)
{
    size_t db_filename_sz = 0;
    int err = 0;

    if (daocfgp == NULL && db_filename == NULL) {
        return (EINVAL);
    }

    if ((*daocfgp = calloc(1, sizeof(dao_config_t))) == NULL) {
        return (ENOMEM);
    }

    (*daocfgp)->db = NULL;

    db_filename_sz = strlen(db_filename) + 1;

    /* Allocate memory for the filename */
    if(((*daocfgp)->db_filename = calloc(db_filename_sz, sizeof(char))) 
        == NULL) {
        free(*daocfgp);
        *daocfgp = NULL;
        return (ENOMEM);
    }

    /* TODO: Should use a strcpy function instead of binary copy! */
    bcopy(db_filename, (*daocfgp)->db_filename, db_filename_sz * sizeof(char));

    return (0);
}

/* 
 * dao_free closes the SQLite database and frees the dao_config. 
 */ 
void
dao_free(dao_config_t *daocfg)
{
    if (daocfg == NULL) {
        return;
    }

    /* Close the db if opened */
    dao_db_close(daocfg);

    if (daocfg->db_filename != NULL) {
        free(daocfg->db_filename);
    }

    free(daocfg);
}

/* 
 * dao_db_open opens the SQLite database and stores the handler in the 
 * dao_config. 
 */ 
int
dao_db_open(dao_config_t *daocfg)
{
    if (daocfg == NULL) {
        return (EINVAL);
    }
    
    if (sqlite3_open(daocfg->db_filename, &(daocfg->db)) != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(daocfg->db));
        sqlite3_close(daocfg->db);

        /* Reset db pointer to NULL */
        daocfg->db = NULL;

        return (EIO);
    }

    return (0);
}

/* 
 * dao_db_close closes the SQLite database if open. 
 */ 
int
dao_db_close(dao_config_t *daocfg)
{
    if (daocfg == NULL) {
        return (EINVAL);
    }

    if (daocfg->db != NULL) {
        sqlite3_close(daocfg->db);
        
        /* Reset db pointer to NULL */
        daocfg->db = NULL;
    }

    return (0);
}

int
dao_create_vpn_client(dao_config_t *daocfg, const char *cn, 
    const char *ipv4_addr, const char *ipv4_remote_addr, const char *ipv6_addr, 
    const char *ipv6_remote_addr)
{
    sqlite3_stmt *stmt = NULL;
    int err = 0;

    if (daocfg == NULL || cn == NULL || ipv4_addr == NULL || 
        ipv4_remote_addr == NULL) {
        return (EINVAL);
    }

    if (daocfg->db == NULL && (err = dao_db_open(daocfg)) != 0) {
        return (err);
    }

    char *sql = "INSERT INTO vpn_clients (cn, ipv4_addr, ipv4_remote_addr, "
        "ipv6_addr, ipv6_remote_addr) "
        "VALUES (?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(daocfg->db, sql, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to execute statement: %s\n", 
            sqlite3_errmsg(daocfg->db));
        err = EIO;
        goto out_sql_finalize;
    }

    if (sqlite3_bind_text(stmt, 1, cn, strlen(cn), SQLITE_STATIC) 
        != SQLITE_OK || 
        sqlite3_bind_text(stmt, 2, ipv4_addr, strlen(ipv4_addr), SQLITE_STATIC) 
        != SQLITE_OK ||
        sqlite3_bind_text(stmt, 3, ipv4_remote_addr, strlen(ipv4_remote_addr), 
         SQLITE_STATIC) != SQLITE_OK) {
        fprintf(stderr, "Failed to bind value to statement: %s\n", 
            sqlite3_errmsg(daocfg->db));
        err = EINVAL;
        goto out_sql_finalize;
    }

    if (ipv6_addr != NULL) {
        if (sqlite3_bind_text(stmt, 4, ipv6_addr, strlen(ipv6_addr), 
            SQLITE_STATIC) != SQLITE_OK) {
            err = EINVAL;
            goto out_sql_finalize;
        }
    }
    else {
        if (sqlite3_bind_null(stmt, 4) != SQLITE_OK) {
            err = EINVAL;
            goto out_sql_finalize;
        }
    }

    if (ipv6_remote_addr != NULL) {
        if (sqlite3_bind_text(stmt, 5, ipv6_remote_addr, 
            strlen(ipv6_remote_addr), SQLITE_STATIC) != SQLITE_OK) {
            err = EINVAL;
            goto out_sql_finalize;
        }
    }
    else {
        if (sqlite3_bind_null(stmt, 5) != SQLITE_OK) {
            err = EINVAL;
            goto out_sql_finalize;
        }
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        fprintf(stderr, "Failed to step statement: %s\n", 
            sqlite3_errmsg(daocfg->db));
        err = EIO;
        goto out_sql_finalize;
    }

out_sql_finalize:
    sqlite3_finalize(stmt);
    return (err);
}

static void
i_dao_copy_str(char *dst, const char *src, size_t num)
{
    assert(dst != NULL);
    assert(src != NULL);

    strncpy(dst, src, num);
}

static void
i_dao_copy_nullable_str(char *dst, const char *src, size_t num)
{
    assert(dst != NULL);

    if (src != NULL) {
        strncpy(dst, src, num);
    }
}

/* 
 * dao_vpn_client_find_by_cn searches the SQLite database for a VPN client entry
 * with the given common name (cn). 
 */ 
int
dao_vpn_client_find_by_cn(dao_config_t *daocfg, const char *cn, 
                          struct vpn_client *model)
{
    sqlite3_stmt *stmt = NULL;
    int err = 0;

    if (daocfg == NULL || cn == NULL || model == NULL) {
        return (EINVAL);
    }

    /* Ensure that the SQLite database is open. On error exit. */
    if (daocfg->db == NULL && (err = dao_db_open(daocfg)) != 0) {
        return (err);
    }

    char *sql = 
        "SELECT ID, CN, IS_ACTIVE, IPV4_ADDR, IPV4_REMOTE_ADDR, IPV6_ADDR, "
        "    IPV6_REMOTE_ADDR "
        "FROM VPN_CLIENTS "
        "WHERE CN = ?";

    if (sqlite3_prepare_v2(daocfg->db, sql, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", 
            sqlite3_errmsg(daocfg->db));
        err = EIO;
        goto out_sql_finalize;
    }

    if (sqlite3_bind_text(stmt, 1, cn, strlen(cn), SQLITE_STATIC) != SQLITE_OK) {
        fprintf(stderr, "Failed to bind param: %s\n", 
            sqlite3_errmsg(daocfg->db));
        err = EIO;
        goto out_sql_finalize;
    }

    if (sqlite3_step(stmt) != SQLITE_ROW) {
        err = ENOENT;
        goto out_sql_finalize;
    }

    /* Zero model to receive a clean result. */
    memset(model, 0, sizeof(struct vpn_client));

    model->id = sqlite3_column_int(stmt, 0);
    i_dao_copy_str(model->cn, (const char *)sqlite3_column_text(stmt, 1), 
        RFC5280_CN_MAX_LENGTH);
    model->is_active = sqlite3_column_int(stmt, 2);
    i_dao_copy_str(model->ipv4_addr,
        (const char *)sqlite3_column_text(stmt, 3), INET_ADDRSTRLEN);
    i_dao_copy_str(model->ipv4_remote_addr, 
        (const char *)sqlite3_column_text(stmt, 4), INET_ADDRSTRLEN);
    i_dao_copy_nullable_str(model->ipv6_addr, 
        (const char *)sqlite3_column_text(stmt, 5), INET6_ADDRSTRLEN_W_PREFIX);
    i_dao_copy_nullable_str(model->ipv6_addr, 
        (const char *)sqlite3_column_text(stmt, 6), INET6_ADDRSTRLEN);

out_sql_finalize:
    sqlite3_finalize(stmt);
    return (err);
}

/* 
 * dao_vpn_client_network_find_by_client_id searches the SQLite database for all
 *  VPN client network entries regarding to the given client_id. 
 */ 
int
dao_vpn_client_network_find_by_client_id(dao_config_t *daocfg, int client_id, 
    vector_t *results)
{
    sqlite3_stmt *stmt = NULL;
    struct vpn_client_network row = {0};
    int err = 0;

    if (daocfg == NULL || results == NULL || client_id == 0) {
        return (EINVAL);
    }

    /* Ensure that the SQLite database is open. On error exit. */
    if (daocfg->db == NULL && (err = dao_db_open(daocfg)) != 0) {
        return (err);
    }

    char *sql = 
        "SELECT ID, CLIENT_ID, NETWORK_ADDR "
        "FROM VPN_CLIENT_NETWORKS "
        "WHERE CLIENT_ID = ?";

    if (sqlite3_prepare_v2(daocfg->db, sql, -1, &stmt, 0) != SQLITE_OK) {
        fprintf(stderr, "Failed to prepare statement: %s\n", 
            sqlite3_errmsg(daocfg->db));
        err = EIO;
        goto out_sql_finalize;
    }

    if (sqlite3_bind_int(stmt, 1, client_id) != SQLITE_OK) {
        fprintf(stderr, "Failed to bind param: %s\n", 
            sqlite3_errmsg(daocfg->db));
        err = EIO;
        goto out_sql_finalize;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        /* Zero model to receive a clean result. */
        memset(&row, 0, sizeof(struct vpn_client_network));

        row.id = sqlite3_column_int(stmt, 0);
        row.client_id = sqlite3_column_int(stmt, 1);
        i_dao_copy_str(row.network_addr, 
            (const char *)sqlite3_column_text(stmt, 2), 
            INET6_ADDRSTRLEN_W_PREFIX);

        vector_push_back(results, &row);
    }

out_sql_finalize:
    sqlite3_finalize(stmt);
    return (err);
}
