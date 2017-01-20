/* mbed Microcontroller Library
 * Copyright (c) 2016 ARM Limited
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#if DEVICE_EMAC

#include "emac_api.h"
#include "emac_stack_mem.h"
#include "lwip/tcpip.h"
#include "lwip/tcp.h"
#include "lwip/ip.h"
#include "netif/etharp.h"

static err_t emac_lwip_low_level_output(struct netif *netif, struct pbuf *p)
{
    emac_interface_t *mac = (emac_interface_t *)netif->state;
    bool ret = mac->ops->link_out(mac, (emac_stack_mem_chain_t *)p);
    return ret ? ERR_OK : ERR_IF;
}

static void emac_lwip_input(void *data, emac_stack_mem_t *buf)
{
    struct pbuf *p = (struct pbuf *)buf;
    struct netif *netif = (struct netif *)data;

    /* pass all packets to ethernet_input, which decides what packets it supports */
    if (netif->input(p, netif) != ERR_OK) {
        LWIP_DEBUGF(NETIF_DEBUG, ("Emac LWIP: IP input error\n"));

        pbuf_free(p);
    }
}

static void emac_lwip_state_change(void *data, bool up)
{
    struct netif *netif = (struct netif *)data;

    if (up) {
        tcpip_callback_with_block((tcpip_callback_fn)netif_set_link_up, netif, 1);
    } else {
        tcpip_callback_with_block((tcpip_callback_fn)netif_set_link_down, netif, 1);
    }
}

#if LWIP_IGMP

#include "lwip/igmp.h"
/**
 * IPv4 address filtering setup.
 *
 * \param[in] netif the lwip network interface structure
 * \param[in] group IPv4 group to modify
 * \param[in] action
 * \return ERR_OK or error code
 */
err_t igmp_mac_filter(struct netif *netif, const ip4_addr_t *group, u8_t action)
{
    emac_interface_t *emac = netif->state;
    if (emac->ops->add_multicast_group == NULL) {
        return ERR_OK; /* This function is not mandatory */
    }

    switch (action) {
        case IGMP_ADD_MAC_FILTER:
        {
            uint32_t group23 = ntohl(group->addr) & 0x007FFFFF;
            uint8_t addr[6];
            addr[0] = LL_IP4_MULTICAST_ADDR_0;
            addr[1] = LL_IP4_MULTICAST_ADDR_1;
            addr[2] = LL_IP4_MULTICAST_ADDR_2;
            addr[3] = group23 >> 16;
            addr[4] = group23 >> 8;
            addr[5] = group23;
            emac->ops->add_multicast_group(emac, addr);
            return ERR_OK;
        }
        case IGMP_DEL_MAC_FILTER:
            /* As we don't reference count, silently ignore delete requests */
            return ERR_OK;
        default:
            return ERR_ARG;
    }
}
#endif

#if LWIP_IPV6_MLD

#include "lwip/mld6.h"
/**
 * IPv6 address filtering setup.
 *
 * \param[in] netif the lwip network interface structure
 * \param[in] group IPv6 group to modify
 * \param[in] action
 * \return ERR_OK or error code
 */
err_t mld_mac_filter(struct netif *netif, const ip6_addr_t *group, u8_t action)
{
    emac_interface_t *emac = netif->state;
    if (emac->ops->add_multicast_group == NULL) {
        return ERR_OK; /* This function is not mandatory */
    }

    switch (action) {
        case MLD6_ADD_MAC_FILTER:
        {
            uint32_t group32 = ntohl(group->addr[3]);
            uint8_t addr[6];
            addr[0] = LL_IP6_MULTICAST_ADDR_0;
            addr[1] = LL_IP6_MULTICAST_ADDR_1;
            addr[2] = group32 >> 24;
            addr[3] = group32 >> 16;
            addr[4] = group32 >> 8;
            addr[5] = group32;
            emac->ops->add_multicast_group(emac, addr);
            return ERR_OK;
        }
        case MLD6_DEL_MAC_FILTER:
            /* As we don't reference count, silently ignore delete requests */
            return ERR_OK;
        default:
            return ERR_ARG;
    }
}
#endif

err_t emac_lwip_if_init(struct netif *netif)
{
    int err = ERR_OK;
    emac_interface_t *emac = netif->state;

    emac->ops->set_link_input_cb(emac, emac_lwip_input, netif);
    emac->ops->set_link_state_cb(emac, emac_lwip_state_change, netif);

    netif->hwaddr_len = emac->ops->get_hwaddr_size(emac);
    emac->ops->get_hwaddr(emac, netif->hwaddr);
    netif->mtu = emac->ops->get_mtu_size(emac);

    /* Interface capabilities */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

    emac->ops->get_ifname(emac, netif->name, 2);

#if LWIP_IPV4
    netif->output = etharp_output;
#if LWIP_IGMP
    netif->igmp_mac_filter = igmp_mac_filter;
    netif->flags |= NETIF_FLAG_IGMP;
#endif /* LWIP_IGMP */
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#if LWIP_IPV6_MLD
    netif->mld_mac_filter = mld_mac_filter;
    netif->flags |= NETIF_FLAG_MLD6;
#else
// Would need to enable all multicasts here - no API in fsl_enet to do that
#error "IPv6 multicasts won't be received if LWIP_IPV6_MLD is disabled, breaking the system"
#endif
#endif

    netif->linkoutput = emac_lwip_low_level_output;

    if (!emac->ops->power_up(emac)) {
        err = ERR_IF;
    }

    return err;
}

#endif /* DEVICE_EMAC */
