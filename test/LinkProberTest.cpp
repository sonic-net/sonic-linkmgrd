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
 *  Created on: May 12, 2021
 *      Author: taahme
 */

#include <boost/lexical_cast.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "common/MuxException.h"
#include "link_prober/IcmpPayload.h"
#include "LinkProberTest.h"

namespace test
{

LinkProberTest::LinkProberTest() :
    mDbInterfacePtr(std::make_shared<FakeDbInterface> (&mIoService)),
    mFakeMuxPort(
        mDbInterfacePtr,
        mMuxConfig,
        mPortName,
        mServerId,
        mIoService
    ),
    mLinkProber(const_cast<common::MuxPortConfig&> (
        mFakeMuxPort.getMuxPortConfig()),
        mIoService,
        mFakeMuxPort.getLinkProberStateMachinePtr(),
        nullptr
    )
{
    mMuxConfig.setTimeoutIpv4_msec(1);
}

size_t LinkProberTest::appendTlvCommand(link_prober::Command commandType)
{
    return mLinkProber.appendTlvCommand(commandType);
}

size_t LinkProberTest::appendTlvSentinel()
{
    return mLinkProber.appendTlvSentinel();
}

size_t LinkProberTest::appendTlvDummy(size_t paddingSize, int seqNo)
{
    return mLinkProber.appendTlvDummy(paddingSize, seqNo);
}

size_t LinkProberTest::findNextTlv(size_t readOffset, size_t bytesTransferred)
{
    return mLinkProber.findNextTlv(readOffset, bytesTransferred);
}

TEST_F(LinkProberTest, InitializeSendBuffer)
{
    initializeSendBuffer();
    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> txBuffer = getTxBuffer();

    ether_header *ethHeader = reinterpret_cast<ether_header *> (txBuffer.data());
    EXPECT_TRUE(memcmp(
        ethHeader->ether_dhost,
        mFakeMuxPort.getMuxPortConfig().getBladeMacAddress().data(),
        sizeof(ethHeader->ether_dhost)
    ) == 0);
    EXPECT_TRUE(ethHeader->ether_type == htons(ETHERTYPE_IP));

    iphdr *ipHeader = reinterpret_cast<iphdr *> (txBuffer.data() + sizeof(ether_header));
    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (txBuffer.data() + sizeof(ether_header) + sizeof(iphdr));
    link_prober::IcmpPayload *icmpPayload = reinterpret_cast<link_prober::IcmpPayload *> (txBuffer.data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr));
    link_prober::Tlv *tlvPtr = reinterpret_cast<link_prober::Tlv *> (txBuffer.data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr) + sizeof(*icmpPayload));

    EXPECT_TRUE(ipHeader->ihl == sizeof(iphdr) >> 2);
    EXPECT_TRUE(ipHeader->version == IPVERSION);
    EXPECT_TRUE(ipHeader->tos == 0xb8);
    EXPECT_TRUE(ipHeader->tot_len == htons(sizeof(iphdr) + sizeof(icmphdr) + sizeof(link_prober::IcmpPayload) + sizeof(link_prober::TlvHead)));
    EXPECT_TRUE(ipHeader->frag_off == 0);
    EXPECT_TRUE(ipHeader->ttl == 64);
    EXPECT_TRUE(ipHeader->protocol == IPPROTO_ICMP);
    EXPECT_TRUE(ipHeader->check == 62919);
    EXPECT_TRUE(ipHeader->saddr == htonl(mFakeMuxPort.getMuxPortConfig().getLoopbackIpv4Address().to_v4().to_uint()));
    EXPECT_TRUE(ipHeader->daddr == htonl(mFakeMuxPort.getMuxPortConfig().getBladeIpv4Address().to_v4().to_uint()));

    EXPECT_TRUE(icmpHeader->type == ICMP_ECHO);
    EXPECT_TRUE(icmpHeader->code == 0);
    EXPECT_TRUE(icmpHeader->un.echo.id == htons(mFakeMuxPort.getMuxPortConfig().getServerId()));
    EXPECT_TRUE(icmpHeader->un.echo.sequence == htons(0xffff));

    EXPECT_TRUE(icmpPayload->cookie == htonl(link_prober::IcmpPayload::getCookie()));
    EXPECT_TRUE(icmpPayload->version == htonl(link_prober::IcmpPayload::getVersion()));
    EXPECT_TRUE(memcmp(
        icmpPayload->uuid,
        link_prober::IcmpPayload::getGuidData(),
        sizeof(icmpPayload->uuid)
    ) == 0);

    EXPECT_TRUE(tlvPtr->tlvhead.type == link_prober::TlvType::TLV_SENTINEL);
    EXPECT_TRUE(tlvPtr->tlvhead.length == 0);
}

