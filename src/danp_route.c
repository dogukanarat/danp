/* danp_route.c - static routing implementation */

/* All Rights Reserved */

/* Includes */

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "osal/osal_mutex.h"

#include "danp/danp.h"
#include "danp_debug.h"

/* Types */

typedef struct danp_route_entry_s
{
    uint16_t dest_node;          /**< Destination node address. */
    danp_interface_t *iface;     /**< Outgoing interface for the destination. */
} danp_route_entry_t;

/* Variables */

/** @brief List of registered network interfaces. */
static danp_interface_t *iface_list = NULL;

/** @brief Static routing table. */
static danp_route_entry_t route_table[DANP_MAX_NODES];

/** @brief Number of active entries in the routing table. */
static size_t route_count = 0U;

/** @brief Mutex protecting routing state and interface list. */
static osal_mutex_handle_t route_mutex;

/**
 * @brief Lazily create and lock the routing mutex.
 * @return true if the mutex is locked, false otherwise.
 */
static bool danp_route_lock(void)
{
    if (route_mutex == NULL)
    {
        osal_mutex_attr_t attr = {
            .name = "danpRouteLock",
            .attr_bits = OSAL_MUTEX_PRIO_INHERIT,
            .cb_mem = NULL,
            .cb_size = 0,
        };

        route_mutex = osal_mutex_create(&attr);
        if (route_mutex == NULL)
        {
            /* LCOV_EXCL_START */
            danp_log_message(DANP_LOG_ERROR, "Failed to create routing mutex");
            return false;
            /* LCOV_EXCL_STOP */
        }
    }

    if (osal_mutex_lock(route_mutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS)
    {
        /* LCOV_EXCL_START */
        return false;
        /* LCOV_EXCL_STOP */
    }

    return true;
}

static void danp_route_unlock(bool is_locked)
{
    if (is_locked)
    {
        osal_mutex_unlock(route_mutex);
    }
}

/* Functions */

/**
 * @brief Trim leading and trailing whitespace from a string in-place.
 * @param str String to trim.
 * @return Pointer to the trimmed string (may be the original pointer).
 */
static char *danp_trim(char *str)
{
    while (str && *str && isspace((unsigned char)*str))
    {
        str++;
    }

    if (!str || *str == '\0')
    {
        return str;
    }

    char *end = str + strlen(str);
    while (end > str && isspace((unsigned char)*(end - 1)))
    {
        end--;
    }
    *end = '\0';

    return str;
}

/**
 * @brief Find an interface by its name.
 * @param name Name of the interface.
 * @return Pointer to the interface or NULL if not found.
 */
static danp_interface_t *danp_find_interface_by_name(const char *name)
{
    danp_interface_t *cur = iface_list;
    size_t guard = 0U;

    while (cur)
    {
        if (guard++ > DANP_MAX_NODES)
        {
            break; // Prevent infinite loop on corrupted list
        }
        if (cur->name && strcmp(cur->name, name) == 0)
        {
            return cur;
        }
        cur = cur->next;
    }

    return NULL;
}

/**
 * @brief Lookup the route for a destination node.
 * @param dest_node_id Destination node ID.
 * @return Pointer to the interface to use, or NULL if no route found.
 */
static danp_interface_t *danp_route_lookup(uint16_t dest_node_id)
{
    danp_interface_t *iface = NULL;
    bool locked = danp_route_lock();

    if (!locked)
    {
        /* LCOV_EXCL_START */
        return NULL;
        /* LCOV_EXCL_STOP */
    }

    for (size_t i = 0; i < route_count; i++)
    {
        if (route_table[i].dest_node == dest_node_id)
        {
            iface = route_table[i].iface;
            break;
        }
    }

    danp_route_unlock(locked);
    return iface;
}

/**
 * @brief Register a network interface.
 * @param iface Pointer to the interface to register.
 */
void danp_register_interface(void *iface)
{
    danp_interface_t *iface_common = iface;
    bool locked = false;
    if (!iface_common)
    {
        danp_log_message(DANP_LOG_ERROR, "Cannot register NULL interface");
        return;
    }
    if (!iface_common->tx_func)
    {
        danp_log_message(DANP_LOG_ERROR, "Interface tx_func is NULL, cannot register");
        return;
    }
    if (!iface_common->name)
    {
        danp_log_message(DANP_LOG_ERROR, "Interface name is NULL, cannot register");
        return;
    }
    if (iface_common->mtu == 0)
    {
        danp_log_message(DANP_LOG_ERROR, "Interface MTU is zero, cannot register");
        return;
    }
    locked = danp_route_lock();
    if (!locked)
    {
        /* LCOV_EXCL_START */
        return;
        /* LCOV_EXCL_STOP */
    }

    if (!iface_list)
    {
        danp_log_message(DANP_LOG_INFO, "Registering first network interface: %s", iface_common->name);
    }
    else
    {
        danp_log_message(DANP_LOG_INFO, "Registering network interface: %s", iface_common->name);
    }
    iface_common->next = iface_list;
    iface_list = iface;
    danp_log_message(DANP_LOG_VERBOSE, "Registered network interface");

    danp_route_unlock(locked);
}

/**
 * @brief Parse and install a static routing table from a string.
 *
 * The table uses comma or newline separated entries with the format
 * "<destination_node>:<interface_name>". Whitespace around tokens is ignored.
 *
 * Example: "1:if0, 42:backbone\n100:radio".
 *
 * @param table Routing table string.
 * @return 0 on success, negative on error.
 */
int32_t danp_route_table_load(const char *table)
{
    bool locked = false;

    if (!table)
    {
        danp_log_message(DANP_LOG_ERROR, "Routing table string is NULL");
        return -1;
    }

    locked = danp_route_lock();
    if (!locked)
    {
        /* LCOV_EXCL_START */
        return -1;
        /* LCOV_EXCL_STOP */
    }

    const size_t len = strlen(table);
    if (len == 0U)
    {
        route_count = 0U;
        danp_route_unlock(locked);
        return 0;
    }

    char *buffer = (char *)malloc(len + 1U);
    if (!buffer)
    {
        danp_log_message(DANP_LOG_ERROR, "Failed to allocate buffer for routing table");
        danp_route_unlock(locked);
        return -1;
    }
    memcpy(buffer, table, len + 1U);

    char *saveptr = NULL;
    char *entry = strtok_r(buffer, "\n,", &saveptr);
    route_count = 0U;

    while (entry)
    {
        char *working = danp_trim(entry);
        if (*working == '\0')
        {
            entry = strtok_r(NULL, "\n,", &saveptr);
            continue;
        }

        char *separator = strchr(working, ':');
        if (!separator)
        {
            danp_log_message(DANP_LOG_ERROR, "Invalid route entry '%s' (missing ':')", working);
            route_count = 0U;
            free(buffer);
            danp_route_unlock(locked);
            return -1;
        }

        *separator = '\0';
        char *dest_str = danp_trim(working);
        char *iface_str = danp_trim(separator + 1);

        if (*dest_str == '\0' || *iface_str == '\0')
        {
            danp_log_message(DANP_LOG_ERROR, "Invalid route entry '%s'", entry);
            route_count = 0U;
            free(buffer);
            danp_route_unlock(locked);
            return -1;
        }

        char *endptr = NULL;
        unsigned long dest_val = strtoul(dest_str, &endptr, 0);
        if (*endptr != '\0' || dest_val > UINT16_MAX)
        {
            danp_log_message(DANP_LOG_ERROR, "Invalid destination node '%s'", dest_str);
            route_count = 0U;
            free(buffer);
            danp_route_unlock(locked);
            return -1;
        }

        if (route_count >= (sizeof(route_table) / sizeof(route_table[0])))
        {
            danp_log_message(DANP_LOG_ERROR, "Routing table full, cannot add destination %lu", dest_val);
            route_count = 0U;
            free(buffer);
            danp_route_unlock(locked);
            return -1;
        }

        danp_interface_t *iface = danp_find_interface_by_name(iface_str);
        if (!iface)
        {
            danp_log_message(DANP_LOG_ERROR, "Interface '%s' not registered for destination %lu", iface_str, dest_val);
            route_count = 0U;
            free(buffer);
            danp_route_unlock(locked);
            return -1;
        }

        // Replace existing entry if present
        bool replaced = false;
        for (size_t i = 0; i < route_count; i++)
        {
            if (route_table[i].dest_node == (uint16_t)dest_val)
            {
                route_table[i].iface = iface;
                replaced = true;
                break;
            }
        }

        if (!replaced)
        {
            route_table[route_count].dest_node = (uint16_t)dest_val;
            route_table[route_count].iface = iface;
            route_count++;
        }

        entry = strtok_r(NULL, "\n,", &saveptr);
    }

    free(buffer);
    danp_route_unlock(locked);
    return 0;
}

/**
 * @brief Route a packet for transmission.
 * @param pkt Pointer to the packet to route.
 * @return 0 on success, negative on error.
 */
int32_t danp_route_tx(danp_packet_t *pkt)
{
    if (!pkt)
    {
        danp_log_message(DANP_LOG_ERROR, "NULL packet passed to router");
        return -1;
    }

    uint16_t dst, src;
    uint8_t dst_port, src_port, flags;
    danp_unpack_header(pkt->header_raw, &dst, &src, &dst_port, &src_port, &flags);

    danp_interface_t *out = danp_route_lookup(dst);
    if (!out)
    {
        danp_log_message(DANP_LOG_ERROR, "No route to destination %u", dst);
        return -1;
    }

    if ((uint32_t)pkt->length + DANP_HEADER_SIZE > out->mtu)
    {
        danp_log_message(DANP_LOG_ERROR, "Packet length %u exceeds MTU %u for interface %s", pkt->length + DANP_HEADER_SIZE, out->mtu, out->name);
        return -1;
    }

    danp_log_message(
        DANP_LOG_DEBUG,
        "TX [dst]=%u, [src]=%u, [dPort]=%u, [sPort]=%u, [flags]=0x%02X, [len]=%u, [iface]=%s",
        dst,
        src,
        dst_port,
        src_port,
        flags,
        pkt->length,
        out->name);
    return out->tx_func(out, pkt);
}
