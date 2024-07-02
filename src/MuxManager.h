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
 * MuxManager.h
 *
 *  Created on: Oct 4, 2020
 *      Author: Tamer Ahmed
 */

#ifndef MUXMANAGER_H_
#define MUXMANAGER_H_

#include <map>
#include <memory>

#include <common/BoostAsioBehavior.h>
#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>

#include "MuxPort.h"
#include "common/MuxConfig.h"
#include "common/MuxPortConfig.h"
#include "DbInterface.h"

namespace test {
class MuxManagerTest;
}

namespace mux
{
using PortMap = std::map<std::string, std::shared_ptr<MuxPort>>;
using PortMapIterator = PortMap::iterator;

using PortCableTypeMap = std::map<std::string, common::MuxPortConfig::PortCableType>;
using PortCableTypeMapIterator = PortCableTypeMap::iterator;

const std::array<uint8_t, ETHER_ADDR_LEN> KNOWN_MAC_START = {0x04, 0x27, 0x28, 0x7a, 0x00, 0x00};
const size_t KNOWN_MAC_COUNT = 1024;

/**
 *@class MuxManager
 *
 *@brief host collection MuxPort object, each has MuxPort configuration, ActiveStandbyStateMachine.
 */
class MuxManager
{
public:
    /**
    *@method MuxManager
    *
    *@brief class default constructor
    */
    MuxManager();

    /**
    *@method MuxManager
    *
    *@brief class copy constructor
    *
    *@param MuxManager (in)  reference to MuxManager object to be copied
    */
    MuxManager(const MuxManager &) = delete;

    /**
    *@method ~MuxManager
    *
    *@brief class destructor
    */
    virtual ~MuxManager() = default;

public:
    /**
    *@method getIoService
    *
    *@brief getter for Boost IO Service/Context object
    *
    *@return reference to Boost IO Service/Context object
    */
    inline boost::asio::io_service& getIoService() {return mIoService;};

    /**
    *@method getDbInterface
    *
    *@brief getter for DbInterface object
    *
    *@return reference to DbInterface object
    */
    inline std::shared_ptr<mux::DbInterface> getDbInterfacePtr() {return mDbInterfacePtr;};

    /**
    *@method setTimeoutIpv4_msec
    *
    *@brief setter for IPv4 LinkProber timeout in msec
    *
    *@param timeout_msec (in)  timeout in msec
    *
    *@return none
    */
    inline void setTimeoutIpv4_msec(uint32_t timeout_msec) {mMuxConfig.setTimeoutIpv4_msec(timeout_msec);};

    /**
    *@method setTimeoutIpv6_msec
    *
    *@brief setter for IPv6 LinkProber timeout in msec
    *
    *@param timeout_msec (in)  timeout in msec
    *
    *@return none
    */
    inline void setTimeoutIpv6_msec(uint32_t timeout_msec) {mMuxConfig.setTimeoutIpv6_msec(timeout_msec);};

    /**
    *@method setOscillationEnabled
    *
    *@brief setter for enable/disable oscillation
    *
    *@param enable (in)  true to enable oscillation
    *
    *@return none
    */
    inline void setOscillationEnabled(bool enable) {mMuxConfig.setOscillationEnabled(enable);};

    /**
    *@method setOscillationInterval_sec
    *
    *@brief setter for oscillation interval in sec
    *
    *@param interval_sec (in)  interval in sec
    *
    *@return none
    */
    inline void setOscillationInterval_sec(uint32_t interval_sec) {mMuxConfig.setOscillationInterval_sec(interval_sec);};

    /**
    *@method setLinkProberStatUpdateIntervalCount
    *
    *@brief setter for link prober stats posting interval
    *
    *@param interval_count (in)  interval in heartbeat count
    *
    *@return none
    */
    inline void setLinkProberStatUpdateIntervalCount(uint32_t interval_count) {mMuxConfig.setLinkProberStatUpdateIntervalCount(interval_count);};

    /**
    *@method setPositiveStateChangeRetryCount
    *
    *@brief setter for LinkProber positive state change retry count
    *
    *@param stateChangeRetryCount (in)  state change retry count
    *
    *@return none
    */
    inline void setPositiveStateChangeRetryCount(uint32_t stateChangeRetryCount) {
        mMuxConfig.setPositiveStateChangeRetryCount(stateChangeRetryCount);
    };

