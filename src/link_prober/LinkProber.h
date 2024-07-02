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

#include <common/BoostAsioBehavior.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>

#include "IcmpPayload.h"
#include "LinkProberStateMachineActiveActive.h"
#include "LinkProberStateMachineActiveStandby.h"
#include "LinkProberSessionStateMachine.h"
#include "common/MuxPortConfig.h"

namespace test {
class LinkProberTest;
class LinkProberMockTest;
}

namespace link_prober
{
using SockFilter = struct sock_filter;
using SockFilterProg = struct sock_fprog;
using SockAddrLinkLayer = struct sockaddr_ll;

/**
 *@enum HeartbeatType
 *
 *@brief Received heartbeat type
 */
enum class HeartbeatType: uint8_t {
    HEARTBEAT_SELF,
    HEARTBEAT_PEER,

    Count
};

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
    *@param linkProberStateMachinePtr (in) reference to LinkProberStateMachineBase object
    *@param linkProberSessionStateMachinePtr (in) reference to LinkProberSessionStateMachine object
    */
    LinkProber(
        common::MuxPortConfig &muxPortConfig,
        boost::asio::io_service &ioService,
        LinkProberStateMachineBase *linkProberStateMachinePtr,
        LinkProberSessionStateMachine *linkProberSessionStateMachinePtr
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
    *@method detectLink
    *
    *@brief detect link status
    *
    *@return none
    */
    void detectLink();

    /**
    *@method sendPeerSwitchCommand
    *
    *@brief send switch command to peer ToR
    *
    *@return none
    */
    void sendPeerSwitchCommand();

    /**
    *@method sendPeerProbeCommand
    *
    *@brief send probe command to peer ToR
    *
    *@return none
    */
    void sendPeerProbeCommand();

    /**
     * @method resetIcmpPacketCounts()
     * 
     * @brief reset Icmp packet counts, post a pck loss ratio update immediately 
     * 
     * @return none
    */
    void resetIcmpPacketCounts();

    /**
     * @method shutdownTxProbes
     * 
     * @brief stop sending ICMP ECHOREQUEST packets indefinitely.
     * 
     * @return none
     */
    void shutdownTxProbes();

    /**
     * @method restartTxProbes
     * 
     * @brief restart sending ICMP ECHOREQUEST packets
     * 
     * @return none
     */
    void restartTxProbes();

    /**
     * @method decreaseProbeIntervalAfterSwitch
     *  
     * @brief adjust link prober interval to 10 ms after switchover to better measure the switchover overhead.
     * 
     * @param switchTime_msec (in) switchover is expected to complete  within this time window
     * @param expectingLinkProberEvent (in) depends on which state LinkManager is switching to, link prober expects self or peer events
     * 
     * @return none
     */
    void decreaseProbeIntervalAfterSwitch(uint32_t switchTime_msec);

    /**
     * @method revertProbeIntervalAfterSwitchComplete 
     * 
     * @brief revert probe interval change after switchover is completed
     * 
     * @return none
     */
    void revertProbeIntervalAfterSwitchComplete();

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
    *@method handleSendProbeCommand
    *
    *@brief send probe command to peer ToR
    *
    *@return none
    */
    void handleSendProbeCommand();

    /**
    *@method sendHeartbeat
    *
    *@brief send ICMP ECHOREQUEST packet
    *
    *@param forceSend (in)  Force sending heartbeat, used in link detect only
    *
    *@return none
    */
    void sendHeartbeat(bool forceSend = false);

    /**
    *@method handleTlvCommandRecv
    *
    *@brief handle TLV command
    *
    *@param tlvPtr (in)     Tlv ptr points to the start of TlvCommand in mRxBuffer
    *@param isPeer (in)     True if the reply received is from the peer ToR
    *
    *@return none
    */
    void handleTlvCommandRecv(
        Tlv *tlvPtr,
        bool isPeer
    );

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
    *@return tx buffer
    */
    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> getTxBuffer() {return mTxBuffer;};

    /**
    *@method findNextTlv
    *
    *@brief Find next TLV in rxBuffer starting at readOffset
    *
    *@param readOffset (in)         starting offset to read
    *@param bytesTransferred (in)   total bytes received in rxBuffer
    *
    *@return the next TLV size
    */
    size_t findNextTlv(size_t readOffset, size_t bytesTransferred);

    void resetTxBufferTlv() {mTxPacketSize = mTlvStartOffset;};

    /**
    *@method appendTlvCommand
    *
    *@brief append TlvCommand to txBuffer
    *
    *@param commandType (in)    command type
    *
    *@return the appended TLV size
    */
    size_t appendTlvCommand(Command commandType = Command::COMMAND_SWITCH_ACTIVE);

    /**
    *@method appendTlvSentinel
    *
    *@brief append TlvSentinel to txBuffer
    *
    *@return the appended TLV size
    */
    size_t appendTlvSentinel();

    /**
    *@method appendTlvDummy
    *
    *@brief append dummy TLV, test purpose only
    *
    *@return the appended TLV size
    */
    size_t appendTlvDummy(size_t paddingSize, int seqNo);

    /**
     * @method initTxBufferTlvSendSwitch
     * 
     * @brief initialize TX buffer TLVs to send switch command to peer
     * 
     * @return none
     */
    void initTxBufferTlvSendSwitch();

    /**
     * @method initTxBufferTlvSendProbe
     *
     * @brief initialize TX buffer TLVs to send probe command to peer
     *
     * @return none
     */
    void initTxBufferTlvSendProbe();

    /**
     * @method initTxBufferTlvSentinel
     * 
     * @brief initialize TX buffer to have only TLV sentinel
     * 
     * @return none
     */
    void initTxBufferTlvSentinel();

    /**
     * @method calculateChecksum
     * 
     * @brief calculate TX packet checksums in both IP header and ICMP header
     * 
     * @return none
     */
    inline void calculateTxPacketChecksum();

    /**
     * @method getProbingInterval
     * 
     * @brief get link prober interval
     * 
     * @return link prober interval
     */
    inline uint32_t getProbingInterval(); 

    /**
     * @method handleSwitchoverTimeout
     * 
     * @brief handle switchover time out 
     * 
     * @param errorCode (in) socket error code 
     * 
     * @return none
     */
    void handleSwitchoverTimeout(boost::system::error_code errorCode);

    friend class test::LinkProberTest;
    friend class test::LinkProberMockTest;

private:
    static SockFilter mIcmpFilter[];

private:
    /**
     * @method reportHeartbeatReplyReceivedActiveStandby
     * 
     * @brief report heartbeat reply received to active-standby mode link prober state machine
     * 
     * @return none
     */
    void reportHeartbeatReplyReceivedActiveStandby(HeartbeatType heartbeatType);

    /**
     *@method reportHeartbeatReplyReceivedActiveActive
     *
     *@brief report heartbeat reply received to active-active mode link prober state machine
     *
     *@param heartbeatType (in) received heartbeat type
     *
     *@return none
     */
    void reportHeartbeatReplyReceivedActiveActive(HeartbeatType heartbeatType);

    /**
     *@method reportHeartbeatReplyReceivedSession
     *
     *@brief report heartbeat reply received to link prober session state machine
     *
     *@param heartbeatType (in) received heartbeat type
     *
     * @return none
     */
    void reportHeartbeatReplyReceivedSession(HeartbeatType heartbeatType);

    /**
     * @method reportHeartbeatReplyNotReceivedActiveStandby
     * 
     * @brief report heartbeat reply not received to active-standby mode link prober state machine
     * 
     * @param heartbeatType (in) received heartbeat type
     *
     * @return none
     */
    void reportHeartbeatReplyNotReceivedActiveStandby();

    /**
     * @method reportHeartbeatReplyNotReceivedActiveActive
     * 
     * @brief report heartbeat reply not received to active-active mode link prober state machine
     * 
     * @return none
     */
    void reportHeartbeatReplyNotReceivedActiveActive();

    /**
     * @method reportHeartbeatReplyNotReceivedSession
     * 
     * @brief report heartbeat reply not received to link prober session state machine
     * 
     * @return none
     */
    void reportHeartbeatReplyNotReceivedSession();

private:
    common::MuxPortConfig &mMuxPortConfig;
    boost::asio::io_service &mIoService;
    LinkProberStateMachineBase *mLinkProberStateMachinePtr;
    LinkProberSessionStateMachine *mLinkProberSessionStateMachinePtr;

    boost::function<void (HeartbeatType heartbeatType)> mReportHeartbeatReplyReceivedFuncPtr;
    boost::function<void ()> mReportHeartbeatReplyNotRecivedFuncPtr;

    uint16_t mTxSeqNo = 0xffff;
    uint16_t mRxSelfSeqNo = 0;
    uint16_t mRxPeerSeqNo = 0;

    uint32_t mIcmpChecksum = 0;
    uint32_t mIpChecksum = 0;

    static const size_t mPacketHeaderSize = sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr);
    static const size_t mTlvStartOffset = sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr) + sizeof(IcmpPayload);

    boost::asio::io_service::strand mStrand;
    boost::asio::deadline_timer mDeadlineTimer;
    boost::asio::deadline_timer mSuspendTimer;
    boost::asio::deadline_timer mSwitchoverTimer;
    boost::asio::posix::stream_descriptor mStream;

    std::shared_ptr<SockFilter> mSockFilterPtr;
    SockFilterProg mSockFilterProg;

    int mSocket = 0;

    std::size_t mTxPacketSize;
    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> mTxBuffer;
    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> mRxBuffer;

    bool mSuspendTx = false;
    bool mShutdownTx = false;
    bool mDecreaseProbingInterval = false;

    uint64_t mIcmpUnknownEventCount = 0;
    uint64_t mIcmpPacketCount = 0;
};

} /* namespace link_prober */

#endif /* LINKPROBER_H_ */