TEST_F(LinkProberTest, CalculateChecksum)
{
    link_prober::IcmpPayload *icmpPayload = new (
        getTxBuffer().data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr)
    ) link_prober::IcmpPayload();
    boost::uuids::uuid guid = boost::lexical_cast<boost::uuids::uuid> ("44f49d86-c312-414b-b6a1-be82901ac459");
    memcpy(icmpPayload->uuid, guid.data, sizeof(icmpPayload->uuid));
    initializeSendBuffer();

    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (getTxBuffer().data() + sizeof(ether_header) + sizeof(iphdr));
    EXPECT_TRUE(icmpHeader->checksum == 12100);
}

TEST_F(LinkProberTest, handleSendSwitchCommand)
{
    initializeSendBuffer();

    iphdr *ipHeader = reinterpret_cast<iphdr *>(getTxBufferData() + sizeof(ether_header));
    icmphdr *icmpHeader = reinterpret_cast<icmphdr *>(getTxBufferData() + sizeof(ether_header) + sizeof(iphdr));
    ipHeader->id = static_cast<uint16_t> (17767);
    initTxBufferSentinel();
    EXPECT_TRUE(ipHeader->check == 62919);
    EXPECT_TRUE(icmpHeader->checksum == 12100);

    initTxBufferTlvSendSwitch();
    EXPECT_TRUE(ipHeader->check == 61895);
    EXPECT_TRUE(icmpHeader->checksum == 11838);

    initTxBufferSentinel();
    EXPECT_TRUE(ipHeader->check == 62919);
    EXPECT_TRUE(icmpHeader->checksum == 12100);
}

TEST_F(LinkProberTest, handleSendProbeCommand)
{
    initializeSendBuffer();

    iphdr *ipHeader = reinterpret_cast<iphdr *>(getTxBufferData() + sizeof(ether_header));
    icmphdr *icmpHeader = reinterpret_cast<icmphdr *>(getTxBufferData() + sizeof(ether_header) + sizeof(iphdr));
    ipHeader->id = static_cast<uint16_t> (17767);
    initTxBufferSentinel();
    EXPECT_TRUE(ipHeader->check == 62919);
    EXPECT_TRUE(icmpHeader->checksum == 12100);

    initTxBufferTlvSendProbe();
    EXPECT_TRUE(ipHeader->check == 61895);
    EXPECT_TRUE(icmpHeader->checksum == 11582);

    initTxBufferSentinel();
    EXPECT_TRUE(ipHeader->check == 62919);
    EXPECT_TRUE(icmpHeader->checksum == 12100);
}

TEST_F(LinkProberTest, UpdateEthernetFrame)
{
    link_prober::IcmpPayload *icmpPayload = new (
        getTxBuffer().data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr)
    ) link_prober::IcmpPayload();
    boost::uuids::uuid guid = boost::lexical_cast<boost::uuids::uuid> ("44f49d86-c312-414b-b6a1-be82901ac459");
    memcpy(icmpPayload->uuid, guid.data, sizeof(icmpPayload->uuid));
    handleUpdateEthernetFrame();

    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (getTxBuffer().data() + sizeof(ether_header) + sizeof(iphdr));
    EXPECT_TRUE(icmpHeader->checksum == 12100);
}