    /**
    *@method setNegativeStateChangeRetryCount
    *
    *@brief setter for LinkProber negative state change retry count
    *
    *@param stateChangeRetryCount (in)  state change retry count
    *
    *@return none
    */
    inline void setNegativeStateChangeRetryCount(uint32_t stateChangeRetryCount) {
        mMuxConfig.setNegativeStateChangeRetryCount(stateChangeRetryCount);
    };

    /**
    *@method setSuspendTimeout_msec
    *
    *@brief setter for LinkProber suspend timer timeout
    *
    *@param suspendTimeout_msec (in)  suspend timer timeout
    *
    *@return none
    */
    inline void setSuspendTimeout_msec(uint32_t suspendTimeout_msec) {mMuxConfig.setSuspendTimeout_msec(suspendTimeout_msec);};

    /**
    *@method setSuspendTimeout_msec
    *
    *@brief setter for LinkProber suspend timer timeout
    *
    *@param suspendTimeout_msec (in)  suspend timer timeout
    *
    *@return none
    */
    inline void setTorMacAddress(std::array<uint8_t, ETHER_ADDR_LEN> &address) {mMuxConfig.setTorMacAddress(address);};

    /**
     * @method setVlanMacAddress
     * 
     * @brief setter for Vlan Mac Address
     * 
     * @param address (in) Vlan mac address 
     * 
     * @return none
     */
    inline void setVlanMacAddress(std::array<uint8_t, ETHER_ADDR_LEN> &address) {mMuxConfig.setVlanMacAddress(address);};

    /**
     * @method setIfUseTorMacAsSrcMac
     * 
     * @brief setter for flag whether use ToR MAC address as link prober src MAC 
     * 
     * @param useTorMac (in) bool
     * 
     * @return none
     */
    void setIfUseTorMacAsSrcMac(bool useTorMac) {mMuxConfig.setIfUseTorMacAsSrcMac(useTorMac);};

    /**
     * @method processSrcMac
     * 
     * @brief processs src mac config 
     * 
     * @param enable (in) bool 
     * 
     * @return none
     */
    void processSrcMac(bool useTorMac);

    /**
    *@method setSuspendTimeout_msec
    *
    *@brief setter for LinkProber suspend timer timeout
    *
    *@param suspendTimeout_msec (in)  suspend timer timeout
    *
    *@return none
    */
    inline void setLoopbackIpv4Address(boost::asio::ip::address& address) {mMuxConfig.setLoopbackIpv4Address(address);};


    /**
    *@method setUseWellKnownMacActiveActive
    *
    *@brief setter to set if use well known mac to probe for active-active ports
    *
    *@param useWellKnownMac (in)  true to use known mac to probe
    *
    *@return none
    */
    void setUseWellKnownMacActiveActive(bool useWellKnownMac);

    /**
     * @method getLoopbackIpv4Address
     * 
     * @brief getter for loop back ipv4 address 
     * 
     * @return IPv4 address
     */
    inline boost::asio::ip::address getLoopbackIpv4Address() {return mMuxConfig.getLoopbackIpv4Address();};

    /**
    *@method initialize
    *
    *@brief initialize MuxManager class and creates DbInterface instance that reads/listen from/to Redis db
    *
    * @param enable_feature_measurement (in) whether the feature that decreases link prober interval is enabled or not 
    * @param enable_feature_default_route (in) whether the feature that shutdowns link prober & avoid switching active when defaul route is missing, is enable or not
    * 
    * @return none
    */
    void initialize(bool enable_feature_measurement, bool enable_feature_default_route, bool enable_simulate_lfd_offload);

    /**
    *@method deinitialize
    *
    *@brief deinitialize MuxManager class and deinitialize DbInterface instance
    *
    *@return none
    */
    void deinitialize();

    /**
    *@method run
    *
    *@brief start Boost IO Service event loop
    *
    *@return none
    */
    void run();

    /**
    *@method terminate
    *
    *@brief stop and terminate Boost IO Service event loop
    *
    *@return none
    */
    void terminate();

    /**
    *@method updateLogVerbosity
    *
    *@brief update current log verbosity
    *
    *@param verbosity (in)   log verbosity
    *
    *@return none
    */
    void updateLogVerbosity(std::string &verbosity);

