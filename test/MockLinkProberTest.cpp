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
#include <boost/uuid/string_generator.hpp>
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <iostream>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include <netinet/icmp6.h>
#include <netinet/ip6.h>

#include "common/MuxLogger.h"
#include "MockLinkProberTest.h"

using ::testing::InSequence;
using ::testing::Mock;

namespace test
{
LinkProberMockTest::LinkProberMockTest()
    : mMuxPortConfig(mMuxConfig, mPortName, mServerId, common::MuxPortConfig::PortCableType::DefaultType),
      mStrand(mIoService)
{
    boost::uuids::string_generator gen;
    PeerGuid = gen("ABCDEF12-3456-7890-ABCD-EF1234567890");
    mMuxConfig.setNegativeStateChangeRetryCount(1);
}

void LinkProberMockTest::SetUp(common::MuxPortConfig::PortCableType portCableType, bool ipv6)
{
    mMuxPortConfig.setPortCableType(portCableType);
    mLinkManagerStateMachinePtr = std::make_shared<MockLinkManagerStateMachine>(
        mStrand,
        mMuxPortConfig
    );
    switch (portCableType) {
        case common::MuxPortConfig::PortCableType::ActiveStandby: {
            mLinkProberStateMachinePtr = std::make_shared<link_prober::LinkProberStateMachineActiveStandby>(
                mLinkManagerStateMachinePtr.get(),
                mStrand,
                mMuxPortConfig,
                link_prober::LinkProberState::Label::Unknown
            );
            break;
        }
        case common::MuxPortConfig::PortCableType::ActiveActive: {
            mLinkProberStateMachinePtr = std::make_shared<link_prober::LinkProberStateMachineActiveActive>(
                mLinkManagerStateMachinePtr.get(),
                mStrand,
                mMuxPortConfig,
                link_prober::LinkProberState::Label::Unknown
            );
            break;
        }
        default: {
            break;
        }
    }
    mMuxPortConfig.setBladeIpv4Address(boost::asio::ip::make_address(ipv6 ? "fc02:1000::9" : "192.168.0.9"));
    if (ipv6) {
        auto loopback = boost::asio::ip::make_address("fc00:1::1");
        auto loopback3 = boost::asio::ip::make_address("fc00:3::1");
        mMuxConfig.setLoopbackIpv6Address(loopback);
        mMuxConfig.setLoopback3Ipv6Address(loopback3);
    }
    mLinkProberPtr = std::make_shared<link_prober::LinkProberSw>(
        mMuxPortConfig,
        mIoService,
        mLinkProberStateMachinePtr.get()
    );
    initializeSendBuffer();
}

void LinkProberMockTest::TearDown()
{
    // mLinkProberPtr.reset();
    // mLinkProberStateMachinePtr.reset();
    // mLinkManagerStateMachinePtr.reset();
    Mock::VerifyAndClearExpectations(mLinkManagerStateMachinePtr.get());

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

    if (mMuxPortConfig.getBladeIpv4Address().is_v6()) {
        auto *txIpHeader = reinterpret_cast<ip6_hdr *>(getTxBuffer().data() + sizeof(ether_header));
        auto *rxIpHeader = reinterpret_cast<ip6_hdr *>(mBuffer.data() + sizeof(ether_header));
        memcpy(&rxIpHeader->ip6_src, &txIpHeader->ip6_dst, sizeof(rxIpHeader->ip6_src));
        memcpy(&rxIpHeader->ip6_dst, &txIpHeader->ip6_src, sizeof(rxIpHeader->ip6_dst));
        auto *icmpHeader = reinterpret_cast<icmphdr *>(mBuffer.data() + getPacketHeaderSize() - sizeof(icmphdr));
        icmpHeader->type = ICMP6_ECHO_REPLY;
        mLinkProberPtr->computeIcmpv6Checksum(icmpHeader, getTxPacketSize() - getPacketHeaderSize() + sizeof(icmphdr));
        return;
    }

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
    memcpy(icmpPayload->uuid, PeerGuid.data, sizeof(icmpPayload->uuid));
    if (mMuxPortConfig.getBladeIpv4Address().is_v6()) {
        auto *icmpHeader = reinterpret_cast<icmphdr *>(getRxBuffer().data() + getPacketHeaderSize() - sizeof(icmphdr));
        mLinkProberPtr->computeIcmpv6Checksum(icmpHeader, getTxPacketSize() - getPacketHeaderSize() + sizeof(icmphdr));
    }
    std::string tempGuid;
    mLinkProberPtr->getGuidStr(icmpPayload, tempGuid);
    setPeerGuidData(tempGuid);
}

void LinkProberMockTest::receivePeerIcmpReplyWithShortCommandTlv()
{
    receivePeerIcmpReply();
    size_t tlvStartOffset = getPacketHeaderSize() + sizeof(link_prober::IcmpPayload);
    link_prober::Tlv *tlvPtr = reinterpret_cast<link_prober::Tlv *>(
        getRxBuffer().data() + tlvStartOffset
    );
    tlvPtr->tlvhead.type = link_prober::TlvType::TLV_COMMAND;
    tlvPtr->tlvhead.length = 0;
    tlvPtr->command = static_cast<uint8_t>(link_prober::Command::COMMAND_SWITCH_ACTIVE);
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

TEST_F(LinkProberMockTest, LinkProberActiveStandbyIgnoresShortCommandTlv)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveStandby);

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
        handleSwitchActiveRequestEvent()
    ).Times(0);

    sendHeartbeat();

    receiveSelfIcmpReply();
    handleRecv();
    runIoService();

    sendHeartbeat();

    receivePeerIcmpReplyWithShortCommandTlv();
    handleRecv();
    runIoService();

    TearDown();
}

