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

#ifndef MOCKLINKPROBERSTATEMACHINETEST_H_
#define MOCKLINKPROBERSTATEMACHINETEST_H_

#include "common/MuxPortConfig.h"
#include "link_prober/LinkProberStateMachineActiveStandby.h"
#include "link_prober/LinkProberStateMachineActiveActive.h"

#include "FakeDbInterface.h"
#include "MockMuxPort.h"
#include "MockLinkManagerStateMachine.h"

namespace test
{
class LinkProberStateMachineMockTest : public ::testing::Test
{
public:
    LinkProberStateMachineMockTest();
    virtual ~LinkProberStateMachineMockTest() = default;
    void SetUp(common::MuxPortConfig::PortCableType portCableType, bool simulateOffload = false);
    void TearDown() override;
    void runIoService(uint32_t count = 0);
    std::shared_ptr<MockMuxPort> getMuxPortPtr() { return mMuxPortPtr; }
    std::shared_ptr<MockLinkManagerStateMachine> getLinkManagerStateMachinePtr() { return mLinkManagerStateMachinePtr; }
    std::shared_ptr<link_prober::LinkProberStateMachineBase> getLinkProberStateMachinePtr() { return mLinkProberStateMachinePtr; }
    std::shared_ptr<link_prober::LinkProberSessionStateMachine> getLinkProberSessionStateMachinePtr() { return mLinkProberSessionStateMachinePtr; }
    void handleLinkProberSessionStateNotification(link_prober::LinkProberState::Label label, uint32_t count = 0);

private:
    void buildIcmpReply();

private:
    std::string mPortName = "Ethernet4";
    uint16_t mServerId = 4;
    common::MuxConfig mMuxConfig;
    boost::asio::io_service mIoService;
    std::shared_ptr<FakeDbInterface> mDbInterfacePtr;
    std::shared_ptr<MockMuxPort> mMuxPortPtr;
    std::shared_ptr<MockLinkManagerStateMachine> mLinkManagerStateMachinePtr;
    std::shared_ptr<link_prober::LinkProberStateMachineBase> mLinkProberStateMachinePtr;
    std::shared_ptr<link_prober::LinkProberSessionStateMachine> mLinkProberSessionStateMachinePtr;
};
} // namespace test

#endif
