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

#include <boost/asio.hpp>
#include <boost/asio/signal_set.hpp>

#include "MuxPort.h"
#include "common/MuxConfig.h"
#include "DbInterface.h"

namespace test {
class MuxManagerTest;
}

namespace mux
{
using PortMap = std::map<std::string, std::shared_ptr<MuxPort>>;
using PortMapIterator = PortMap::iterator;

/**
 *@class MuxManager
 *
 *@brief host collection MuxPort object, each has MuxPort configuration, LinkManagerStateMachine.
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
    *@method initialize
    *
    *@brief initialize MuxManager class and creates DbInterface instance that reads/listen from/to Redis db
    *
    * @param enable_feature_measurement (in) whether the feature that decreases link prober interval is enabled or not 
    * 
    * @return none
    */
    void initialize(bool enable_feature_measurement);

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

private:
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
    common::MuxConfig mMuxConfig;

    boost::asio::io_service mIoService;
    boost::asio::io_service::work mWork;
    boost::thread_group mThreadGroup;
    boost::asio::signal_set mSignalSet;

    std::shared_ptr<mux::DbInterface> mDbInterfacePtr;

    PortMap mPortMap;

    std::string mIpv4DefaultRouteState = "na";
    std::string mIpv6DefaultRouteState = "na";
};

} /* namespace mux */

#endif /* MUXMANAGER_H_ */