TEST_F(LinkProberMockTest, RejectsRepliesFromOtherEndpoints)
{
    for (bool ipv6 : {false, true}) {
        SetUp(common::MuxPortConfig::PortCableType::ActiveActive, ipv6);
        EXPECT_CALL(*getLinkManagerStateMachinePtr(),
            handlePeerLinkProberStateChange(link_prober::LinkProberState::Label::PeerActive)).Times(0);
        sendHeartbeat();
        receivePeerIcmpReply();
        setPeerGuidData("");
        if (ipv6) {
            auto *header = reinterpret_cast<ip6_hdr *>(getRxBuffer().data() + sizeof(ether_header));
            auto source = boost::asio::ip::make_address_v6("fc02:1000::99").to_bytes();
            memcpy(&header->ip6_src, source.data(), source.size());
        } else {
            auto *header = reinterpret_cast<iphdr *>(getRxBuffer().data() + sizeof(ether_header));
            header->saddr = htonl(boost::asio::ip::make_address_v4("192.168.0.99").to_uint());
        }
        handleRecv();
        runIoService();
        EXPECT_TRUE(getLinkProberPtr()->getPeerGuidData().empty());
        TearDown();
    }
}

TEST_F(LinkProberMockTest, Ipv6SelfAndPeerHeartbeats)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveActive, true);
    InSequence sequence;
    EXPECT_CALL(*getLinkManagerStateMachinePtr(),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Active));
    EXPECT_CALL(*getLinkManagerStateMachinePtr(),
        handlePeerLinkProberStateChange(link_prober::LinkProberState::Label::PeerActive));

    sendHeartbeat();
    receiveSelfIcmpReply();
    handleRecv();
    runIoService();
    receivePeerIcmpReply();
    handleRecv();
    runIoService();
}

TEST_F(LinkProberMockTest, Ipv6PeerSwitchCommand)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveStandby, true);
    EXPECT_CALL(*getLinkManagerStateMachinePtr(),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Active));
    EXPECT_CALL(*getLinkManagerStateMachinePtr(),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Standby));
    EXPECT_CALL(*getLinkManagerStateMachinePtr(), handleSwitchActiveRequestEvent());

    sendHeartbeat();
    receiveSelfIcmpReply();
    handleRecv();
    runIoService();
    sendHeartbeat();
    getLinkProberPtr()->initTxBufferTlvSendSwitch();
    receivePeerIcmpReply();
    handleRecv();
    runIoService();
}

TEST_F(LinkProberMockTest, Ipv6TruncatedReplyDoesNotReuseSelfPayload)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveActive, true);
    EXPECT_CALL(*getLinkManagerStateMachinePtr(),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Active)).Times(1);
    EXPECT_CALL(*getLinkManagerStateMachinePtr(),
        handleLinkProberStateChange(link_prober::LinkProberState::Label::Unknown)).Times(1);
    EXPECT_CALL(*getLinkManagerStateMachinePtr(),
        handlePeerLinkProberStateChange(link_prober::LinkProberState::Label::PeerUnknown)).Times(1);

    sendHeartbeat();
    receiveSelfIcmpReply();
    handleRecv();
    runIoService();
    sendHeartbeat();
    handleTimeout();
    runIoService();
    for (size_t payloadSize = 0; payloadSize < sizeof(link_prober::IcmpPayload); ++payloadSize) {
        handleRecv(getPacketHeaderSize() + payloadSize);
        runIoService();
    }
}

TEST_F(LinkProberMockTest, Ipv6TruncatedReplyDoesNotRelearnPeer)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveActive, true);
    EXPECT_CALL(*getLinkManagerStateMachinePtr(),
        handlePeerLinkProberStateChange(link_prober::LinkProberState::Label::PeerActive));

    sendHeartbeat();
    receivePeerIcmpReply();
    handleRecv();
    runIoService();
    setPeerGuidData("");
    for (size_t payloadSize = 0; payloadSize < sizeof(link_prober::IcmpPayload); ++payloadSize) {
        handleRecv(getPacketHeaderSize() + payloadSize);
        runIoService();
        EXPECT_TRUE(getLinkProberPtr()->getPeerGuidData().empty());
    }
}

} // namespace test
