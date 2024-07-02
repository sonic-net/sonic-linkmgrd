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
#include "common/MuxException.h"
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
    boost::asio::io_service &ioService,
    common::MuxPortConfig::PortCableType portCableType
) :
    mDbInterfacePtr(dbInterfacePtr),
    mMuxPortConfig(
        muxConfig,
        portName,
        serverId,
        portCableType
    ),
    mStrand(ioService)
{
    assert(dbInterfacePtr != nullptr);
    switch (portCableType) {
        case common::MuxPortConfig::PortCableType::ActiveActive:
            mLinkManagerStateMachinePtr = std::make_shared<link_manager::ActiveActiveStateMachine>(
                this,
                mStrand,
                mMuxPortConfig
            );
            break;
        case common::MuxPortConfig::PortCableType::ActiveStandby:
            mLinkManagerStateMachinePtr = std::make_shared<link_manager::ActiveStandbyStateMachine>(
                this,
                mStrand,
                mMuxPortConfig
            );
            break;
        default:
            break;
    }
    assert(mLinkManagerStateMachinePtr.get() != nullptr);

    if (mMuxPortConfig.getIfEnableSimulateLfdOffload()) {
        initLinkProberSessions();
    }
}

void MuxPort::handleBladeIpv4AddressUpdate(boost::asio::ip::address address)
{
    MUXLOGDEBUG(boost::format("port: %s") % mMuxPortConfig.getPortName());

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleSwssBladeIpv4AddressUpdate,
        mLinkManagerStateMachinePtr.get(),
        address
    )));
}

//
// ---> handleSoCIpv4AddressUpdate(boost::asio::ip::address address);
//
// handles SoC IP address update for port in active-active cable type
//
void MuxPort::handleSoCIpv4AddressUpdate(boost::asio::ip::address address)
{
    MUXLOGDEBUG(boost::format("port: %s") % mMuxPortConfig.getPortName());

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleSwssSoCIpv4AddressUpdate,
        mLinkManagerStateMachinePtr.get(),
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
        &link_manager::LinkManagerStateMachineBase::handleSwssLinkStateNotification,
        mLinkManagerStateMachinePtr.get(),
        label
    )));
}

//
// ---> handlePeerLinkState(const std::string &linkState); 
//
// handle peer's link state updates
//
void MuxPort::handlePeerLinkState(const std::string &linkState)
{
    MUXLOGDEBUG(boost::format("port: %s, state db peer link state: %s") % mMuxPortConfig.getPortName() % linkState);

    link_state::LinkState::Label label = link_state::LinkState::Label::Up;
    if (linkState == "down") {
        label = link_state::LinkState::Label::Down;
    }

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handlePeerLinkStateNotification,
        mLinkManagerStateMachinePtr.get(),
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
        &link_manager::LinkManagerStateMachineBase::handleGetServerMacAddressNotification,
        mLinkManagerStateMachinePtr.get(),
        address
    )));
}

//
// ---> handleUseWellKnownMacAddress()
//
// handles use well known mac
//
void MuxPort::handleUseWellKnownMacAddress()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleUseWellKnownMacAddressNotification,
        mLinkManagerStateMachinePtr.get()
    )));
}

//
// ---> handleSrcMacAddressUpdate();
// 
// handles src mac address config update 
//
void MuxPort::handleSrcMacAddressUpdate()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleSrcMacConfigNotification,
        mLinkManagerStateMachinePtr.get()
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
        &link_manager::LinkManagerStateMachineBase::handleGetMuxStateNotification,
        mLinkManagerStateMachinePtr.get(),
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
    } else if (muxState == "failure" && mMuxPortConfig.getPortCableType() == common::MuxPortConfig::PortCableType::ActiveActive) {
        // gRPC connection failure for for active-active interfaces 
        boost::asio::post(mStrand, boost::bind(
            &link_manager::LinkManagerStateMachineBase::handleProbeMuxFailure,
            mLinkManagerStateMachinePtr.get()
        ));

        return;
    }

    boost::asio::post(mStrand, boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleProbeMuxStateNotification,
        mLinkManagerStateMachinePtr.get(),
        label
    ));
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
        &link_manager::LinkManagerStateMachineBase::handleMuxStateNotification,
        mLinkManagerStateMachinePtr.get(),
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
    } else if (config == "detach") {
        if (mMuxPortConfig.getPortCableType() ==  common::MuxPortConfig::PortCableType::ActiveStandby) {
            MUXLOGWARNING(boost::format("port: %s, detach mode is only supported for acitve-active cable type")  % mMuxPortConfig.getPortName());
            return;
        }
        mode = common::MuxPortConfig::Detached;
    }

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleMuxConfigNotification,
        mLinkManagerStateMachinePtr.get(),
        mode
    )));
}

//
// ---> handlePeerMuxState(const std::string &peerMuxState);
//
// handles peer MUX state updates
//
void MuxPort::handlePeerMuxState(const std::string &peerMuxState)
{
    MUXLOGDEBUG(boost::format("port: %s, state db peer mux state: %s") % mMuxPortConfig.getPortName() % peerMuxState);

    mux_state::MuxState::Label label = mux_state::MuxState::Label::Unknown;
    if (peerMuxState == "active") {
        label = mux_state::MuxState::Label::Active;
    } else if (peerMuxState == "standby") {
        label = mux_state::MuxState::Label::Standby;
    } else if (peerMuxState == "error") {
        label = mux_state::MuxState::Label::Error;
    }

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handlePeerMuxStateNotification,
        mLinkManagerStateMachinePtr.get(),
        label
    )));
}

