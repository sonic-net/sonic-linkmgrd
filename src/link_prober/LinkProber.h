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
 * LinkProber.h
 *
 *  Created on: Oct 4, 2020
 *      Author: tamer
 */

#ifndef LINKPROBER_H_
#define LINKPROBER_H_

#include <memory>
#include <stdint.h>
#include <vector>
#include <linux/filter.h>

#include <boost/asio.hpp>

#include "IcmpPayload.h"
#include "LinkProberStateMachine.h"
#include "common/MuxPortConfig.h"

namespace test {
class LinkProberTest;
}

namespace link_prober
{
using SockFilter = struct sock_filter;
using SockFilterProg = struct sock_fprog;
using SockAddrLinkLayer = struct sockaddr_ll;

/**
 *@class LinkProber
 *
 *@brief probes the server sing ICMP ECHPREQUEST packet. The packet payload
 *       holds GUID that identifies this ToR. Reception of this ToR's GUID
 *       indicate that the link is in Active state. Reception of unknown
 *       GUID will indicate standby state. Lack of ICMP packets will signal
 *       that the link state is unknown.
 */
class LinkProber
{
public:
    /**
    *@method LinkProber
    *
    *@brief class default constructor
    */
    LinkProber() = delete;

    /**
    *@method LinkProber
    *
    *@brief class copy constructor
    *
    *@param LinkProber (in)  reference to LinkProber object to be copied
    */
    LinkProber(const LinkProber &) = delete;

    /**
    *@method LinkProber
    *
    *@brief class constructor
    *
    *@param muxPortConfig (in)          reference to MuxPortConfig object
    *@param ioService (in)              reference to boost io_service object
    *@param linkProberStateMachine (in) reference to LinkProberStateMachine object
    */
    LinkProber(
        common::MuxPortConfig &muxPortConfig,
        boost::asio::io_service &ioService,
        LinkProberStateMachine &linkProberStateMachine
    );

    /**
    *@method ~LinkProber
    *
    *@brief class destructor
    */
    virtual ~LinkProber() = default;

    /**
    *@method initialize
    *
    *@brief initialize link prober sockets and builds ICMP packet
    *
    *@return none
    */
    void initialize();

    /**
    *@method startProbing
    *
    *@brief start probing server/blade using ICMP ECHOREQUEST
    *
    *@return none
    */
    void startProbing();

    /**
    *@method suspendTxProbes
    *
    *@brief suspend sending ICMP ECHOREQUEST packets
    *
    *@param suspendTime_msec suspension time in msec
    *
    *@return none
    */
    void suspendTxProbes(uint32_t suspendTime_msec);

    /**
    *@method resumeTxProbes
    *
    *@brief resume sending ICMP ECHOREQUEST packets
    *
    *@return none
    */
    void resumeTxProbes();

    /**
    *@method updateEthernetFrame
    *
    *@brief update Ethernet frame of Tx Buffer
    *
    *@return none
    */
    void updateEthernetFrame();

    /**
    *@method probePeerTor
    *
    *@brief send an early HB to peer ToR
    *
    *@return none
    */
    void probePeerTor();

    /**
    *@method sendPeerSwitchCommand
    *
    *@brief send switch command to peer ToR
    *
    *@return none
    */
    void sendPeerSwitchCommand();

private:
    /**
    *@method handleUpdateEthernetFrame
    *
    *@brief update Ethernet frame of Tx Buffer
    *
    *@return none
    */
    void handleUpdateEthernetFrame();

    /**
    *@method handleSendSwitchCommand
    *
    *@brief send switch command to peer ToR
    *
    *@return none
    */
    void handleSendSwitchCommand();

    /**
    *@method sendHeartbeat
    *
    *@brief send ICMP ECHOREQUEST packet
    *
    *@return none
    */
    void sendHeartbeat();

    /**
    *@method handleRecv
    *
    *@brief handle packet reception
    *
    *@param errorCode (in)          socket error code
    *@param bytesTransferred (in)   number of bytes received
    *
    *@return none
    */
    void handleRecv(
        const boost::system::error_code &errorCode,
        size_t bytesTransferred
    );

