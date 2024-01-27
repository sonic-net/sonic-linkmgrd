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

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <iostream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "common/MuxLogger.h"
#include "MockLinkProberTest.h"

using ::testing::InSequence;
using ::testing::Mock;

namespace test
{
LinkProberMockTest::LinkProberMockTest()
{
    boost::uuids::random_generator gen;
    mPeerGuid = gen();

    mMuxConfig.setNegativeStateChangeRetryCount(1);
}

void LinkProberMockTest::SetUp(common::MuxPortConfig::PortCableType portCableType, bool simulateOffload)
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
    mLinkProberPtr = std::make_shared<link_prober::LinkProber>(
        const_cast<common::MuxPortConfig&>(mMuxPortPtr->getMuxPortConfig()),
        mIoService,
        mLinkProberStateMachinePtr.get(),
        mLinkProberSessionStateMachinePtr.get()
    );
    link_prober::IcmpPayload::generateGuid();
    initializeSendBuffer();
}

void LinkProberMockTest::TearDown()
{
    // mLinkProberPtr.reset();
    // mLinkProberStateMachinePtr.reset();
    // mLinkManagerStateMachinePtr.reset();
    Mock::VerifyAndClearExpectations(mLinkManagerStateMachinePtr.get());
    Mock::VerifyAndClearExpectations(mMuxPortPtr.get());

    mIoService.stop();
}

void LinkProberMockTest::runIoService(uint32_t count)
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

void LinkProberMockTest::buildIcmpReply()
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

void LinkProberMockTest::receiveSelfIcmpReply()
{
    buildIcmpReply();
    memcpy(getRxBuffer().data(), mBuffer.data(), getRxBuffer().size());
}

void LinkProberMockTest::receivePeerIcmpReply()
{
    buildIcmpReply();
    memcpy(getRxBuffer().data(), mBuffer.data(), getRxBuffer().size());
    link_prober::IcmpPayload *icmpPayload = reinterpret_cast<link_prober::IcmpPayload *>(
        getRxBuffer().data() + getPacketHeaderSize()
    );
    memcpy(icmpPayload->uuid, mPeerGuid.data, sizeof(icmpPayload->uuid));
}

TEST_F(LinkProberMockTest, LinkProberActiveActive)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveActive);

    InSequence seq;
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Active)
    );
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handlePeerLinkProberStateChange(link_prober::LinkProberState::Label::PeerActive)
    );
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Unknown)
    );
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handlePeerLinkProberStateChange(link_prober::LinkProberState::Label::PeerUnknown)
    );

    sendHeartbeat();

    receiveSelfIcmpReply();
    handleRecv();
    runIoService();

    receivePeerIcmpReply();
    handleRecv();
    runIoService();

    handleTimeout();
    runIoService();

    sendHeartbeat();

    handleTimeout();
    runIoService();

    TearDown();
}

TEST_F(LinkProberMockTest, LinkProberActiveStandby)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveStandby);

    InSequence seq;
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Active)
    );
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Standby)
    );
    EXPECT_CALL(
        *(getLinkManagerStateMachinePtr()),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Unknown)
    );

    sendHeartbeat();

    receiveSelfIcmpReply();
    handleRecv();
    runIoService();

    handleTimeout();
    runIoService();

    sendHeartbeat();

    receivePeerIcmpReply();
    handleRecv();
    runIoService();

    handleTimeout();
    runIoService();

    sendHeartbeat();

    handleTimeout();
    runIoService();

    TearDown();
}

TEST_F(LinkProberMockTest, LinkProberSessionState)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveStandby, true);

    auto muxPortPtr = getMuxPortPtr();
    auto linkProberSessionStateMachinePtr = getLinkProberSessionStateMachinePtr();

    InSequence seq;
    EXPECT_CALL(
        *muxPortPtr,
        postLinkProberSessionStateNotificationToDb(linkProberSessionStateMachinePtr->getSelfSessionId(), link_prober::LinkProberState::Label::SelfUp)
    );
    EXPECT_CALL(
        *muxPortPtr,
        postLinkProberSessionStateNotificationToDb(linkProberSessionStateMachinePtr->getPeerSessionId(), link_prober::LinkProberState::Label::PeerUp)
    );

    EXPECT_CALL(
        *muxPortPtr,
        postLinkProberSessionStateNotificationToDb(linkProberSessionStateMachinePtr->getPeerSessionId(), link_prober::LinkProberState::Label::PeerDown)
    );

    EXPECT_CALL(
        *muxPortPtr,
        postLinkProberSessionStateNotificationToDb(linkProberSessionStateMachinePtr->getPeerSessionId(), link_prober::LinkProberState::Label::PeerUp)
    );
    EXPECT_CALL(
        *muxPortPtr,
        postLinkProberSessionStateNotificationToDb(linkProberSessionStateMachinePtr->getSelfSessionId(), link_prober::LinkProberState::Label::SelfDown)
    );

    EXPECT_CALL(
        *muxPortPtr,
        postLinkProberSessionStateNotificationToDb(linkProberSessionStateMachinePtr->getPeerSessionId(), link_prober::LinkProberState::Label::PeerDown)
    );

    // first probe, receive replies for both self and peer, self up and peer up
    sendHeartbeat();

    receiveSelfIcmpReply();
    handleRecv();
    runIoService();

    receivePeerIcmpReply();
    handleRecv();
    runIoService();

    handleTimeout();
    runIoService();

    // second probe, receive reply for self, self up and peer down
    sendHeartbeat();

    receiveSelfIcmpReply();
    handleRecv();
    runIoService();

    handleTimeout();
    runIoService();

    // third probe, receive reply for peer, self down and peer up
    sendHeartbeat();

    receivePeerIcmpReply();
    handleRecv();
    runIoService();

    handleTimeout();
    runIoService();

    // fourth probe, receive nothing, self down and peer down
    sendHeartbeat();

    handleTimeout();
    runIoService();

    TearDown();
}

} // namespace test
