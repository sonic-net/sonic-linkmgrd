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

#ifndef MOCKLINKPROBERTEST_H_
#define MOCKLINKPROBERTEST_H_

#include <boost/uuid/uuid.hpp>

#include "common/MuxPortConfig.h"
#include "link_prober/LinkProberSw.h"
#include "link_prober/LinkProberStateMachineActiveStandby.h"
#include "link_prober/LinkProberStateMachineActiveActive.h"

#include "MockLinkManagerStateMachine.h"

namespace test
{
class LinkProberMockTest : public ::testing::Test
{
public:
    LinkProberMockTest();
    virtual ~LinkProberMockTest() = default;
    void SetUp(common::MuxPortConfig::PortCableType portCableType);
    void TearDown() override;
    void runIoService(uint32_t count = 0);
    void initializeSendBuffer() { mLinkProberPtr->initializeSendBuffer(); }
    void updateIcmpSequenceNo() { mLinkProberPtr->updateIcmpSequenceNo(); }
    void sendHeartbeat() { mLinkProberPtr->updateIcmpSequenceNo(); }
    boost::asio::posix::stream_descriptor& getStream() const { return mLinkProberPtr->mStream; }
    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE>& getTxBuffer() { return mLinkProberPtr->mTxBuffer; }
    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE>& getRxBuffer() { return mLinkProberPtr->mRxBuffer; }
    std::size_t getTxPacketSize() { return mLinkProberPtr->mTxPacketSize; }
    const std::size_t getPacketHeaderSize() { return mLinkProberPtr->mPacketHeaderSize; }
    std::shared_ptr<MockLinkManagerStateMachine> getLinkManagerStateMachinePtr() { return mLinkManagerStateMachinePtr; }
    std::shared_ptr<link_prober::LinkProberStateMachineBase> getLinkProberStateMachinePtr() { return mLinkProberStateMachinePtr; }
    std::shared_ptr<link_prober::LinkProberSw> getLinkProberPtr() { return mLinkProberPtr; }
    void computeChecksum(icmphdr* icmpHeader, size_t size) { mLinkProberPtr->computeChecksum(icmpHeader, size); }
    void handleRecv() { mLinkProberPtr->handleRecv(boost::system::error_code(), getTxPacketSize()); 
    }
    void handleTimeout() { mLinkProberPtr->mReportHeartbeatReplyNotReceivedFuncPtr(link_prober::HeartbeatType::HEARTBEAT_SELF); }
    void receiveSelfIcmpReply();
    void receivePeerIcmpReply();
    void postGenerateGuid(uint32_t count);
    boost::asio::io_service &getIoService() {  return mIoService; }

private:
    void buildIcmpReply();

private:
    std::string mPortName = "Ethernet4";
    uint16_t mServerId = 4;
    common::MuxConfig mMuxConfig;
    common::MuxPortConfig mMuxPortConfig;
    boost::asio::io_service mIoService;
    boost::asio::io_service::strand mStrand;
    std::shared_ptr<MockLinkManagerStateMachine> mLinkManagerStateMachinePtr;
    std::shared_ptr<link_prober::LinkProberStateMachineBase> mLinkProberStateMachinePtr;
    std::shared_ptr<link_prober::LinkProberSw> mLinkProberPtr;
    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> mBuffer;
    boost::uuids::uuid mPeerGuid;
};
} // namespace test

#endif
