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
#include <boost/bind.hpp>
#include <boost/thread.hpp>
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
    : mMuxPortConfig(mMuxConfig, mPortName, mServerId, common::MuxPortConfig::PortCableType::DefaultType),
      mStrand(mIoService)
{
    boost::uuids::random_generator gen;
    mPeerGuid = gen();

    mMuxConfig.setNegativeStateChangeRetryCount(1);
}

void LinkProberMockTest::SetUp(common::MuxPortConfig::PortCableType portCableType)
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
    mLinkProberPtr = std::make_shared<link_prober::LinkProberSw>(
        mMuxPortConfig,
        mIoService,
        mLinkProberStateMachinePtr.get()
    );
    //link_prober::IcmpPayload::generateGuid();
    initializeSendBuffer();
}

void LinkProberMockTest::TearDown()
{
    // mLinkProberPtr.reset();
    // mLinkProberStateMachinePtr.reset();
    // mLinkManagerStateMachinePtr.reset();
    Mock::VerifyAndClearExpectations(mLinkManagerStateMachinePtr.get());

    link_prober::LinkProberBase::mGuidSet.clear();
    mIoService.stop();
}

void LinkProberMockTest::postGenerateGuid(uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i)
    {
        boost::asio::post(mIoService, boost::bind(
            &link_prober::LinkProberBase::generateGuid,
            mLinkProberPtr.get()
        ));
    }
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

TEST_F(LinkProberMockTest, GuidGenRaceCondition)
{
    SetUp(common::MuxPortConfig::PortCableType::ActiveStandby);

    std::unique_ptr<boost::asio::io_service::work> work(new boost::asio::io_service::work(getIoService()));

    boost::thread_group threads;
    for (size_t i = 0; i < 8; ++i)
    {
        threads.create_thread(boost::bind(&boost::asio::io_service::run, &(getIoService())));
    }

    postGenerateGuid(10000);

    for (size_t i = 0; i < 100; ++i)
    {
        if (link_prober::LinkProberBase::mGuidSet.size() == 10001)
        {
            break;
        }
        sleep(1);
    }

    EXPECT_EQ(10001, link_prober::LinkProberBase::mGuidSet.size());

    work.reset();
    threads.join_all();

    TearDown();
}

} // namespace test