    /**
    *@method addOrUpdateMuxPort
    *
    *@brief update MUX port server/blade IPv4 Address. If port is not found, create new MuxPort object
    *
    *@param portName (in)   Mux port name
    *@param address (in)    server/blade IP address
    *
    *@return none
    */
    void addOrUpdateMuxPort(const std::string &portName, boost::asio::ip::address address);

    /**
    *@method addOrUpdateMuxPortSoCAddress
    *
    *@brief update MUX port SoC IPv4 Address. If port is not found, create new MuxPort object
    *
    *@param portName (in)   Mux port name
    *@param address (in)    SoC IP address
    *
    *@return none
    */
    void addOrUpdateMuxPortSoCAddress(const std::string &portName, boost::asio::ip::address address);

    /**
    *@method updateMuxPortConfig
    *
    *@brief update MUX port server/blade IPv4 Address. If port is not found, create new MuxPort object
    *
    *@param portName (in)   Mux port name
    *@param linkState (in)  Mux port link state
    *
    *@return none
    */
    void updateMuxPortConfig(const std::string &portName, const std::string &linkState);

   /**
    *@method updatePortCableType
    *
    *@brief update port cable type
    *
    *@param portName (in)   port name
    *@param cableType (in)  port cable type
    *
    *@return none
    */
    void updatePortCableType(const std::string &portName, const std::string &cableType);

    /**
     * @method resetPckLossCount
     * 
     * @brief reset ICMP packet loss count. 
     * 
     * @param portName (in) Mux port name 
     * 
     * @return none
    */
    void resetPckLossCount(const std::string &portName);

    /**
    *@method addOrUpdateMuxPortLinkState
    *
    *@brief update MUX port link state. If port is not found, create new MuxPort object
    *
    *@param portName (in)   Mux port name
    *@param linkState (in)  Mux port link state
    *
    *@return none
    */
    void addOrUpdateMuxPortLinkState(const std::string &portName, const std::string &linkState);

    /**
     * @method addOrUpdatePeerLinkState
     * 
     * @brief update mux port's peer link state. If port is not found, create new MuxPort object.
     * 
     * @param portName (in) Mux port name
     * @param linkState (in) Peer's link state
     * 
     * @return none
    */
    void addOrUpdatePeerLinkState(const std::string &portName, const std::string &linkState);

    /**
    *@method addOrUpdateMuxPortMuxState
    *
    *@brief update MUX port state db notification
    *
    *@param portName (in)   Mux port name
    *@param muxState (in)   Mux port state
    *
    *@return none
    */
    void addOrUpdateMuxPortMuxState(const std::string &portName, const std::string &muxState);

    /**
    *@method processGetServerMacAddress
    *
    *@brief update MUX port server MAC address
    *
    *@param portName (in)   Mux port name
    *@param address (in)    Server MAC address
    *
    *@return none
    */
    void processGetServerMacAddress(const std::string &portName, const std::array<uint8_t, ETHER_ADDR_LEN> &address);

    /**
    *@method processGetMuxState
    *
    *@brief update MUX port app db notification
    *
    *@param portName (in)   Mux port name
    *@param muxState (in)   Mux port state
    *
    *@return none
    */
    void processGetMuxState(const std::string &portName, const std::string &muxState);

    /**
    *@method processProbeMuxState
    *
    *@brief update MUX port app db notification
    *
    *@param portName (in)   Mux port name
    *@param muxState (in)   Mux port state
    *
    *@return none
    */
    void processProbeMuxState(const std::string &portName, const std::string &muxState);

    /**
    *@method processPeerMuxState
    *
    *@brief update peer MUX port state db notification
    *
    *@param portName        (in)   Mux port name
    *@param peerMuxState    (in)   Peer mux port state
    *
    *@return none
    */
    void processPeerMuxState(const std::string &portName, const std::string &peerMuxState);

    /**
     * @method addOrUpdateDefaultRouteState
     * 
     * @brief update default route state based on state db notification
     * 
     * @param ipAddress
     * @param routeState
     *  
     * @return none
     * 
    */
    void addOrUpdateDefaultRouteState(bool is_v4, const std::string &routeState);

    /**
     * @method updateWarmRestartReconciliationCount
     * 
     * @brief update warm restart reconciliation count
     * 
     * @param increment
     * 
     * @return none
     */
    void updateWarmRestartReconciliationCount(int increment);

