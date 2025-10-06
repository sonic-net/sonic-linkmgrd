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

#include "gmock/gmock.h"
#include "gtest/gtest.h"
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

using ::testing::InSequence;
using ::testing::Mock;

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
    boost::uuids::random_generator gen;
    mPeerGuid = gen();
    mLinkProber.initializeSendBuffer();
}

void LinkProberHardwareTest::changePeerGuid()
{
    boost::uuids::random_generator gen;
    mPeerGuid = gen();
    mLinkProber.initializeSendBuffer();
}

void LinkProberHardwareTest::buildIcmpReply()
{
    memcpy(mBuffer.data(), getTxBuffer().data(), mBuffer.size());
    ether_header *txEtherHeader = reinterpret_cast<ether_header *>(getTxBuffer().data());
    ether_header *rxEtherHeader = reinterpret_cast<ether_header *>(mBuffer.data());
    memcpy(rxEtherHeader->ether_shost, txEtherHeader->ether_dhost, sizeof(rxEtherHeader->ether_shost));
    memcpy(rxEtherHeader->ether_dhost, txEtherHeader->ether_shost, sizeof(rxEtherHeader->ether_dhost));

    iphdr *txIpHeader = reinterpret_cast<iphdr *>(getTxBuffer().data() + sizeof(ether_header));
    iphdr *rxIpHeader = reinterpret_cast<iphdr *>(mBuffer.data() + sizeof(ether_header));
    rxIpHeader->saddr = txIpHeader->daddr;
    rxIpHeader->daddr = txIpHeader->saddr;

    icmphdr *rxIcmpHeader = reinterpret_cast<icmphdr *>(mBuffer.data() + sizeof(ether_header) + sizeof(iphdr));
    rxIcmpHeader->type = 0;
    computeChecksum(rxIcmpHeader, sizeof(icmphdr) + getTxPacketSize() - getPacketHeaderSize());
}

void LinkProberHardwareTest::receivePeerSoftwareIcmpReply()
{
    buildIcmpReply();
    memcpy(getRxBuffer().data(), mBuffer.data(), getRxBuffer().size());
    link_prober::IcmpPayload *icmpPayload = reinterpret_cast<link_prober::IcmpPayload *>(
        getRxBuffer().data() + getPacketHeaderSize()
    );
    memcpy(icmpPayload->uuid, mPeerGuid.data, sizeof(icmpPayload->uuid));
}

void LinkProberHardwareTest::receivePeerHardwareIcmpReply()
{
    buildIcmpReply();
    memcpy(getRxBuffer().data(), mBuffer.data(), getRxBuffer().size());
    link_prober::IcmpPayload *icmpPayload = reinterpret_cast<link_prober::IcmpPayload *>(
        getRxBuffer().data() + getPacketHeaderSize()
    );
    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (
        getRxBuffer().data() + sizeof(ether_header) + sizeof(iphdr)
    );
    uint32_t HwCookie = htonl(icmpPayload->getHardwareCookie());
    memcpy(icmpPayload->uuid, mPeerGuid.data, sizeof(icmpPayload->uuid));
    icmpHeader->un.echo.id = 0;
    icmpPayload->cookie = HwCookie;

}

void LinkProberHardwareTest::runIoService(uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        mIoService.run_one();
        mIoService.reset();
    }
}

void LinkProberHardwareTest::TearDown()
{
    Mock::VerifyAndClearExpectations(mFakeMuxPort.getActiveActiveStateMachinePtr().get());
    mIoService.stop();
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

TEST_F(LinkProberHardwareTest, DISABLED_icmpSessionSuspendTest)
{
    mLinkProber.startProbing();
    mLinkProber.suspendTxProbes(2);
    EXPECT_EQ(0, mDbInterfacePtr->mIcmpSessionsCount);
    EXPECT_TRUE(getSuspendTx());
    sleep(3);
    runIoService(2);
    EXPECT_FALSE(getSuspendTx());
}

TEST_F(LinkProberHardwareTest, DISABLED_icmpSessionSuspendResumeTest)
{
    mLinkProber.startProbing();
    mLinkProber.suspendTxProbes(10000);
    EXPECT_EQ(0, mDbInterfacePtr->mIcmpSessionsCount);
    EXPECT_TRUE(getSuspendTx());
    mLinkProber.resumeTxProbes();
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

TEST_F(LinkProberHardwareTest, TestHandleSelfStateDbUpdate)
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
    state = "Up";
    mLinkProber.handleStateDbStateUpdate(state, sessionType);
    runIoService(4);
    VALIDATE_STATE(Active, Standby, Down);
}

TEST_F(LinkProberHardwareTest, TestHandlePeerStateDbUpdate)
{
    std::string state = "Up";
    std::string sessionType = "RX";
    mMuxConfig.enableDefaultRouteFeature(false);
    mFakeMuxPort.activateStateMachine();
    VALIDATE_PEER_STATE(PeerWait, Wait);
    mLinkProber.handleStateDbStateUpdate(state, sessionType);
    runIoService(4);
    VALIDATE_PEER_STATE(PeerActive, Active);
    state = "Down";
    mLinkProber.handleStateDbStateUpdate(state, sessionType);
    runIoService(4);
    VALIDATE_PEER_STATE(PeerUnknown, Active);
    state = "Up";
    mLinkProber.handleStateDbStateUpdate(state, sessionType);
    runIoService(4);
    VALIDATE_PEER_STATE(PeerActive, Active);
}

TEST_F(LinkProberHardwareTest, TestHandleStateDbWaitToUnknownUpdate)
{
    std::string state = "Down";
    std::string sessionType = "RX";
    mMuxConfig.enableDefaultRouteFeature(false);
    mFakeMuxPort.activateStateMachine();
    VALIDATE_PEER_STATE(PeerWait, Wait);
    VALIDATE_STATE(Wait, Wait, Down);
    mLinkProber.handleStateDbStateUpdate(state, sessionType);
    runIoService(4);
    VALIDATE_PEER_STATE(PeerUnknown, Wait);
    sessionType = "NORMAL";
    mLinkProber.handleStateDbStateUpdate(state, sessionType);
    runIoService(4);
    VALIDATE_STATE(Unknown, Standby, Down);
}

TEST_F(LinkProberHardwareTest, LinkProberHardwareHandleIcmpPayload)
{
    InSequence seq;
    receivePeerHardwareIcmpReply();
    handleRecv();
    runIoService(1);
    receivePeerHardwareIcmpReply();
    handleRecv();
    runIoService(1);
    changePeerGuid();
    receivePeerHardwareIcmpReply();
    handleRecv();
    runIoService(1);
    receivePeerSoftwareIcmpReply();
    handleRecv();
    runIoService(1);
    receivePeerSoftwareIcmpReply();
    handleRecv();
    runIoService(1);
    changePeerGuid();
    receivePeerSoftwareIcmpReply();
    handleRecv();
    runIoService(1);
    TearDown();
}

} /* namespace test */
