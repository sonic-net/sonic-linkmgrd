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
 * LinkProberHardwareTest.h
 *
 *  Created on: May 12, 2025
 *      Author: Harjot SIngh
 */

#ifndef LINKPROBERHARDWARETEST_H_
#define LINKPROBERHARDWARETEST_H_

#include "gtest/gtest.h"

#include "FakeMuxPort.h"
#include "link_prober/LinkProberHw.h"
#include "common/MuxLogger.h"
namespace test
{

class LinkProberHardwareTest: public ::testing::Test
{
public:
    LinkProberHardwareTest();
    virtual ~LinkProberHardwareTest() = default;

    boost::asio::io_service mIoService;
    common::MuxConfig mMuxConfig;
    std::shared_ptr<FakeDbInterface> mDbInterfacePtr;
    std::string mPortName = "EtherTest01";
    std::string mSmartNicIpAddress = "192.168.1.20";
    uint16_t mServerId = 01;
    boost::uuids::uuid mPeerGuid;

    FakeMuxPort mFakeMuxPort;
    //MuxPort mMuxPort;
    //link_prober::LinkProberStateMachineActiveActive mLinkProberStateMachine;
    link_prober::LinkProberHw mLinkProber;
    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> mBuffer;

    void runIoService(uint32_t count);
    bool getSuspendTx() {return mLinkProber.mSuspendTx;};
    std::size_t getTxPacketSize() { return mLinkProber.mTxPacketSize; }
    const std::size_t getPacketHeaderSize() { return mLinkProber.mPacketHeaderSize; }
    void handleRecv() { mLinkProber.handleRecv(boost::system::error_code(), getTxPacketSize()); }
    void computeChecksum(icmphdr* icmpHeader, size_t size) { mLinkProber.computeChecksum(icmpHeader, size); }
    link_manager::ActiveActiveStateMachine::CompositeState mTestCompositeState;

    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE>& getTxBuffer() { return mLinkProber.mTxBuffer; }
    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE>& getRxBuffer() { return mLinkProber.mRxBuffer; }
    void receivePeerSoftwareIcmpReply();
    void receivePeerHardwareIcmpReply();
    void changePeerGuid();
    void buildIcmpReply();
    void TearDown();
};

} /* namespace test */

#endif /* LINKPROBERHARDWARETEST_H_ */
