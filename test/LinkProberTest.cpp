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
        mFakeMuxPort.getLinkProberStateMachine()
    )
{
    mMuxConfig.setTimeoutIpv4_msec(1);
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
    EXPECT_TRUE(ipHeader->ihl == sizeof(iphdr) >> 2);
    EXPECT_TRUE(ipHeader->version == IPVERSION);
    EXPECT_TRUE(ipHeader->tos == 0xb8);
    EXPECT_TRUE(ipHeader->tot_len == htons(sizeof(iphdr) + sizeof(icmphdr) + sizeof(link_prober::IcmpPayload)));
    EXPECT_TRUE(ipHeader->frag_off == 0);
    EXPECT_TRUE(ipHeader->ttl == 64);
    EXPECT_TRUE(ipHeader->protocol == IPPROTO_ICMP);
    EXPECT_TRUE(ipHeader->check == 62663);
    EXPECT_TRUE(ipHeader->saddr == htonl(mFakeMuxPort.getMuxPortConfig().getLoopbackIpv4Address().to_v4().to_uint()));
    EXPECT_TRUE(ipHeader->daddr == htonl(mFakeMuxPort.getMuxPortConfig().getBladeIpv4Address().to_v4().to_uint()));

    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (txBuffer.data() + sizeof(ether_header) + sizeof(iphdr));
    EXPECT_TRUE(icmpHeader->type == ICMP_ECHO);
    EXPECT_TRUE(icmpHeader->code == 0);
    EXPECT_TRUE(icmpHeader->un.echo.id == htons(mFakeMuxPort.getMuxPortConfig().getServerId()));
    EXPECT_TRUE(icmpHeader->un.echo.sequence == htons(0xffff));

    link_prober::IcmpPayload *icmpPayload = new (
        txBuffer.data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr)
    ) link_prober::IcmpPayload();

    EXPECT_TRUE(icmpPayload->cookie == htonl(link_prober::IcmpPayload::getCookie()));
    EXPECT_TRUE(icmpPayload->version == htonl(link_prober::IcmpPayload::getVersion()));
    EXPECT_TRUE(memcmp(
        icmpPayload->un.uuid.data,
        link_prober::IcmpPayload::getGuidData(),
        link_prober::IcmpPayload::getGuid().size()
    ) == 0);
}

TEST_F(LinkProberTest, CalculateChecksum)
{
    link_prober::IcmpPayload *icmpPayload = new (
        getTxBuffer().data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr)
    ) link_prober::IcmpPayload();
    boost::uuids::uuid guid = boost::lexical_cast<boost::uuids::uuid> ("44f49d86-c312-414b-b6a1-be82901ac459");
    memcpy(icmpPayload->un.uuid.data, guid.data, guid.size());
    initializeSendBuffer();

    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (getTxBuffer().data() + sizeof(ether_header) + sizeof(iphdr));
    EXPECT_TRUE(icmpHeader->checksum == 12355);
}

TEST_F(LinkProberTest, UpdateEthernetFrame)
{
    link_prober::IcmpPayload *icmpPayload = new (
        getTxBuffer().data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr)
    ) link_prober::IcmpPayload();
    boost::uuids::uuid guid = boost::lexical_cast<boost::uuids::uuid> ("44f49d86-c312-414b-b6a1-be82901ac459");
    memcpy(icmpPayload->un.uuid.data, guid.data, guid.size());
    handleUpdateEthernetFrame();

    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (getTxBuffer().data() + sizeof(ether_header) + sizeof(iphdr));
    EXPECT_TRUE(icmpHeader->checksum == 12355);
}

TEST_F(LinkProberTest, UpdateSequenceNo)
{
    link_prober::IcmpPayload *icmpPayload = new (
        getTxBuffer().data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr)
    ) link_prober::IcmpPayload();
    boost::uuids::uuid guid = boost::lexical_cast<boost::uuids::uuid> ("44f49d86-c312-414b-b6a1-be82901ac459");
    memcpy(icmpPayload->un.uuid.data, guid.data, guid.size());

    handleUpdateEthernetFrame();

    // update sequence number twice as we start with 0xffff
    handleUpdateSequenceNumber();
    handleUpdateSequenceNumber();

    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (getTxBuffer().data() + sizeof(ether_header) + sizeof(iphdr));
    EXPECT_TRUE(icmpHeader->checksum == 12099);

    EXPECT_TRUE(getRxSelfSeqNo() + 1 == ntohs(icmpHeader->un.echo.sequence));
    EXPECT_TRUE(getRxPeerSeqNo() + 1 == ntohs(icmpHeader->un.echo.sequence));
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
        icmpPayload->un.uuid.data,
        link_prober::IcmpPayload::getGuidData(),
        link_prober::IcmpPayload::getGuid().size()
    ) == 0);
}

TEST_F(LinkProberTest, UpdateToRMac)
{
    link_prober::IcmpPayload::generateGuid();

    std::array<uint8_t, ETHER_ADDR_LEN> torMac = {0, 'b', 2, 'd', 4, 'f'};
    mMuxConfig.setTorMacAddress(torMac);

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

TEST_F(LinkProberTest, InitializeException)
{
    EXPECT_THROW(initialize(), common::SocketErrorException);
}

} /* namespace test */