    /**
     * @method handleTsaEnableNotification
     * 
     * @brief handle TSA Enable Notification
     * 
     * @return none
     */
    void handleTsaEnableNotification(bool enable);

    /**
    *@method handleLinkProberSessionStateNotification
    *
    *@brief handle the link prober session state notification from ASIC DB
    *
    *@param sessionId (in)   link prober session id
    *@param sessionState (in)   link prober session state
    *
    *@return reference to the mux config
    */
    void handleLinkProberSessionStateNotification(const std::string &sessionId, const std::string &sessionState);

    /**
    *@method getMuxConfig
    *
    *@brief getter for the mux config
    *
    *@return reference to the mux config
    */
    common::MuxConfig &getMuxConfig() {return mMuxConfig;};

private:
    /**
    *@method getMuxPortCableType
    *
    *@brief retrieve the mux port cable type for port
    *
    *@param portName (in)   Mux port name
    *
    *@return port cable type
    */
    inline common::MuxPortConfig::PortCableType getMuxPortCableType(const std::string &portName);

    /**
    *@method getMuxPortPtr
    *
    *@brief get the mux port that the link prober session belongs to
    *
    *@param sessionId (in)   link prober session id
    *
    *@return pointer to the mux port
    */
    inline std::shared_ptr<MuxPort> getMuxPortPtr(const std::string &sessionId);

    /**
    *@method getMuxPortPtrOrThrow
    *
    *@brief retrieve a pointer to MuxPort if it exist or create a new MuxPort object
    *
    *@param portName (in)   Mux port name
    *
    *@return pointer to MuxPort object
    */
    std::shared_ptr<MuxPort> getMuxPortPtrOrThrow(const std::string &portName);

    /**
    *@method handleSignal
    *
    *@brief handles system signal
    *
    *@param errorCode (in)      Boost error code
    *@param signalNumber (in)   Signal number
    *
    *@return none
    */
    void handleSignal(const boost::system::error_code errorCode, int signalNumber);

    /**
    *@method handleProcessTerminate
    *
    *@brief stop DB interface thread and stop boost io service
    *
    *@return none
    */
    void handleProcessTerminate();

    /**
    *@method generateServerMac
    *
    *@brief generate Server MAC for port in active-active cable type
    *
    *@return none
    */
    void generateServerMac(uint16_t serverId, std::array<uint8_t, ETHER_ADDR_LEN> &address);

private:
    friend class test::MuxManagerTest;
    /**
    *@method setDbInterfacePtr
    *
    *@brief set DbInterface to an external instance and is solely used for unit tests
    *
    *@return none
    */
    void setDbInterfacePtr(std::shared_ptr<mux::DbInterface> dbInterfacePtr) {mDbInterfacePtr = dbInterfacePtr;};

private: 
    /**
     * @method startWarmRestartReconciliationTimer
     * 
     * @brief start warm restart reconciliation timer
     * 
     * @return none
     */
    void startWarmRestartReconciliationTimer(uint32_t timeout=0);

    /**
     * @method handleWarmRestartReconciliationTimeout
     * 
     * @brief handle warm restart reconciliationTimeout
     *
     * @param errorCode (in) Boost error code 
     *  
     * @return none
     */
    void handleWarmRestartReconciliationTimeout(const boost::system::error_code errorCode);

    /**
     * @method handleUpdateReconciliationCount
     * 
     * @brief handler of updating reconciliation port count 
     * 
     * @param increment
     * 
     * @return none
     */
    void handleUpdateReconciliationCount(int increment);

private:
    common::MuxConfig mMuxConfig;

    boost::asio::io_service mIoService;
    boost::asio::io_service::work mWork;
    boost::thread_group mThreadGroup;
    boost::asio::signal_set mSignalSet;

    boost::asio::io_service::strand mStrand;
    boost::asio::deadline_timer mReconciliationTimer;
    uint16_t mPortReconciliationCount = 0;

    std::shared_ptr<mux::DbInterface> mDbInterfacePtr;

    PortMap mPortMap;
    PortCableTypeMap mPortCableTypeMap;

    std::string mIpv4DefaultRouteState = "na";
    std::string mIpv6DefaultRouteState = "na";
};

} /* namespace mux */

#endif /* MUXMANAGER_H_ */