    /**
    *@method handleInitRecv
    *
    *@brief handle packet reception
    *
    *@param errorCode (in)          socket error code
    *@param bytesTransferred (in)   number of bytes received
    *
    *@return none
    */
    void handleInitRecv(
        const boost::system::error_code &errorCode,
        size_t bytesTransferred
    );

    /**
    *@method handleTimeout
    *
    *@brief handle ICMP packet reception timeout
    *
    *@param errorCode (in)          socket error code
    *
    *@return none
    */
    void handleTimeout(boost::system::error_code errorCode);

    /**
    *@method handleSuspendTimeout
    *
    *@brief handle suspend timer timeout
    *
    *@param errorCode (in)          socket error code
    *
    *@return none
    */
    void handleSuspendTimeout(boost::system::error_code errorCode);

    /**
    *@method startRecv
    *
    *@brief start ICMP ECHOREPLY reception
    *
    *@return none
    */
    void startRecv();

    /**
    *@method startInitRecv
    *
    *@brief start ICMP ECHOREPLY reception
    *
    *@return none
    */
    void startInitRecv();

    /**
    *@method startTimer
    *
    *@brief start ICMP ECHOREPLY timeout timer
    *
    *@return none
    */
    void startTimer();

    /**
    *@method calculateChecksum
    *
    *@brief calculate ICMP payload checksum
    *
    *@param data (in)   pointer to data buffer
    *@param size (in)   size of data buffer
    *
    *@return CRC checksum
    */
    uint32_t calculateChecksum(uint16_t *data, size_t size);

    /**
    *@method addChecksumCarryover
    *
    *@brief add checksum carryover
    *
    *@param checksum (out)  pointer to checksum field
    *@param sum (in)        current sum of the buffer payload
    *
    *@return CRC checksum
    */
    void addChecksumCarryover(uint16_t *checksum, uint32_t sum);

    /**
    *@method computeChecksum
    *
    *@brief compute ICMP checksum
    *
    *@param icmpHeader (in, out)    pointer ICMP header
    *@param size (in)               size of ICMP payload
    *
    *@return CRC checksum
    */
    void computeChecksum(icmphdr *icmpHeader, size_t size);

    /**
    *@method computeChecksum
    *
    *@brief compute IPv4 checksum
    *
    *@param ipHeader (in, out)  pointer IPv4 header
    *@param size (in)           size of IPv4 header
    *
    *@return CRC checksum
    */
    void computeChecksum(iphdr *ipHeader, size_t size);

    /**
    *@method initializeSendBuffer
    *
    *@brief initialize ICMP packet once
    *
    *@return CRC checksum
    */
    void initializeSendBuffer();

    /**
    *@method updateIcmpSequenceNo
    *
    *@brief update ICMP packet checksum, used before sending new heartbeat
    *
    *@return CRC checksum
    */
    void updateIcmpSequenceNo();

    /**
    *@method getTxBuffer
    *
    *@brief getter for TxBuffer used for testing
    *
    *@return CRC checksum
    */
    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> getTxBuffer() {return mTxBuffer;};

    friend class test::LinkProberTest;

private:
    static SockFilter mIcmpFilter[];

private:
    common::MuxPortConfig &mMuxPortConfig;
    boost::asio::io_service &mIoService;
    LinkProberStateMachine &mLinkProberStateMachine;

    uint16_t mTxSeqNo = 0xffff;
    uint16_t mRxSelfSeqNo = 0;
    uint16_t mRxPeerSeqNo = 0;

    uint32_t mIcmpChecksum = 0;
    uint32_t mIpChecksum = 0;

    boost::asio::io_service::strand mStrand;
    boost::asio::deadline_timer mDeadlineTimer;
    boost::asio::deadline_timer mSuspendTimer;
    boost::asio::posix::stream_descriptor mStream;

    std::shared_ptr<SockFilter> mSockFilterPtr;
    SockFilterProg mSockFilterProg;

    int mSocket = 0;

    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> mTxBuffer;
    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> mRxBuffer;

    bool mSuspendTx = false;
};

} /* namespace link_prober */

#endif /* LINKPROBER_H_ */
