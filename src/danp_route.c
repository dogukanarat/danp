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
#include "danp_internal_types.h"

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
static osal_mutex_handle_t route_mutex = NULL;

/* Functions */

int32_t danp_route_init(void)
{
    if (route_mutex != NULL)
    {
        return 0;
    }

    osal_mutex_attr_t attr = {
        .name = "danpRouteLock",
        .attr_bits = OSAL_MUTEX_PRIO_INHERIT,
        .cb_mem = NULL,
        .cb_size = 0,
    };

    route_mutex = osal_mutex_create(&attr);
    if (route_mutex == NULL)
    {
        DANP_LOG_ERR("Failed to create routing mutex");
        return -1;
    }

    return 0;
}

static bool danp_route_lock(void)
{
    if (route_mutex == NULL)
    {
        /* Must be initialized first */
        return false;
    }

    if (osal_mutex_lock(route_mutex, OSAL_WAIT_FOREVER) != OSAL_SUCCESS)
    {
        return false;
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

static danp_interface_t *danp_route_lookup(uint16_t dest_node_id)
{
    danp_interface_t *iface = NULL;
    bool locked = danp_route_lock();

    if (!locked)
    {
        return NULL;
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

void danp_start_interfaces(void)
{
    danp_interface_t *cur = iface_list;
    bool locked = danp_route_lock();

    if (!locked)
    {
        return;
    }

    while (cur)
    {
        cur->data.is_running = true;
        cur = cur->next;
    }

    danp_route_unlock(locked);
}

void danp_stop_interfaces(void)
{
    danp_interface_t *cur = iface_list;
    bool locked = danp_route_lock();

    if (!locked)
    {
        return;
    }

    while (cur)
    {
        cur->data.is_running = false;
        cur = cur->next;
    }

    danp_route_unlock(locked);
}

void danp_shutdown_interfaces(void)
{
    danp_interface_t *cur = iface_list;
    bool locked = danp_route_lock();

    if (!locked)
    {
        return;
    }

    while (cur)
    {
        cur->data.is_exiting = true;
        cur = cur->next;
    }

    danp_route_unlock(locked);
}

void danp_register_interface(danp_interface_t *iface)
{
    bool locked = false;
    if (!iface)
    {
        DANP_LOG_ERR("Cannot register NULL interface");
        return;
    }
    if (!iface->ops.tx)
    {
        DANP_LOG_ERR("Interface tx_func is NULL, cannot register");
        return;
    }
    if (!iface->name)
    {
        DANP_LOG_ERR("Interface name is NULL, cannot register");
        return;
    }
    if (iface->mtu == 0)
    {
        DANP_LOG_ERR("Interface MTU is zero, cannot register");
        return;
    }
    locked = danp_route_lock();
    if (!locked)
    {
        return;
    }

    if (!iface_list)
    {
        DANP_LOG_INF("Registering first network interface: %s", iface->name);
    }
    else
    {
        DANP_LOG_INF("Registering network interface: %s", iface->name);
    }
    iface->next = iface_list;
    iface_list = iface;
    DANP_LOG_VER("Registered network interface");

    danp_route_unlock(locked);
}

int32_t danp_route_table_load(const char *table)
{
    bool locked = false;

    if (!table)
    {
        DANP_LOG_ERR("Routing table string is NULL");
        return -1;
    }

    locked = danp_route_lock();
    if (!locked)
    {
        return -1;
    }

    const size_t len = strlen(table);
    if (len == 0U)
    {
        route_count = 0U;
        danp_route_unlock(locked);
        return 0;
    }

    // Use a fixed size buffer on stack instead of malloc
    #define ROUTE_BUFFER_SIZE 256
    if (len >= ROUTE_BUFFER_SIZE) {
        DANP_LOG_ERR("Routing table too large for static buffer");
        danp_route_unlock(locked);
        return -1;
    }

    char buffer[ROUTE_BUFFER_SIZE];
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
            DANP_LOG_ERR("Invalid route entry '%s' (missing ':')", working);
            route_count = 0U;
            danp_route_unlock(locked);
            return -1;
        }

        *separator = '\0';
        char *dest_str = danp_trim(working);
        char *iface_str = danp_trim(separator + 1);

        if (*dest_str == '\0' || *iface_str == '\0')
        {
            DANP_LOG_ERR("Invalid route entry '%s'", entry);
            route_count = 0U;
            danp_route_unlock(locked);
            return -1;
        }

        char *endptr = NULL;
        unsigned long dest_val = strtoul(dest_str, &endptr, 0);
        if (*endptr != '\0' || dest_val > UINT16_MAX)
        {
            DANP_LOG_ERR("Invalid destination node '%s'", dest_str);
            route_count = 0U;
            danp_route_unlock(locked);
            return -1;
        }

        if (route_count >= (sizeof(route_table) / sizeof(route_table[0])))
        {
            DANP_LOG_ERR("Routing table full, cannot add destination %lu", dest_val);
            route_count = 0U;
            danp_route_unlock(locked);
            return -1;
        }

        danp_interface_t *iface = danp_find_interface_by_name(iface_str);
        if (!iface)
        {
            DANP_LOG_ERR("Interface '%s' not registered for destination %lu", iface_str, dest_val);
            route_count = 0U;
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

    danp_route_unlock(locked);
    return 0;
}

int32_t danp_route_tx(danp_packet_t *pkt)
{
    if (!pkt)
    {
        DANP_LOG_ERR("NULL packet passed to router");
        return -1;
    }

    uint16_t dst, src;
    uint8_t dst_port, src_port, flags;
    danp_unpack_header(pkt->header_raw, &dst, &src, &dst_port, &src_port, &flags);

    danp_interface_t *out = danp_route_lookup(dst);
    if (!out)
    {
        DANP_LOG_ERR("No route to destination %u", dst);
        return -1;
    }
    if (out->ops.tx == NULL)
    {
        DANP_LOG_ERR("No TX function for interface %s", out->name);
        return -1;
    }

    if ((uint32_t)pkt->length + DANP_HEADER_SIZE > out->mtu)
    {
        DANP_LOG_ERR("Packet length %u exceeds MTU %u for interface %s", pkt->length + DANP_HEADER_SIZE, out->mtu, out->name);
        return -1;
    }

    DANP_LOG_DBG(
        "Output: [dst]=%u [src]=%u [dPort]=%u [sPort]=%u [flags]=0x%02X [len]=%u [iface]=%s",
        dst,
        src,
        dst_port,
        src_port,
        flags,
        pkt->length,
        out->name);

    return out->ops.tx(out, pkt);
}
