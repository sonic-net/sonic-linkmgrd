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
 * FakeLinkManagerStateMachine.h
 *
 *  Created on: Oct 23, 2020
 *      Author: Longxiang Lyu <lolv@microsoft.com>
 */

#ifndef FAKELINKMANAGERSTATEMACHINE_H_
#define FAKELINKMANAGERSTATEMACHINE_H_

#include "link_manager/LinkManagerStateMachineBase.h"

namespace test
{

class FakeLinkManagerStateMachine : public link_manager::LinkManagerStateMachineBase
{
public:
    FakeLinkManagerStateMachine(
        mux::MuxPort *muxPortPtr,
        boost::asio::io_service::strand &strand,
        common::MuxPortConfig &muxPortConfig
    );

    ~FakeLinkManagerStateMachine() = default;

    void setLabel(Label label) override {};
    void initializeTransitionFunctionTable() override {};
    void handleStateChange(link_manager::LinkProberEvent& event, link_prober::LinkProberState::Label state) override {};
    void handleStateChange(link_manager::MuxStateEvent& event, mux_state::MuxState::Label state) override {};
    void handleStateChange(link_manager::LinkStateEvent& event, link_state::LinkState::Label state) override {};
    void handleSwssBladeIpv4AddressUpdate(boost::asio::ip::address address) override;
    void handleSwssSoCIpv4AddressUpdate(boost::asio::ip::address address) override;
    void handleGetServerMacAddressNotification(std::array<uint8_t, ETHER_ADDR_LEN> address) override;
    void handleGetMuxStateNotification(mux_state::MuxState::Label label) override;
    void handleProbeMuxStateNotification(mux_state::MuxState::Label label) override;
    void handleMuxStateNotification(mux_state::MuxState::Label label) override;
    void handleSwssLinkStateNotification(const link_state::LinkState::Label label) override;
    void handlePeerLinkStateNotification(const link_state::LinkState::Label label) override;
    void handlePeerMuxStateNotification(mux_state::MuxState::Label label) override;
    void handleMuxConfigNotification(const common::MuxPortConfig::Mode mode) override;
    void handleDefaultRouteStateNotification(const DefaultRoute routeState) override;

public:
    boost::asio::ip::address mLastSwssBladeIpv4Address;
    uint32_t mSwssBladeIpv4AddressUpdateHandlerCalled = 0;

    boost::asio::ip::address mLastSwssSoCIpv4Address;
    uint32_t mSwssSoCIpv4AddressUpdateHandlerCalled = 0;

    std::array<uint8_t, ETHER_ADDR_LEN> mLastServerMacAddress;
    uint32_t mGetServerMacAddressNotificationHandlerCalled = 0;

    mux_state::MuxState::Label mLastGetMuxState;
    uint32_t mGetMuxStateNotificationHandlerCalled = 0;

    mux_state::MuxState::Label mLastProbeMuxState;
    uint32_t mProbeMuxStateNotificationHandlerCalled = 0;

    mux_state::MuxState::Label mLastMuxStateNotification;
    uint32_t mMuxStateNotificationHandlerCalled = 0;

    link_state::LinkState::Label mLastSwssLinkState;
    uint32_t mSwssLinkStateNotificationHandlerCalled = 0;

    link_state::LinkState::Label mLastPeerLinkState;
    uint32_t mPeerLinkStateNotificationHandlerCalled = 0;

    mux_state::MuxState::Label mLastPeerMuxState;
    uint32_t mPeerMuxStateNotificationHandlerCalled = 0;

    common::MuxPortConfig::Mode mLastMuxConfig;
    uint32_t mMuxConfigNotificationHandlerCalled = 0;

    DefaultRoute mLastDefaultRouteState;
    uint32_t mDefaultRouteStateNotificationHandlerCalled = 0;
};

} /* namespace test */

#endif /* FAKELINKMANAGERSTATEMACHINE_H_ */
