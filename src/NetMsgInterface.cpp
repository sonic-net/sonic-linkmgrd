/*
 *  Copyright 2021 (c) Microsoft Corporation.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

/*
 * NetMsgInterface.cpp
 *
 *  Created on: Jan 12, 2021
 *      Author: Tamer Ahmed
 */

#include <array>
#include <stdexcept>
#include <string>
#include <netinet/in.h>
#include <netlink/route/link.h>
#include <netlink/route/neighbour.h>

#include "swss/linkcache.h"
#include "swss/macaddress.h"

#include "NetMsgInterface.h"
#include "common/MuxLogger.h"
#include "common/MuxException.h"

namespace mux
{

//
// ---> NetMsgInterface(DbInterface &dbInterface);
//
// class constructor
//
NetMsgInterface::NetMsgInterface(DbInterface &dbInterface) :
    mDbInterface(dbInterface)
{
}

//
// ---> onMsg(int msgType, NetlinkObject *netlinkObject);
//
// handle received net link messages
//
void NetMsgInterface::onMsg(int msgType, NetlinkObject *netlinkObject)
{
    static auto constexpr NONE_MAC = "none";

    if ((msgType == RTM_NEWNEIGH) || (msgType == RTM_GETNEIGH) || (msgType == RTM_DELNEIGH)) {
        RouteNetlinkNeighbor *routeNetlinkNeighbor = reinterpret_cast<RouteNetlinkNeighbor *> (netlinkObject);
        if (rtnl_neigh_get_family(routeNetlinkNeighbor) == AF_INET ||
            rtnl_neigh_get_family(routeNetlinkNeighbor) == AF_INET6) {
            std::string portName = swss::LinkCache::getInstance().ifindexToName(
                rtnl_neigh_get_ifindex(routeNetlinkNeighbor)
            );

            std::array<char, MAX_ADDR_SIZE + 1> macStr;
            nl_addr2str(rtnl_neigh_get_lladdr(routeNetlinkNeighbor), macStr.data(), macStr.size() - 1);

            if (strncmp(macStr.data(), NONE_MAC, strlen(NONE_MAC))) {
                std::array<char, MAX_ADDR_SIZE + 1> ipStr;
                nl_addr2str(rtnl_neigh_get_dst(routeNetlinkNeighbor), ipStr.data(), ipStr.size() - 1);

                updateMacAddress(portName, ipStr, macStr);
            }
        }
    }
}

//
// ---> updateMacAddress(
//          std::string &port,
//          std::array<char, MAX_ADDR_SIZE + 1> &ip,
//          std::array<char, MAX_ADDR_SIZE + 1> &mac
//      );
//
// update server MAC address
//
void NetMsgInterface::updateMacAddress(
    std::string &port,
    std::array<char, MAX_ADDR_SIZE + 1> &ip,
    std::array<char, MAX_ADDR_SIZE + 1> &mac
)
{
    MUXLOGDEBUG(boost::format("%s: interface IP '%s', MAC '%s'") %
        port % ip.data() % mac.data()
    );

    boost::system::error_code errorCode;
    boost::asio::ip::address ipAddress = boost::asio::ip::make_address(ip.data(), errorCode);
    if (!errorCode) {
        try {
            swss::MacAddress macAddress(mac.data());
            mDbInterface.updateServerMacAddress(ipAddress, macAddress.getMac());
        }
        catch (const std::invalid_argument &invalidArgument) {
            MUXLOGWARNING(boost::format("%s: invalid argument for interface IP '%s', MAC '%s'") %
                port % ip.data() % mac.data()
            );
        }
    }
}

} /* namespace mux */
