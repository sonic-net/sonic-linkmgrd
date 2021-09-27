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
 * MuxPort.cpp
 *
 *  Created on: Oct 7, 2020
 *      Author: tamer
 */

#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <netinet/ether.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <netpacket/packet.h>

#include <boost/bind/bind.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/mersenne_twister.hpp>

#include "MuxPort.h"
#include "common/MuxLogger.h"

namespace mux
{
//
// ---> MuxPort(
//          mux::DbInterface *dbInterface,
//          common::MuxConfig &muxConfig,
//          const std::string &portName,
//          uint16_t serverId,
//          boost::asio::io_service &ioService
//      );
//
// class constructor
//
MuxPort::MuxPort(
    std::shared_ptr<mux::DbInterface> dbInterfacePtr,
    common::MuxConfig &muxConfig,
    const std::string &portName,
    uint16_t serverId,
    boost::asio::io_service &ioService
) :
    mDbInterfacePtr(dbInterfacePtr),
    mMuxPortConfig(
        muxConfig,
        portName,
        serverId
    ),
    mStrand(ioService),
    mLinkManagerStateMachine(
        this,
        mStrand,
        mMuxPortConfig
    )
{
    assert(dbInterfacePtr != nullptr);
}

void MuxPort::handleBladeIpv4AddressUpdate(boost::asio::ip::address address)
{
    MUXLOGDEBUG(boost::format("port: %s") % mMuxPortConfig.getPortName());

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachine::handleSwssBladeIpv4AddressUpdate,
        &mLinkManagerStateMachine,
        address
    )));
}

//
// ---> handleLinkState(const std::string &linkState);
//
// handles link state updates
//
void MuxPort::handleLinkState(const std::string &linkState)
{
    MUXLOGDEBUG(boost::format("port: %s, state db link state: %s") % mMuxPortConfig.getPortName() % linkState);

    link_state::LinkState::Label label = link_state::LinkState::Label::Down;
    if (linkState == "up") {
        label = link_state::LinkState::Label::Up;
    }

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachine::handleSwssLinkStateNotification,
        &mLinkManagerStateMachine,
        label
    )));
}

//
// ---> handleGetServerMacAddress(const std::array<uint8_t, ETHER_ADDR_LEN> &address)
//
// handles get Server MAC address
//
void MuxPort::handleGetServerMacAddress(const std::array<uint8_t, ETHER_ADDR_LEN> &address)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachine::handleGetServerMacAddressNotification,
        &mLinkManagerStateMachine,
        address
    )));
}

//
// ---> handleGetMuxState(const std::string &muxState);
//
// handles MUX state updates
//
void MuxPort::handleGetMuxState(const std::string &muxState)
{
    MUXLOGDEBUG(boost::format("port: %s, state db mux state: %s") % mMuxPortConfig.getPortName() % muxState);

    mux_state::MuxState::Label label = mux_state::MuxState::Label::Unknown;
    if (muxState == "active") {
        label = mux_state::MuxState::Label::Active;
    } else if (muxState == "standby") {
        label = mux_state::MuxState::Label::Standby;
    } else if (muxState == "error") {
        label = mux_state::MuxState::Label::Error;
    }

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachine::handleGetMuxStateNotification,
        &mLinkManagerStateMachine,
        label
    )));
}

//
// ---> handleProbeMuxState(const std::string &muxState);
//
// handles MUX state updates
//
void MuxPort::handleProbeMuxState(const std::string &muxState)
{
    MUXLOGDEBUG(boost::format("port: %s, state db mux state: %s") % mMuxPortConfig.getPortName() % muxState);

    mux_state::MuxState::Label label = mux_state::MuxState::Label::Unknown;
    if (muxState == "active") {
        label = mux_state::MuxState::Label::Active;
    } else if (muxState == "standby") {
        label = mux_state::MuxState::Label::Standby;
    }

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachine::handleProbeMuxStateNotification,
        &mLinkManagerStateMachine,
        label
    )));
}

//
// ---> handleMuxState(const std::string &muxState);
//
// handles MUX state updates
//
void MuxPort::handleMuxState(const std::string &muxState)
{
    MUXLOGDEBUG(boost::format("port: %s, state db mux state: %s") % mMuxPortConfig.getPortName() % muxState);

    mux_state::MuxState::Label label = mux_state::MuxState::Label::Unknown;
    if (muxState == "active") {
        label = mux_state::MuxState::Label::Active;
    } else if (muxState == "standby") {
        label = mux_state::MuxState::Label::Standby;
    } else if (muxState == "error") {
        label = mux_state::MuxState::Label::Error;
    }

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachine::handleMuxStateNotification,
        &mLinkManagerStateMachine,
        label
    )));
}

//
// ---> handleMuxConfig(const std::string &config);
//
// handles MUX config updates when switching between auto/active
//
void MuxPort::handleMuxConfig(const std::string &config)
{
    MUXLOGDEBUG(boost::format("port: %s, config db mux config: %s") % mMuxPortConfig.getPortName() % config);

    common::MuxPortConfig::Mode mode = common::MuxPortConfig::Auto;
    if (config == "active") {
        mode = common::MuxPortConfig::Active;
    } else if (config == "manual") {
        mode = common::MuxPortConfig::Manual;
    } else if (config == "standby") {
        mode = common::MuxPortConfig::Standby;
    }

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachine::handleMuxConfigNotification,
        &mLinkManagerStateMachine,
        mode
    )));
}

} /* namespace mux */
