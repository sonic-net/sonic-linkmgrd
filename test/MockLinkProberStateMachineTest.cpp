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

#include <iostream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "common/MuxLogger.h"
#include "MockLinkProberStateMachineTest.h"

using ::testing::InSequence;
using ::testing::Mock;

namespace test
{
LinkProberStateMachineMockTest::LinkProberStateMachineMockTest()
{
    mMuxConfig.setNegativeStateChangeRetryCount(1);
}

void LinkProberStateMachineMockTest::SetUp(common::MuxPortConfig::PortCableType portCableType, bool simulateOffload)
{
    mMuxConfig.enableSimulateLfdOffload(simulateOffload);
    mDbInterfacePtr = std::make_shared<FakeDbInterface>(&mIoService);
    mMuxPortPtr = std::make_shared<MockMuxPort>(
        mDbInterfacePtr,
        mMuxConfig,
        mPortName,
        mServerId,
        mIoService,
        portCableType
    );
    mLinkManagerStateMachinePtr = std::dynamic_pointer_cast<MockLinkManagerStateMachine>(mMuxPortPtr->getLinkManagerStateMachinePtr());
    mLinkProberStateMachinePtr = mMuxPortPtr->getLinkProberStateMachinePtr();
    mLinkProberSessionStateMachinePtr = mMuxPortPtr->getLinkProberSessionStateMachinePtr();
}

void LinkProberStateMachineMockTest::TearDown()
{
    Mock::VerifyAndClearExpectations(mLinkManagerStateMachinePtr.get());

    mIoService.stop();
}

void LinkProberStateMachineMockTest::runIoService(uint32_t count)
{
    if (count == 0) {
        if (mIoService.stopped()) {
            mIoService.restart();
        }
        mIoService.run();
    }

    for (uint8_t i = 0; i < count; i++) {
        if (mIoService.stopped()) {
            mIoService.restart();
        }
        mIoService.run_one();
    }
}

void LinkProberStateMachineMockTest::handleLinkProberSessionStateNotification(link_prober::LinkProberState::Label label, uint32_t count)
{
    switch (label) {
        case link_prober::LinkProberState::Label::SelfUp: {
            mMuxPortPtr->handleLinkProberSessionStateNotification(mMuxPortPtr->getSelfSessionId(), "Up");
            runIoService(count);
            break;
        }

        case link_prober::LinkProberState::Label::SelfDown: {
            mMuxPortPtr->handleLinkProberSessionStateNotification(mMuxPortPtr->getSelfSessionId(), "Down");
            runIoService(count);
            break;
        }
        case link_prober::LinkProberState::Label::PeerUp: {
            mMuxPortPtr->handleLinkProberSessionStateNotification(mMuxPortPtr->getPeerSessionId(), "Up");
            runIoService(count);
            break;
        }
        case link_prober::LinkProberState::Label::PeerDown: {
            mMuxPortPtr->handleLinkProberSessionStateNotification(mMuxPortPtr->getPeerSessionId(), "Down");
            runIoService(count);
            break;
        }
        default:
            break;
    }
}


TEST_F(LinkProberStateMachineMockTest, LinkProberStateMachineActiveActiveSessionWaitToActive)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveActive, true);

    InSequence seq;
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Active)
    );

    getLinkProberStateMachinePtr()->enterState(link_prober::LinkProberState::Label::Wait);
    handleLinkProberSessionStateNotification(link_prober::LinkProberState::Label::SelfUp, 2);

    TearDown();
}

TEST_F(LinkProberStateMachineMockTest, LinkProberStateMachineActiveActiveSessionWaitToUnknown)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveActive, true);

    InSequence seq;
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Unknown)
    );

    getLinkProberStateMachinePtr()->enterState(link_prober::LinkProberState::Label::Wait);
    handleLinkProberSessionStateNotification(link_prober::LinkProberState::Label::SelfDown, 2);

    TearDown();
}

TEST_F(LinkProberStateMachineMockTest, LinkProberStateMachineActiveActiveSessionUnknownToActive)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveActive, true);

    InSequence seq;
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Active)
    );
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Unknown)
    );

    getLinkProberStateMachinePtr()->enterState(link_prober::LinkProberState::Label::Unknown);
    handleLinkProberSessionStateNotification(link_prober::LinkProberState::Label::SelfUp, 2);
    handleLinkProberSessionStateNotification(link_prober::LinkProberState::Label::SelfDown, 2);

    TearDown();
}

TEST_F(LinkProberStateMachineMockTest, LinkProberStateMachineActiveActiveSessionPeerWaitToPeerActive)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveActive, true);

    InSequence seq;
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handlePeerLinkProberStateChange(link_prober::LinkProberState::Label::PeerActive)
    );

    getLinkProberStateMachinePtr()->enterPeerState(link_prober::LinkProberState::Label::PeerWait);
    handleLinkProberSessionStateNotification(link_prober::LinkProberState::Label::PeerUp, 2);

    TearDown();
}

TEST_F(LinkProberStateMachineMockTest, LinkProberStateMachineActiveActiveSessionPeerWaitToPeerUnknown)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveActive, true);

    InSequence seq;
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handlePeerLinkProberStateChange(link_prober::LinkProberState::Label::PeerUnknown)
    );

    getLinkProberStateMachinePtr()->enterPeerState(link_prober::LinkProberState::Label::PeerWait);
    handleLinkProberSessionStateNotification(link_prober::LinkProberState::Label::PeerDown, 2);

    TearDown();
}

TEST_F(LinkProberStateMachineMockTest, LinkProberStateMachineActiveActiveSessionPeerUnknownToPeerActive)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveActive, true);

    InSequence seq;
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handlePeerLinkProberStateChange(link_prober::LinkProberState::Label::PeerActive)
    );
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handlePeerLinkProberStateChange(link_prober::LinkProberState::Label::PeerUnknown)
    );

    getLinkProberStateMachinePtr()->enterPeerState(link_prober::LinkProberState::Label::PeerUnknown);
    handleLinkProberSessionStateNotification(link_prober::LinkProberState::Label::PeerUp, 2);
    handleLinkProberSessionStateNotification(link_prober::LinkProberState::Label::PeerDown, 2);

    TearDown();
}

} // namespace test
