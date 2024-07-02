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
        Standby,
        Detached      // mux mode for active-active cable type only 
    };

    /**
     * @enum PortCableType
     * 
     * @brief Port cable type
     */
    enum PortCableType {
        ActiveStandby,
        ActiveActive,
        DefaultType = ActiveStandby
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
        uint16_t serverId,
        PortCableType portCableType
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
    *@method getLinkProberStatUpdateIntervalCount
    *
    *@brief getter for LinkProber negative state change retry count
    *
    *@return state change retry count
    */
    inline uint32_t getLinkProberStatUpdateIntervalCount() const {return mMuxConfig.getLinkProberStatUpdateIntervalCount();};

    /**
    *@method getSuspendTimeout_msec
    *
    *@brief getter for LinkProber suspend timer timeout
    *
    *@return suspend timer timeout
    */
    inline uint32_t getLinkWaitTimeout_msec() const {return mMuxConfig.getSuspendTimeout_msec();};

    /**
    *@method getIfOscillationEnabled
    *
    *@brief getter for mux state oscillation enable flag
    *
    *@return oscillation enable flag
    */
    inline bool getIfOscillationEnabled() const {return mMuxConfig.getIfOscillationEnabled();};

    /**
    *@method getOscillationInterval_sec
    *
    *@brief getter for mux state oscillation interval
    *
    *@return oscillation interval
    */
    inline uint32_t getOscillationInterval_sec() const {return mMuxConfig.getOscillationInterval_sec();};

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
     * @method getVlanMacAddress
     * 
     * @brief getter for Vlan MAC address
     * 
     * @return Vlan MAC address
     */
    inline const std::array<uint8_t, ETHER_ADDR_LEN>& getVlanMacAddress() const {return mMuxConfig.getVlanMacAddress();};

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

    /**
    *@method getPortCableType
    *
    *@brief getter for port cable type
    *
    *@return port cable type
    */
    inline PortCableType getPortCableType() const {return mPortCableType;};

    /**
    *@method getDecreasedTimeoutIpv4_msec
    *
    *@brief getter for decreased IPv4 LinkProber timeout in msec
    *
    *@return timeout in msec
    */
    inline uint32_t getDecreasedTimeoutIpv4_msec() const {return mMuxConfig.getDecreasedTimeoutIpv4_msec();};

    /**
     * @method setPortCableType
     * 
     * @brief Set the Port Cable Type object
     * 
     * @param portCableType         port cable type
     * 
     * @return none
     */
    inline void setPortCableType(PortCableType portCableType) { mPortCableType = portCableType; };

    /**
     * @method ifEnableSwitchoverMeasurement
     * 
     * @brief check if the feature that decreases link prober interval to measure switch overhead is enabled or not 
     * 
     * @return if switch overhead measurement feature is enabled
     */
    inline bool ifEnableSwitchoverMeasurement() {return mMuxConfig.getIfEnableSwitchoverMeasurement();};

    /**
     * @method ifEnableDefaultRouteFeature
     * 
     * @brief check if the default route related feature is enabled or not 
     * 
     * @return if the feature is enabled
     */
    inline bool ifEnableDefaultRouteFeature() {return mMuxConfig.getIfEnableDefaultRouteFeature();};

    /**
     * @method getIfUseWellKnownMacActiveActive
     * 
     * @brief check if use well known mac to probe for active-active ports
     * 
     * @return true to use well known mac to probe for active-active ports
     */
    inline bool getIfUseWellKnownMacActiveActive() { return mMuxConfig.getIfUseWellKnownMacActiveActive(); }

    /**
    *@method getWellKnownMacAddress
    *
    *@brief getter for server well known MAC address
    *
    *@return MAC address
    */
    inline const std::array<uint8_t, ETHER_ADDR_LEN>& getWellKnownMacAddress() const {return mWellKnownMacAddress;};

    /**
    *@method setWellKnownMacAddress
    *
    *@brief setter for server well known MAC address
    *
    *@param address (in) server well known MAC address
    *
    *@return none
    */
    inline void setWellKnownMacAddress(const std::array<uint8_t, ETHER_ADDR_LEN> &address) {mWellKnownMacAddress = address;};

    /**
    *@method getLastUpdatedMacAddress
    *
    *@brief getter for last server MAC address update
    *
    *@return MAC address
    */
    inline const std::array<uint8_t, ETHER_ADDR_LEN>& getLastUpdatedMacAddress() const {return mLastUpdatedMacAddress;};

    /**
    *@method setLastUpdatedMacAddress
    *
    *@brief setter for last server MAC address update
    *
    *@param address (in) server known MAC address
    *
    *@return none
    */
    inline void setLastUpdatedMacAddress(const std::array<uint8_t, ETHER_ADDR_LEN> &address) {mLastUpdatedMacAddress = address;};

    /**
     * @method ifEnableUseTorMac
     * 
     * @brief check if use ToR MAC address as src MAC for link prober 
     * 
     * @reutrn if use ToR MAC
     */
    inline bool ifEnableUseTorMac() {return mMuxConfig.getIfEnableUseTorMac();};

    /**
    *@method getIfEnableSimulateLfdOffload
    *
    *@brief getter for enable simulate LFD offload flag
    *
    *@return enable simulate LFD offload flag
    */
    inline bool getIfEnableSimulateLfdOffload() { return mMuxConfig.getIfEnableSimulateLfdOffload(); }

    /**
     * @method getAdminForwardingStateSyncUpInterval
     * 
     * @brief getter of admin forwarding state sync up interval
     * 
     * @return sync up interval in msec
     */
    uint32_t getAdminForwardingStateSyncUpInterval() {return mAdminForwardingStateSyncUpInterval_msec;};

private:
    MuxConfig &mMuxConfig;
    std::string mPortName;
    boost::asio::ip::address mBladeIpv4Address;
    std::array<uint8_t, ETHER_ADDR_LEN> mBladeMacAddress = {0, 0, 0, 0, 0, 0};
    std::array<uint8_t, ETHER_ADDR_LEN> mWellKnownMacAddress = {0, 0, 0, 0, 0, 0};
    std::array<uint8_t, ETHER_ADDR_LEN> mLastUpdatedMacAddress = {0, 0, 0, 0, 0, 0};
    uint16_t mServerId;
    Mode mMode = Manual;
    PortCableType mPortCableType;
    uint32_t mAdminForwardingStateSyncUpInterval_msec = 10000;

};

} /* namespace common */

#endif /* MUXPORTCONFIG_H_ */