TEST_F(LinkProberTest, UpdateSequenceNo)
{
    link_prober::IcmpPayload *icmpPayload = new (
        getTxBuffer().data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr)
    ) link_prober::IcmpPayload();
    boost::uuids::uuid guid = boost::lexical_cast<boost::uuids::uuid> ("44f49d86-c312-414b-b6a1-be82901ac459");
    memcpy(icmpPayload->uuid, guid.data, sizeof(icmpPayload->uuid));

    handleUpdateEthernetFrame();

    // update sequence number twice as we start with 0xffff
    handleUpdateSequenceNumber();
    handleUpdateSequenceNumber();

    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (getTxBuffer().data() + sizeof(ether_header) + sizeof(iphdr));
    EXPECT_TRUE(icmpHeader->checksum == 11844);

    EXPECT_TRUE(getRxSelfSeqNo() + 1 == ntohs(icmpHeader->un.echo.sequence));
    EXPECT_TRUE(getRxPeerSeqNo() + 1 == ntohs(icmpHeader->un.echo.sequence));

    // sequence number should still be updated when heartbeat is suspended
    EXPECT_EQ(getRxSelfSeqNo(), 0);
    EXPECT_EQ(getRxPeerSeqNo(), 0);

    handleSuspendTxProbes();
    EXPECT_TRUE(getSuspendTx());
    
    handleSendHeartbeat();
    EXPECT_EQ(getRxSelfSeqNo(), 1);
    EXPECT_EQ(getRxPeerSeqNo(), 1);
}

TEST_F(LinkProberTest, GenerateGuid)
{
    link_prober::IcmpPayload::generateGuid();
    initializeSendBuffer();

    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> txBuffer = getTxBuffer();
    link_prober::IcmpPayload *icmpPayload = new (
        txBuffer.data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr)
    ) link_prober::IcmpPayload();
    EXPECT_TRUE(memcmp(
        icmpPayload->uuid,
        link_prober::IcmpPayload::getGuidData(),
        sizeof(icmpPayload->uuid)
    ) == 0);
}

TEST_F(LinkProberTest, UpdateToRMac)
{
    link_prober::IcmpPayload::generateGuid();

    std::array<uint8_t, ETHER_ADDR_LEN> torMac = {0, 'b', 2, 'd', 4, 'f'};
    mMuxConfig.setTorMacAddress(torMac);

    mMuxConfig.setIfUseTorMacAsSrcMac(true);

    boost::asio::ip::address ipAddress = boost::asio::ip::address::from_string("192.168.1.100");
    mFakeMuxPort.setServerIpv4Address(ipAddress);

    initializeSendBuffer();

    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> txBuffer = getTxBuffer();
    ether_header *ethHeader = reinterpret_cast<ether_header *> (txBuffer.data());

    EXPECT_TRUE(ethHeader->ether_shost[0] == torMac[0]);
    EXPECT_TRUE(ethHeader->ether_shost[1] == torMac[1]);
    EXPECT_TRUE(ethHeader->ether_shost[2] == torMac[2]);
    EXPECT_TRUE(ethHeader->ether_shost[3] == torMac[3]);
    EXPECT_TRUE(ethHeader->ether_shost[4] == torMac[4]);
    EXPECT_TRUE(ethHeader->ether_shost[5] == torMac[5]);

    iphdr *ipHeader = reinterpret_cast<iphdr *> (txBuffer.data() + sizeof(ether_header));

    EXPECT_TRUE(ipHeader->daddr == htonl(ipAddress.to_v4().to_uint()));
}

