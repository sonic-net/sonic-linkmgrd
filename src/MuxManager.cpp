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
    mDbInterfacePtr(std::make_shared<mux::DbInterface> (this, &mIoService))
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
// ---> initialize();
//
// initialize MuxManager class and creates DbInterface instance that reads/listen from/to Redis db
//
void MuxManager::initialize()
{
    for (uint8_t i = 0; (mMuxConfig.getNumberOfThreads() > 2) &&
                        (i < mMuxConfig.getNumberOfThreads() - 2); i++) {
        mThreadGroup.create_thread(
            boost::bind(&boost::asio::io_service::run, &mIoService)
        );
    }

    mDbInterfacePtr->initialize();
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

    if (address.is_v4()) {
        muxPortPtr->handleBladeIpv4AddressUpdate(address);
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
// ---> addOrUpdateMuxPortLinkState(const std::string &portName, const std::string &linkState);
//
// update MUX port server/blade IPv4 Address. If port is not found, create new MuxPort object
//
void MuxManager::addOrUpdateMuxPortLinkState(const std::string &portName, const std::string &linkState)
{
    MUXLOGWARNING(boost::format("%s: link state: %s") % portName % linkState);

    std::shared_ptr<MuxPort> muxPortPtr = getMuxPortPtrOrThrow(portName);
    muxPortPtr->handleLinkState(linkState);
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
// ---> getMuxPortPtrOrThrow(const std::string &portName);
//
// retrieve a pointer to MuxPort if it exist or create a new MuxPort object
//
std::shared_ptr<MuxPort> MuxManager::getMuxPortPtrOrThrow(const std::string &portName)
{
    std::shared_ptr<MuxPort> muxPortPtr;

    try {
        PortMapIterator portMapIterator = mPortMap.find(portName);
        if (portMapIterator == mPortMap.end()) {
            uint16_t serverId = atoi(portName.substr(portName.find_last_not_of("0123456789") + 1).c_str());
            muxPortPtr = std::make_shared<MuxPort> (
                mDbInterfacePtr,
                mMuxConfig,
                portName,
                serverId,
                mIoService
            );
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

} /* namespace mux */
