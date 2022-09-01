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

#ifndef LINKMANAGERSTATEMACHINEACTIVEACTIVETEST_H_
#define LINKMANAGERSTATEMACHINEACTIVEACTIVETEST_H_

#include "gtest/gtest.h"

#include "FakeMuxPort.h"
#include "FakeLinkProber.h"

namespace test
{

class LinkManagerStateMachineActiveActiveTest : public ::testing::Test
{
public:
    LinkManagerStateMachineActiveActiveTest();
    virtual ~LinkManagerStateMachineActiveActiveTest() = default;

    void runIoService(uint32_t count = 0);
    void pollIoService(uint32_t count = 0);
    void postLinkProberEvent(link_prober::LinkProberState::Label label, uint32_t count = 0);
    void postPeerLinkProberEvent(link_prober::LinkProberState::Label label, uint32_t count = 0);
    void postMuxEvent(mux_state::MuxState::Label label, uint32_t count = 0);
    void postLinkEvent(link_state::LinkState::Label label, uint32_t count = 0);
    void postSuspendTimerExpiredEvent(uint32_t count = 0);
    void handleMuxState(std::string, uint32_t count = 0);
    void handlePeerMuxState(std::string, uint32_t count = 0);
    void handleProbeMuxState(std::string, uint32_t count = 0);
    void handleLinkState(std::string linkState, uint32_t count = 0);
    void handleMuxConfig(std::string config, uint32_t count = 0);
    void activateStateMachine(bool enable_feature_default_route=false);
    void setMuxActive();
    void setMuxStandby();
    void postDefaultRouteEvent(std::string routeState, uint32_t count = 0);
    const common::MuxPortConfig &getMuxPortConfig();

public:
    boost::asio::io_service mIoService;
    common::MuxConfig mMuxConfig;
    std::shared_ptr<FakeDbInterface> mDbInterfacePtr;
    std::string mPortName = "Ethernet10";
    std::string mSmartNicIpAddress = "192.168.1.20";
    common::MuxPortConfig::PortCableType mPortCableType = common::MuxPortConfig::PortCableType::ActiveActive;
    uint16_t mServerId = 10;

    FakeMuxPort mFakeMuxPort;
    link_manager::ActiveStandbyStateMachine::CompositeState mTestCompositeState;

    uint8_t mPositiveUpdateCount = 2;
};

} /* namespace test */

#endif /* LINKMANAGERSTATEMACHINEACTIVEACTIVETEST_H_ */
