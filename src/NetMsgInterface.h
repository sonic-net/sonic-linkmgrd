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
 * NetMsgInterface.h
 *
 *  Created on: Jan 12, 2021
 *      Author: Tamer Ahmed
 */

#ifndef _NETMSGINTERFACE_H_
#define _NETMSGINTERFACE_H_

#include "swss/netmsg.h"
#include "DbInterface.h"

#define MAX_ADDR_SIZE   64

namespace test {
class MuxManagerTest;
}

namespace mux
{
using NetlinkObject = struct nl_object;
using RouteNetlinkNeighbor = struct rtnl_neigh;
//using RouteNetlinkLink = struct rtnl_link;

class NetMsgInterface: public swss::NetMsg
{
public:
    /**
    *@method NetMsgInterface
    *
    *@brief class default constructor
    */
    NetMsgInterface() = delete;

    /**
    *@method NetMsgInterface
    *
    *@brief class copy constructor
    *
    *@param DbInterface (in)  reference to DbInterface object to be copied
    */
    NetMsgInterface(const NetMsgInterface &) = delete;

    /**
    *@method NetMsgInterface
    *
    *@brief class constructor
    *
    *@param dbInterface (in)    reference to DB interface instance
    */
    NetMsgInterface(DbInterface &dbInterface);

    /**
    *@method ~NetMsgInterface
    *
    *@brief class destructor
    */
    virtual ~NetMsgInterface() = default;

    /**
    *@method onMsg
    *
    *@brief handle received net link messages
    *
    *@param msgType (in)        netlink message type
    *@param netlinkObject (in)  pointer to netlink message object
    *
    *@return none
    */
    virtual void onMsg(int msgType, NetlinkObject *netlinkObject) override;

private:
    friend class test::MuxManagerTest;

    /**
    *@method updateMacAddress
    *
    *@brief update server MAC address
    *
    *@param port (in)   port name server is connected to
    *@param ip (in)     server IP address
    *@param mac (in)    server MAC address
    *
    *@return none
    */
    inline void updateMacAddress(
        std::string &port,
        std::array<char, MAX_ADDR_SIZE + 1> &ip,
        std::array<char, MAX_ADDR_SIZE + 1> &mac
    );

private:
    DbInterface &mDbInterface;
};

} /* namespace mux */

#endif /* _NETMSGINTERFACE_H_ */
