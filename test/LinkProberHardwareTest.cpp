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
 * LinkProberTest.cpp
 *
 *  Created on: May 12, 2025
 *      Author: harjosin
 */

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "common/MuxException.h"
#include "link_prober/IcmpPayload.h"
#include "LinkProberHardwareTest.h"

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // For close()

#define VALIDATE_STATE(p, m, l)                                                     \
    do {                                                                            \
        mTestCompositeState = mFakeMuxPort.getCompositeState();                     \
        EXPECT_EQ(ps(mTestCompositeState), link_prober::LinkProberState::Label::p); \
        EXPECT_EQ(ms(mTestCompositeState), mux_state::MuxState::Label::m);          \
        EXPECT_EQ(ls(mTestCompositeState), link_state::LinkState::Label::l);        \
    } while (0)

#define VALIDATE_PEER_STATE(p, m)                                                                 \
    do {                                                                                          \
        EXPECT_EQ(mFakeMuxPort.getPeerLinkProberState(), link_prober::LinkProberState::Label::p); \
        EXPECT_EQ(mFakeMuxPort.getPeerMuxState(), mux_state::MuxState::Label::m);                 \
    } while (0)

namespace test
{

LinkProberHardwareTest::LinkProberHardwareTest() :
    mDbInterfacePtr(std::make_shared<FakeDbInterface> (&mIoService)),
    mFakeMuxPort(
        mDbInterfacePtr,
        mMuxConfig,
        mPortName,
        mServerId,
        mIoService,
        common::MuxPortConfig::PortCableType::ActiveActive
    ),
    mLinkProber(const_cast<common::MuxPortConfig&> (
        mFakeMuxPort.getMuxPortConfig()),
        mIoService,
        mFakeMuxPort.getLinkProberStateMachinePtr(),
        &mFakeMuxPort
    )
{
    mMuxConfig.setTimeoutIpv4_msec(1);
}

void LinkProberHardwareTest::runIoService(uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        mIoService.run_one();
        mIoService.reset();
    }
}

TEST_F(LinkProberHardwareTest, createIcmpSessionProbingTest)
{
    mLinkProber.startProbing();
    EXPECT_EQ(1, mDbInterfacePtr->mIcmpSessionsCount);
}

TEST_F(LinkProberHardwareTest, deleteIcmpSessionShutdownTest)
{
    mLinkProber.startProbing();
    mLinkProber.shutdownTxProbes();
    EXPECT_EQ(0, mDbInterfacePtr->mIcmpSessionsCount);
}

TEST_F(LinkProberHardwareTest, icmpSessionRestartTest)
{
    mLinkProber.startProbing();
    mLinkProber.restartTxProbes();
    EXPECT_EQ(1, mDbInterfacePtr->mIcmpSessionsCount);
}

TEST_F(LinkProberHardwareTest, icmpSessionSuspendTest)
{
    mLinkProber.startProbing();
    mLinkProber.suspendTxProbes(10);
    EXPECT_EQ(0, mDbInterfacePtr->mIcmpSessionsCount);
    EXPECT_TRUE(getSuspendTx());
    sleep(11);
    runIoService(2);
    EXPECT_FALSE(getSuspendTx());
}

TEST_F(LinkProberHardwareTest, updateEthernetFrameTest)
{
    mLinkProber.startProbing();
    EXPECT_EQ(1, mDbInterfacePtr->mIcmpSessionsCount);
    mLinkProber.updateEthernetFrame();
    EXPECT_EQ(1, mDbInterfacePtr->mIcmpSessionsCount);
}

TEST_F(LinkProberHardwareTest, TestHandleStateDbUpdate)
{
    std::string state = "Up";
    std::string sessionType = "NORMAL";
    mMuxConfig.enableDefaultRouteFeature(false);
    mFakeMuxPort.activateStateMachine();
    VALIDATE_STATE(Wait, Wait, Down);
    mLinkProber.handleStateDbStateUpdate(state, sessionType);
    runIoService(4);
    VALIDATE_STATE(Active, Wait, Down);
    state = "Down";
    mLinkProber.handleStateDbStateUpdate(state, sessionType);
    runIoService(4);
    VALIDATE_STATE(Unknown, Standby, Down);
}

} /* namespace test */
