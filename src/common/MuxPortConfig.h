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
 * MuxPortConfig.h
 *
 *  Created on: Oct 21, 2020
 *      Author: tamer
 */

#ifndef MUXPORTCONFIG_H_
#define MUXPORTCONFIG_H_

#include <string>
#include <stdint.h>
#include <net/ethernet.h>

#include "MuxConfig.h"

namespace common
{

/**
 *@class MuxPortConfig
 *
 *@brief Holds MUX port configuration
 */
class MuxPortConfig
{
public:
    /**
     *@enum Mode
     *
     *@brief MUX config mode
     */
    enum Mode {
        Auto,
        Manual,
        Active,
        Standby
    };

public:
    /**
    *@method MuxPortConfig
    *
    *@brief class default constructor
    */
    MuxPortConfig() = delete;

    /**
    *@method MuxPortConfig
    *
    *@brief class copy constructor
    *
    *@param MuxPortConfig (in)  reference to MuxPortConfig object to be copied
    */
    MuxPortConfig(const MuxPortConfig &) = delete;

    /**
    *@method MuxPortConfig
    *
    *@brief class constructor
    *
    *@param muxConfig (in)  reference to MuxConfig object
    *@param portname (in)   reference to port name
    *@param serverId (in)   server Id
    */
    MuxPortConfig(
        MuxConfig &muxConfig,
        const std::string &portName,
        uint16_t serverId
    );

    /**
    *@method ~MuxPortConfig
    *
    *@brief class destructor
    */
    virtual ~MuxPortConfig() = default;

    /**
    *@method setBladeIpv4Address
    *
    *@brief setter for server/blade IPv4 address
    *
    *@param address (in) server IPv4 address
    *
    *@return none
    */
    inline void setBladeIpv4Address(const boost::asio::ip::address &address) {mBladeIpv4Address = address;};

    /**
    *@method setBladeMacAddress
    *
    *@brief setter for server/blade MAC address
    *
    *@param address (in) server MAC address
    *
    *@return none
    */
    inline void setBladeMacAddress(const std::array<uint8_t, ETHER_ADDR_LEN> &address) {mBladeMacAddress = address;};

    /**
    *@method setMode
    *
    *@brief setter for MUX mode
    *
    *@param mode (in)   MUX config mode
    *
    *@return none
    */
    inline void setMode(const Mode mode) {mMode = mode;};

    /**
    *@method getTimeoutIpv4_msec
    *
    *@brief getter for IPv4 LinkProber timeout in msec
    *
    *@return timeout in msec
    */
    inline uint32_t getTimeoutIpv4_msec() const {return mMuxConfig.getTimeoutIpv4_msec();};

    /**
    *@method getTimeoutIpv6_msec
    *
    *@brief getter for IPv6 LinkProber timeout in msec
    *
    *@return timeout in msec
    */
    inline uint32_t getTimeoutIpv6_msec() const {return mMuxConfig.getTimeoutIpv6_msec();};

    /**
    *@method getPositiveStateChangeRetryCount
    *
    *@brief getter for LinkProber positive state change retry count
    *
    *@return state change retry count
    */
    inline uint32_t getPositiveStateChangeRetryCount() const {return mMuxConfig.getPositiveStateChangeRetryCount();};

    /**
    *@method getNegativeStateChangeRetryCount
    *
    *@brief getter for LinkProber negative state change retry count
    *
    *@return state change retry count
    */
    inline uint32_t getNegativeStateChangeRetryCount() const {return mMuxConfig.getNegativeStateChangeRetryCount();};

    /**
    *@method getSuspendTimeout_msec
    *
    *@brief getter for LinkProber suspend timer timeout
    *
    *@return suspend timer timeout
    */
    inline uint32_t getLinkWaitTimeout_msec() const {return mMuxConfig.getSuspendTimeout_msec();};

    /**
    *@method getMuxStateChangeRetryCount
    *
    *@brief getter for MuxState state change retry count
    *
    *@return state change retry count
    */
    inline uint32_t getMuxStateChangeRetryCount() const {return mMuxConfig.getMuxStateChangeRetryCount();};

    /**
    *@method getLinkStateChangeRetryCount
    *
    *@brief getter for LinkeState change retry count
    *
    *@return state change retry count
    */
    inline uint32_t getLinkStateChangeRetryCount() const {return mMuxConfig.getLinkStateChangeRetryCount();};

    /**
    *@method getTorMacAddress
    *
    *@brief getter for ToR MAC address
    *
    *@return ToR MAC address
    */
    inline const std::array<uint8_t, ETHER_ADDR_LEN>& getTorMacAddress() const {return mMuxConfig.getTorMacAddress();};

    /**
    *@method getLoopbackIpv4Address
    *
    *@brief getter for Loopback IPv4 address
    *
    *@return IPv4 address
    */
    inline boost::asio::ip::address getLoopbackIpv4Address() const {return mMuxConfig.getLoopbackIpv4Address();};

    /**
    *@method getPortName
    *
    *@brief getter for port name
    *
    *@return port name
    */
    inline const std::string& getPortName() const {return mPortName;};

    /**
    *@method getBladeIpv4Address
    *
    *@brief getter for server/blade IPv4 address
    *
    *@return IPv4 address
    */
    inline const boost::asio::ip::address& getBladeIpv4Address() const {return mBladeIpv4Address;};

    /**
    *@method getBladeMacAddress
    *
    *@brief getter for server/blade MAC address
    *
    *@return MAC address
    */
    inline const std::array<uint8_t, ETHER_ADDR_LEN>& getBladeMacAddress() const {return mBladeMacAddress;};

    /**
    *@method getServerId
    *
    *@brief getter for server id
    *
    *@return server id
    */
    inline uint16_t getServerId() const {return mServerId;};

    /**
    *@method getMode
    *
    *@brief getter for MUX mode
    *
    *@return MUX mode
    */
    inline Mode getMode() const {return mMode;};

private:
    MuxConfig &mMuxConfig;
    std::string mPortName;
    boost::asio::ip::address mBladeIpv4Address;
    std::array<uint8_t, ETHER_ADDR_LEN> mBladeMacAddress = {0, 0, 0, 0, 0, 0};
    uint16_t mServerId;
    Mode mMode = Manual;
};

} /* namespace common */

#endif /* MUXPORTCONFIG_H_ */
