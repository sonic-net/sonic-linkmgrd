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

#include <common/BoostAsioBehavior.h>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include "FakeLinkManagerStateMachine.h"
#include "common/MuxLogger.h"

namespace test
{

FakeLinkManagerStateMachine::FakeLinkManagerStateMachine(
    mux::MuxPort *muxPortPtr,
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig
) :
    link_manager::LinkManagerStateMachineBase(
        muxPortPtr,
        strand,
        muxPortConfig,
        {link_prober::LinkProberState::Label::Wait,
        mux_state::MuxState::Label::Wait,
        link_state::LinkState::Label::Down}
    )
{
    assert(muxPortPtr != nullptr);
}

void FakeLinkManagerStateMachine::handleSwssBladeIpv4AddressUpdate(boost::asio::ip::address address)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mLastSwssBladeIpv4Address = address;
    ++mSwssBladeIpv4AddressUpdateHandlerCalled;
}

void FakeLinkManagerStateMachine::handleSwssSoCIpv4AddressUpdate(boost::asio::ip::address address)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mLastSwssSoCIpv4Address = address;
    ++mSwssSoCIpv4AddressUpdateHandlerCalled;
}

void FakeLinkManagerStateMachine::handleGetServerMacAddressNotification(std::array<uint8_t, ETHER_ADDR_LEN> address)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mLastServerMacAddress = address;
    ++mGetServerMacAddressNotificationHandlerCalled;
}

void FakeLinkManagerStateMachine::handleGetMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mLastGetMuxState = label;
    ++mGetMuxStateNotificationHandlerCalled;
}

void FakeLinkManagerStateMachine::handleProbeMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mLastProbeMuxState = label;
    ++mProbeMuxStateNotificationHandlerCalled;
}

void FakeLinkManagerStateMachine::handleMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mLastMuxStateNotification = label;
    ++mMuxStateNotificationHandlerCalled;
}

void FakeLinkManagerStateMachine::handleSwssLinkStateNotification(const link_state::LinkState::Label label)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mLastSwssLinkState = label;
    ++mSwssLinkStateNotificationHandlerCalled;
}

void FakeLinkManagerStateMachine::handlePeerLinkStateNotification(const link_state::LinkState::Label label)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mLastPeerLinkState = label;
    ++mPeerLinkStateNotificationHandlerCalled;
}

void FakeLinkManagerStateMachine::handlePeerMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mLastPeerMuxState = label;
    ++mPeerMuxStateNotificationHandlerCalled;
}

void FakeLinkManagerStateMachine::handleMuxConfigNotification(const common::MuxPortConfig::Mode mode)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mLastMuxConfig = mode;
    ++mMuxConfigNotificationHandlerCalled;
}

void FakeLinkManagerStateMachine::handleDefaultRouteStateNotification(const DefaultRoute routeState)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mLastDefaultRouteState = routeState;
    ++mDefaultRouteStateNotificationHandlerCalled;
}

} /* namespace test */