// 
// ---> handleDefaultRouteState(const std::string &routeState);
// 
// handles default route state notification
//
void MuxPort::handleDefaultRouteState(const std::string &routeState)
{
    MUXLOGWARNING(boost::format("port: %s, state db default route state: %s") % mMuxPortConfig.getPortName() % routeState);

    link_manager::LinkManagerStateMachineBase::DefaultRoute state = link_manager::LinkManagerStateMachineBase::DefaultRoute::OK;
    if (routeState == "na" && mMuxPortConfig.ifEnableDefaultRouteFeature()) {
        state = link_manager::LinkManagerStateMachineBase::DefaultRoute::NA;
    }

    boost::asio::post(mStrand, boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleDefaultRouteStateNotification,
        mLinkManagerStateMachinePtr.get(),
        state
    ));
}

// 
// ---> resetPckLossCount();
// 
// reset ICMP packet loss count 
//
void MuxPort::resetPckLossCount()
{
    MUXLOGDEBUG(boost::format("port: %s, reset ICMP packet loss counts ") % mMuxPortConfig.getPortName());

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleResetLinkProberPckLossCount,
        mLinkManagerStateMachinePtr.get()
    )));
}

//
// ---> probeMuxState()
//
// trigger xcvrd to read MUX state using i2c
//
void MuxPort::probeMuxState()
{
    switch (mMuxPortConfig.getPortCableType()) {
        case common::MuxPortConfig::PortCableType::ActiveActive:
            mDbInterfacePtr->probeForwardingState(mMuxPortConfig.getPortName());
            break;
        case common::MuxPortConfig::PortCableType::ActiveStandby:
            mDbInterfacePtr->probeMuxState(mMuxPortConfig.getPortName());
            break;
        default:
            break;
    }
}

//
// ---> warmRestartReconciliation();
//
// brief port warm restart reconciliation procedure
//
void MuxPort::warmRestartReconciliation()
{
    if (mMuxPortConfig.getMode() != common::MuxPortConfig::Mode::Auto) {
        mDbInterfacePtr->warmRestartReconciliation(mMuxPortConfig.getPortName());
    }
}

//
// ---> handleTsaEnable();
//
// handle TSA Enable 
//
void MuxPort::handleTsaEnable(bool enable)
{
    MUXLOGWARNING(
        boost::format("%s: configuring mux mode due to CONFIG DB tsa_enable notification: %d") %
        mMuxPortConfig.getPortName() %
        enable
    );

    common::MuxPortConfig::Mode mode = common::MuxPortConfig::Auto;
    if (enable) {
        mode = common::MuxPortConfig::Standby;
    }

    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleMuxConfigNotification,
        mLinkManagerStateMachinePtr.get(),
        mode
    )));
}

//
// ---> initLinkProberSessions();
//
// initializes the link prober sessions
//
void MuxPort::initLinkProberSessions()
{
    mSelfSessionId = mLinkManagerStateMachinePtr->getLinkProberSessionStateMachinePtr()->getSelfSessionId();
    mPeerSessionId = mLinkManagerStateMachinePtr->getLinkProberSessionStateMachinePtr()->getPeerSessionId();
}

//
// ---> handleLinkProberSessionStateNotification(const std::string &sessionId, const std::string &sessionState);
//
// handle link prober session state notification
//
void MuxPort::handleLinkProberSessionStateNotification(const std::string &sessionId, const std::string &sessionState)
{
    MUXLOGWARNING(boost::format("%s: session %s, session state: %s") % mMuxPortConfig.getPortName() % sessionId % sessionState);

    if (sessionId == mSelfSessionId) {
        if (sessionState == "Up") {
            mLinkProberStateMachinePtr->postLinkProberStateEvent(
                link_prober::LinkProberStateMachineBase::getLinkProberSelfUpEvent()
            );
        } else if (sessionState == "Down") {
            mLinkProberStateMachinePtr->postLinkProberStateEvent(
                link_prober::LinkProberStateMachineBase::getLinkProberSelfDownEvent()
            );
        } else {
            MUXLOGERROR(
                boost::format("%s: unrecognized link prober session state: %s") %
                mMuxPortConfig.getPortName() %
                sessionState
            );
        }
    } else if (sessionId == mPeerSessionId) {
        if (sessionState == "Up") {
            mLinkProberStateMachinePtr->postLinkProberStateEvent(
                link_prober::LinkProberStateMachineBase::getLinkProberPeerUpEvent()
            );
        } else if (sessionState == "Down") {
            mLinkProberStateMachinePtr->postLinkProberStateEvent(
                link_prober::LinkProberStateMachineBase::getLinkProberPeerDownEvent()
            );
        } else {
            MUXLOGERROR(
                boost::format("%s: unrecognized link prober session state: %s") %
                mMuxPortConfig.getPortName() %
                sessionState
            );
        }
    } else {
        MUXLOGERROR(
            boost::format("%s: unrecognized link prober session id: %s") %
            mMuxPortConfig.getPortName() %
            sessionId
        );
    }
}

} /* namespace mux */
