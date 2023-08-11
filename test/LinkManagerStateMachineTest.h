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
 * LinkManagerStateMachineTest.h
 *
 *  Created on: Oct 25, 2020
 *      Author: Tamer Ahmed
 */

#ifndef LINKMANAGERSTATEMACHINETEST_H_
#define LINKMANAGERSTATEMACHINETEST_H_

#include "gtest/gtest.h"

#include "FakeMuxPort.h"
#include "FakeLinkProber.h"

namespace test
{

class LinkManagerStateMachineTest: public ::testing::Test
{
public:
    LinkManagerStateMachineTest();
    virtual ~LinkManagerStateMachineTest() = default;

    void runIoService(uint32_t count = 0);
    void postLinkProberEvent(link_prober::LinkProberState::Label label, uint32_t count = 0, uint32_t detect_multiplier = 0);
    void postMuxEvent(mux_state::MuxState::Label label, uint32_t count = 0);
    void postLinkEvent(link_state::LinkState::Label label, uint32_t count = 0);
    void postSuspendTimerExpiredEvent(uint32_t count = 0);
    void handleMuxState(std::string, uint32_t count = 0);
    void handleGetMuxState(std::string, uint32_t count = 0);
    void handleProbeMuxState(std::string, uint32_t count = 0);
    void handleLinkState(std::string linkState, uint32_t count = 0);
    void handleMuxConfig(std::string config, uint32_t count = 0);
    void activateStateMachine();
    void setMuxActive();
    void setMuxStandby();
    void postDefaultRouteEvent(std::string routeState, uint32_t count = 0);
    void postPeerLinkStateEvent(std::string linkState, uint32_t count = 0);
    void postPckLossRatioUpdateEvent(uint64_t unknownCount, uint64_t totalCount);
    void postPckLossCountsResetEvent();

public:
    boost::asio::io_service mIoService;
    common::MuxConfig mMuxConfig;
    std::shared_ptr<FakeDbInterface> mDbInterfacePtr;
    std::string mPortName = "EtherTest01";
    std::string mSmartNicIpAddress = "192.168.1.20";
    uint16_t mServerId = 01;

    FakeMuxPort mFakeMuxPort;
    link_manager::ActiveStandbyStateMachine::CompositeState mTestCompositeState;

    uint8_t mPositiveUpdateCount = 2;
};

} /* namespace test */

#endif /* LINKMANAGERSTATEMACHINETEST_H_ */