TEST_F(LinkProberTest, ReadWriteTlv)
{
    initializeSendBuffer();
    size_t tlvStartOffset = sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr) + sizeof(link_prober::IcmpPayload);
    // check initial tx buffer packet size
    EXPECT_TRUE(getTxPacketSize() == tlvStartOffset + sizeof(link_prober::TlvHead));

    // build txBuffer
    resetTxBufferTlv();
    size_t tlvCommandSize = appendTlvCommand(link_prober::Command::COMMAND_SWITCH_ACTIVE);
    EXPECT_TRUE(tlvCommandSize == (sizeof(link_prober::TlvHead) + sizeof(link_prober::Command)));
    size_t tlvSentinelSize = appendTlvSentinel();
    EXPECT_TRUE(tlvSentinelSize == sizeof(link_prober::TlvHead));
    EXPECT_TRUE(getTxPacketSize() == tlvStartOffset + tlvCommandSize + tlvSentinelSize);

    // build rxBuffer
    size_t bytesTransferred = getTxPacketSize();
    memcpy(getRxBufferData(), getTxBufferData(), bytesTransferred);

    // start read TLV from rxBuffer
    size_t rxReadOffset = tlvStartOffset;
    size_t tlvSize = findNextTlv(rxReadOffset, bytesTransferred);
    link_prober::Tlv *tlvPtr = reinterpret_cast<link_prober::Tlv *> (getRxBufferData() + rxReadOffset);
    EXPECT_TRUE(tlvSize == tlvCommandSize);
    EXPECT_TRUE(tlvPtr->tlvhead.type == link_prober::TlvType::TLV_COMMAND);
    EXPECT_TRUE(tlvPtr->tlvhead.length == htons(1));
    EXPECT_TRUE(tlvPtr->command == static_cast<uint8_t> (link_prober::Command::COMMAND_SWITCH_ACTIVE));
    rxReadOffset += tlvSize;

    tlvSize = findNextTlv(rxReadOffset, bytesTransferred);
    tlvPtr = reinterpret_cast<link_prober::Tlv *> (getRxBufferData() + rxReadOffset);
    EXPECT_TRUE(tlvSize == tlvSentinelSize);
    EXPECT_TRUE(tlvPtr->tlvhead.type == link_prober::TlvType::TLV_SENTINEL);
    EXPECT_TRUE(tlvPtr->tlvhead.length == 0);
    rxReadOffset += tlvSize;

    tlvSize = findNextTlv(rxReadOffset, bytesTransferred);
    EXPECT_TRUE(tlvSize == 0);
}

TEST_F(LinkProberTest, ReadWriteVariableSizedTlv)
{
    initializeSendBuffer();
    size_t tlvStartOffset = sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr) + sizeof(link_prober::IcmpPayload);
    // check initial tx buffer packet size
    EXPECT_TRUE(getTxPacketSize() == tlvStartOffset + sizeof(link_prober::TlvHead));

    // build txBuffer
    resetTxBufferTlv();
    std::vector<size_t> paddingSizes{0, 1, 2, 3};
    for (const size_t &paddingSize : paddingSizes) {
        size_t tlvSize = appendTlvDummy(paddingSize, paddingSize);
        EXPECT_TRUE(tlvSize == (sizeof(link_prober::TlvHead) + sizeof(uint32_t) + paddingSize));
    }

    // build rxBuffer
    size_t bytesTransferred = getTxPacketSize();
    memcpy(getRxBufferData(), getTxBufferData(), bytesTransferred);

    // start read TLV from rxBuffer
    size_t rxReadOffset = tlvStartOffset;
    for (const size_t &paddingSize : paddingSizes) {
        size_t tlvSize = findNextTlv(rxReadOffset, bytesTransferred);
        link_prober::Tlv *tlvPtr = reinterpret_cast<link_prober::Tlv *> (getRxBufferData() + rxReadOffset);
        EXPECT_TRUE(tlvSize == (sizeof(link_prober::TlvHead) + sizeof(uint32_t) + paddingSize));

        uint32_t *seqNoPtr = reinterpret_cast<uint32_t *> (tlvPtr->data + paddingSize);
        EXPECT_TRUE(ntohl(*seqNoPtr) == paddingSize);
        rxReadOffset += tlvSize;
    }

    EXPECT_TRUE(findNextTlv(rxReadOffset, bytesTransferred) == 0);
}

TEST_F(LinkProberTest, InitializeException)
{
    EXPECT_THROW(initialize(), common::SocketErrorException);
}

} /* namespace test */
