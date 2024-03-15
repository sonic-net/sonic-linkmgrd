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
 * MuxManager.cpp
 *
 *  Created on: Oct 4, 2020
 *      Author: Tamer Ahmed
 */

#include <ctype.h>
#include <iostream>
#include <string>
#include <net/ethernet.h>

#include <boost/bind/bind.hpp>

#include "common/MuxException.h"
#include "common/MuxLogger.h"
#include "MuxManager.h"

namespace mux
{
//
// ---> MuxManager();
//
// class constructor
//
MuxManager::MuxManager() :
    mMuxConfig(),
    mWork(mIoService),
    mSignalSet(boost::asio::signal_set(mIoService, SIGINT, SIGTERM)),
    mDbInterfacePtr(std::make_shared<mux::DbInterface> (this, &mIoService)),
    mStrand(mIoService),
    mReconciliationTimer(mIoService)
{
    mSignalSet.add(SIGUSR1);
    mSignalSet.add(SIGUSR2);
    mSignalSet.async_wait(boost::bind(&MuxManager::handleSignal,
        this,
        boost::asio::placeholders::error,
        boost::asio::placeholders::signal_number
    ));
}

//
// ---> setUseWellKnownMacActiveActive(bool UseWellKnownMac);
//
//  set to use known mac for all ports in active-active cable type
//
void MuxManager::setUseWellKnownMacActiveActive(bool useWellKnownMac)
{
    mMuxConfig.setUseWellKnownMacActiveActive(useWellKnownMac);

    for (const auto & [portName, muxPort] : mPortMap) {
        muxPort->handleUseWellKnownMacAddress();
    }
}


//
// ---> initialize();
//
// initialize MuxManager class and creates DbInterface instance that reads/listen from/to Redis db
//
void MuxManager::initialize(bool enable_feature_measurement, bool enable_feature_default_route, bool enable_simulate_lfd_offload)
{
    for (uint8_t i = 0; (mMuxConfig.getNumberOfThreads() > 2) &&
                        (i < mMuxConfig.getNumberOfThreads() - 2); i++) {
        mThreadGroup.create_thread(
            boost::bind(&boost::asio::io_service::run, &mIoService)
        );
    }

    mMuxConfig.enableSimulateLfdOffload(enable_simulate_lfd_offload);

    mDbInterfacePtr->initialize();

    if (mDbInterfacePtr->isWarmStart()) {
        MUXLOGINFO("Detected warm restart context, starting reconciliation timer.");
        startWarmRestartReconciliationTimer(mDbInterfacePtr->getWarmStartTimer());
    }

    mMuxConfig.enableSwitchoverMeasurement(enable_feature_measurement);
    mMuxConfig.enableDefaultRouteFeature(enable_feature_default_route);
}

//
// ---> deinitialize();
//
// deinitialize MuxManager class and deinitialize DbInterface instance
//
void MuxManager::deinitialize()
{
    mDbInterfacePtr->deinitialize();
}

//
// ---> run();
//
// start Boost IO Service event loop
//
void MuxManager::run()
{
    mIoService.run();
}

//
// ---> terminate();
//
// stop and terminate Boost IO Service event loop
//
void MuxManager::terminate()
{
    mIoService.stop();
    mThreadGroup.join_all();
}

//
// ---> updateLogVerbosity(std::string &verbosity);
//
// update current log verbosity
//
void MuxManager::updateLogVerbosity(std::string &verbosity)
{
    boost::log::trivial::severity_level level = boost::log::trivial::warning;

    if (verbosity == "trace") {
        level = boost::log::trivial::trace;
    } else if (verbosity == "debug") {
        level = boost::log::trivial::debug;
    } else if (verbosity == "info") {
        level = boost::log::trivial::info;
    } else if (verbosity == "error") {
        level = boost::log::trivial::error;
    } else if (verbosity == "fatal") {
        level = boost::log::trivial::fatal;
    }

    common::MuxLogger::getInstance()->setLevel(level);
    MUXLOGFATAL(boost::format("Updated log level to: %s") % level);
}

//
// ---> addOrUpdateMuxPort(const std::string &portName, boost::asio::ip::address address);
//
// update MUX port server/blade IPv4 Address. If port is not found, create new MuxPort object
//
void MuxManager::addOrUpdateMuxPort(const std::string &portName, boost::asio::ip::address address)
{
    MUXLOGWARNING(boost::format("%s: server IP: %s") % portName % address);

    std::shared_ptr<MuxPort> muxPortPtr = getMuxPortPtrOrThrow(portName);
    common::MuxPortConfig::PortCableType portCableType = getMuxPortCableType(portName);

    if (address.is_v4()) {
        if (portCableType == common::MuxPortConfig::PortCableType::ActiveStandby) {
            // notify server IP address for ports in active-standby cable type
            muxPortPtr->handleBladeIpv4AddressUpdate(address);
        }

    } else if (address.is_v6()) {
        // handle IPv6 probing
    }
}

//
// ---> addOrUpdateMuxPortSoCAddress(const std::string &portName, boost::asio::ip::address address);
//
// update MUX port SoC IPv4 Address. If port is not found, create new MuxPort object
//
void MuxManager::addOrUpdateMuxPortSoCAddress(const std::string &portName, boost::asio::ip::address address)
{
    MUXLOGWARNING(boost::format("%s: SoC IP: %s") % portName % address);

    std::shared_ptr<MuxPort> muxPortPtr = getMuxPortPtrOrThrow(portName);
    common::MuxPortConfig::PortCableType portCableType = getMuxPortCableType(portName);

    if (address.is_v4()) {
        if (portCableType == common::MuxPortConfig::PortCableType::ActiveActive) {
            // notify NiC IP address for ports in active-active cable type
            muxPortPtr->handleSoCIpv4AddressUpdate(address);
        }
    } else if (address.is_v6()) {
        // handle IPv6 probing
    }
}

//
// ---> updateMuxPortConfig(const std::string &portName, const std::string &config);
//
// update MUX port server/blade IPv4 Address. If port is not found, create new MuxPort object
//
void MuxManager::updateMuxPortConfig(const std::string &portName, const std::string &config)
{
    MUXLOGWARNING(boost::format("%s: Mux port config: %s") % portName % config);

    PortMapIterator portMapIterator = mPortMap.find(portName);
    if (portMapIterator != mPortMap.end()) {
        portMapIterator->second->handleMuxConfig(config);
    }
}

//
// ---> updatePortCableType(const std::string &portName, const std::string &cableType);
//
// update port cable type
//
void MuxManager::updatePortCableType(const std::string &portName, const std::string &cableType)
{
    MUXLOGWARNING(boost::format("%s: Port cable type: %s") % portName % cableType);

    common::MuxPortConfig::PortCableType portCableType;
    if (cableType == "active-standby") {
        portCableType = common::MuxPortConfig::PortCableType::ActiveStandby;
    } else if (cableType == "active-active") {
        portCableType = common::MuxPortConfig::PortCableType::ActiveActive;
    } else {
        MUXLOGERROR(
            boost::format(
                "%s: Received unsupported port cable type %s, fall back to active-standby!"
            ) % portName % cableType
        );
        portCableType = common::MuxPortConfig::PortCableType::ActiveStandby;
    }
    mPortCableTypeMap.insert(PortCableTypeMap::value_type(portName, portCableType));
}

// 
// ---> resetPckLossCount(const std::string &portName);
//
// reset ICMP packet loss count 
// 
void MuxManager::resetPckLossCount(const std::string &portName)
{
    MUXLOGWARNING(boost::format("%s: reset ICMP packet loss count ") % portName);

    PortMapIterator portMapIterator = mPortMap.find(portName);
    if(portMapIterator != mPortMap.end()) {
        portMapIterator->second->resetPckLossCount();
    }
}

//
// ---> addOrUpdateMuxPortLinkState(const std::string &portName, const std::string &linkState);
//
// update MUX port link state. If port is not found, create new MuxPort object
//
void MuxManager::addOrUpdateMuxPortLinkState(const std::string &portName, const std::string &linkState)
{
    MUXLOGWARNING(boost::format("%s: link state: %s") % portName % linkState);

    std::shared_ptr<MuxPort> muxPortPtr = getMuxPortPtrOrThrow(portName);
    muxPortPtr->handleLinkState(linkState);
}

// 
// ---> addOrUpdatePeerLinkState(const std::string &portName, const std::string &linkState);
// 
// update mux port's peer link state. If port is not found, create new MuxPort object.
//
void MuxManager::addOrUpdatePeerLinkState(const std::string &portName, const std::string &linkState)
{
    MUXLOGDEBUG(boost::format("%s: peer link state %s") % portName % linkState);

    std::shared_ptr<MuxPort> muxPortPtr = getMuxPortPtrOrThrow(portName);
    muxPortPtr->handlePeerLinkState(linkState);
}

//
// ---> addOrUpdateMuxPortMuxState(const std::string &portName, const std::string &muxState);
//
// update MUX port state db notification
//
void MuxManager::addOrUpdateMuxPortMuxState(const std::string &portName, const std::string &muxState)
{
    MUXLOGWARNING(boost::format("%s: state db mux state: %s") % portName % muxState);

    std::shared_ptr<MuxPort> muxPortPtr = getMuxPortPtrOrThrow(portName);
    muxPortPtr->handleMuxState(muxState);
}

//
// ---> processGetServerMacAddress(const std::string &portName, const std::array<uint8_t, ETHER_ADDR_LEN> &address);
//
// update MUX port server MAC address
//
void MuxManager::processGetServerMacAddress(
    const std::string &portName,
    const std::array<uint8_t, ETHER_ADDR_LEN> &address
)
{
    MUXLOGDEBUG(portName);

    PortMapIterator portMapIterator = mPortMap.find(portName);
    if (portMapIterator != mPortMap.end()) {
        portMapIterator->second->handleGetServerMacAddress(address);
    }
}

//
// ---> processSrcMac(bool useTorMac);
//
// processs src mac config 
//
void MuxManager::processSrcMac(bool useTorMac)
{
    if (mMuxConfig.getIfEnableUseTorMac() != useTorMac) {
        setIfUseTorMacAsSrcMac(useTorMac);

        PortMapIterator portMapIterator = mPortMap.begin();
        while (portMapIterator != mPortMap.end()) {
            portMapIterator->second->handleSrcMacAddressUpdate();
            portMapIterator ++;
        }
    }
}


//
// ---> processGetMuxState(const std::string &portName, const std::string &muxState);
//
// update MUX port state db notification
//
void MuxManager::processGetMuxState(const std::string &portName, const std::string &muxState)
{
    MUXLOGDEBUG(boost::format("%s: state db mux state: %s") % portName % muxState);

    PortMapIterator portMapIterator = mPortMap.find(portName);
    if (portMapIterator != mPortMap.end()) {
        portMapIterator->second->handleGetMuxState(muxState);
    }
}

//
// ---> processProbeMuxState(const std::string &portName, const std::string &muxState);
//
// update MUX port app db notification
//
void MuxManager::processProbeMuxState(const std::string &portName, const std::string &muxState)
{
    MUXLOGINFO(boost::format("%s: app db mux state: %s") % portName % muxState);

    PortMapIterator portMapIterator = mPortMap.find(portName);
    if (portMapIterator != mPortMap.end()) {
        portMapIterator->second->handleProbeMuxState(muxState);
    }
}

//
// ---> processPeerMuxState(const std::string &portName, const std::string &peerMuxState);
//
// update peer MUX port state db notification
//
void MuxManager::processPeerMuxState(const std::string &portName, const std::string &peerMuxState)
{
    MUXLOGINFO(boost::format("%s: state db peer mux state: %s") % portName % peerMuxState);

    PortMapIterator portMapIterator = mPortMap.find(portName);
    if (portMapIterator != mPortMap.end()) {
        portMapIterator->second->handlePeerMuxState(peerMuxState);
    }
}

//
// ---> addOrUpdateDefaultRouteState(boost::asio::ip::address& address, const std::string &routeState);
//
// update default route state based on state db notification
//
void MuxManager::addOrUpdateDefaultRouteState(bool is_v4, const std::string &routeState) 
{
    if (is_v4) {
        mIpv4DefaultRouteState = routeState;
    } else {
        mIpv6DefaultRouteState = routeState;
    }

    std::string nextState = "na";
    // For now we only need IPv4 default route state to be "ok". If we switch to IPv6 in the furture, this will cause an issue. 
    if (mIpv4DefaultRouteState == "ok") {
        nextState = "ok";
    }

    MUXLOGINFO(boost::format("Default route state: %s") % nextState);

    PortMapIterator portMapIterator = mPortMap.begin();
    while (portMapIterator != mPortMap.end()) {
        portMapIterator->second->handleDefaultRouteState(nextState);
        portMapIterator ++;
    }
}

//
// ---> getMuxPortCableType(const std::string &portName);
//
// retrieve the mux port cable type for port
//
inline common::MuxPortConfig::PortCableType MuxManager::getMuxPortCableType(const std::string &portName)
{
    PortCableTypeMapIterator portCableTypeIter = mPortCableTypeMap.find(portName);
    if (portCableTypeIter == mPortCableTypeMap.end()) {
        mPortCableTypeMap.insert({portName, common::MuxPortConfig::PortCableType::DefaultType});
    }
    return mPortCableTypeMap[portName];
}

//
// ---> getMuxPortPtrOrThrow(const std::string &portName);
//
// retrieve a pointer to MuxPort if it exist or create a new MuxPort object
//
std::shared_ptr<MuxPort> MuxManager::getMuxPortPtrOrThrow(const std::string &portName)
{
    std::shared_ptr<MuxPort> muxPortPtr;
    common::MuxPortConfig::PortCableType muxPortCableType = getMuxPortCableType(portName);

    try {
        PortMapIterator portMapIterator = mPortMap.find(portName);
        if (portMapIterator == mPortMap.end()) {
            uint16_t serverId = atoi(portName.substr(portName.find_last_not_of("0123456789") + 1).c_str());
            muxPortPtr = std::make_shared<MuxPort> (
                mDbInterfacePtr,
                mMuxConfig,
                portName,
                serverId,
                mIoService,
                muxPortCableType
            );
            if (muxPortCableType == common::MuxPortConfig::PortCableType::ActiveActive) {
                std::array<uint8_t, ETHER_ADDR_LEN> address;
                generateServerMac(serverId, address);
                muxPortPtr->setWellKnownMacAddress(address);
                if (mMuxConfig.getIfUseWellKnownMacActiveActive()) {
                    muxPortPtr->setServerMacAddress(address);
                }
            }
            if (mMuxConfig.getIfEnableSimulateLfdOffload()) {
                // register the dispatch table
            }
            mPortMap.insert({portName, muxPortPtr});
        }
        else {
            muxPortPtr = portMapIterator->second;
        }
    }
    catch (const std::bad_alloc &ex) {
        std::ostringstream errMsg;
        errMsg << "Failed allocate memory. Exception details: " << ex.what();

        throw MUX_ERROR(BadAlloc, errMsg.str());
    }

    return muxPortPtr;
}

//
// ---> handleSignal(const boost::system::error_code errorCode, int signalNumber)'
//
// handles system signal
//
void MuxManager::handleSignal(const boost::system::error_code errorCode, int signalNumber)
{
    MUXLOGFATAL(boost::format("Got signal: %d") % signalNumber);

    if (signalNumber == SIGINT || signalNumber == SIGTERM) {
        mSignalSet.clear();
        handleProcessTerminate();
    } else {
        mSignalSet.async_wait(boost::bind(&MuxManager::handleSignal,
            this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::signal_number
        ));
    }
}

//
// ---> handleProcessTerminate();
//
// stop DB interface thread and stop boost io service
//
void MuxManager::handleProcessTerminate()
{
    mDbInterfacePtr->stopSwssNotificationPoll();
    mDbInterfacePtr->getBarrier().wait();
    mIoService.stop();
    mDbInterfacePtr->getBarrier().wait();
}

//
// ---> generateServerMac();
//
// generate known MAC address based on server ID
//
void MuxManager::generateServerMac(uint16_t serverId, std::array<uint8_t, ETHER_ADDR_LEN> &address)
{
    if (serverId >= KNOWN_MAC_COUNT) {
        throw std::range_error("Out of MAC address range");
    }
    int addrIndex = ETHER_ADDR_LEN - 1;
    uint32_t offset = KNOWN_MAC_START[addrIndex] + serverId;
    address = KNOWN_MAC_START;
    while (offset && addrIndex >= 0) {
        address[addrIndex--] = offset % 0xff;
        offset /= 0xff;
    }
}

// ---> updateWarmRestartReconciliationCount(int increment);
//
// update warm restart reconciliation count
//
void MuxManager::updateWarmRestartReconciliationCount(int increment)
{
    MUXLOGDEBUG(increment);

    boost::asio::post(mStrand,boost::bind(
        &MuxManager::handleUpdateReconciliationCount,
        this,
        increment
    ));
}

// ---> handleUpdateReconciliationCount(int increment);
//
// handler of updating reconciliation port count
// 
void MuxManager::handleUpdateReconciliationCount(int increment)
{
    MUXLOGDEBUG(mPortReconciliationCount);

    mPortReconciliationCount += increment;

    if(mPortReconciliationCount == 0) {
        mReconciliationTimer.cancel();
    } 
}

// ---> startWarmRestartReconciliationTimer
//
// start warm restart reconciliation timer
// 
void MuxManager::startWarmRestartReconciliationTimer(uint32_t timeout)
{
    mReconciliationTimer.expires_from_now(boost::posix_time::seconds(
        timeout == 0? mMuxConfig.getMuxReconciliationTimeout_sec():timeout
    ));
    mReconciliationTimer.async_wait(boost::bind(
        &MuxManager::handleWarmRestartReconciliationTimeout,
        this,
        boost::asio::placeholders::error
    ));
}

// ---> handleWarmRestartReconciliationTimeout
// 
// handle warm restart reconciliationTimeout
//
void MuxManager::handleWarmRestartReconciliationTimeout(const boost::system::error_code errorCode)
{
    if (errorCode == boost::system::errc::success) {
        MUXLOGWARNING("Reconciliation timed out after warm restart, set service to reconciled now.");
    }

    std::map<std::string, std::string> muxModeMap = mDbInterfacePtr->getMuxModeConfig();
    for (auto it = muxModeMap.begin(); it != muxModeMap.end(); ++it) {
        if (it->second == "auto") {
            continue;
        }

        MUXLOGWARNING(boost::format("config mux mode back to auto for %s") % it->first);
        mDbInterfacePtr->setMuxMode(it->first, "auto");
    }

    mDbInterfacePtr->setWarmStartStateReconciled();
}

//
// ---> handleTsaEnableNotification
//
// handle TSA Enable Notification
//
void MuxManager::handleTsaEnableNotification(bool enable)
{
    PortMapIterator portMapIterator = mPortMap.begin();

    while (portMapIterator != mPortMap.end()) {
        portMapIterator->second->handleTsaEnable(enable);
        portMapIterator ++;
    }
}

//
// ---> getMuxPortPtr(const std::string &sessionId)
//
// get the mux port that the link prober session belongs to
//
inline std::shared_ptr<MuxPort> MuxManager::getMuxPortPtr(const std::string &sessionId)
{
    auto pos = sessionId.find('|');
    if (pos != std::string::npos) {
        std::string portName = sessionId.substr(0, pos);

        PortMapIterator portMapIterator = mPortMap.find(portName);
        if (portMapIterator != mPortMap.end()) {
            return portMapIterator->second;
        }
    }
    return nullptr;
}

//
// ---> handleLinkProberSessionStateNotification(const std::string &sessionId, const std::string &sessionState)
//
// handle the link prober session state notification from ASIC DB
//
void MuxManager::handleLinkProberSessionStateNotification(const std::string &sessionId, const std::string &sessionState)
{
    auto muxPortPtr = getMuxPortPtr(sessionId);
    if (muxPortPtr) {
        muxPortPtr->handleLinkProberSessionStateNotification(sessionId, sessionState);
    }
}

} /* namespace mux */
