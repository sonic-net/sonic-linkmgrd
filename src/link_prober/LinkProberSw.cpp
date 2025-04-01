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
 * LinkProberSw.cpp
 *
 *  Created on: Oct 4, 2020
 *      Author: tamer
 */

#include <netpacket/packet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <malloc.h>

#include <boost/bind/bind.hpp>

#include "common/MuxLogger.h"
#include "LinkProberSw.h"
#include "common/MuxException.h"
#include "LinkProberStateMachineActiveActive.h"
#include "LinkProberStateMachineActiveStandby.h"

namespace link_prober
{


//
// ---> LinkProberSw(
//          common::MuxPortConfig &muxPortConfig,
//          boost::asio::io_service &ioService,
//          LinkProberStateMachineBase &linkProberStateMachine
//      );
//
// class constructor
//
LinkProberSw::LinkProberSw(
    common::MuxPortConfig &muxPortConfig,
    boost::asio::io_service &ioService,
    LinkProberStateMachineBase *linkProberStateMachinePtr
) :
    LinkProberBase(muxPortConfig, ioService, linkProberStateMachinePtr)
{
    switch (mMuxPortConfig.getPortCableType()) {
        case common::MuxPortConfig::PortCableType::ActiveActive: {
            mReportHeartbeatReplyReceivedFuncPtr = boost::bind(
                &LinkProberSw::reportHeartbeatReplyReceivedActiveActive,
                this,
                boost::placeholders::_1
            );
            mReportHeartbeatReplyNotReceivedFuncPtr = boost::bind(
                &LinkProberSw::reportHeartbeatReplyNotReceivedActiveActive,
                this,
                boost::placeholders::_1
            );
            break;
        }
        case common::MuxPortConfig::PortCableType::ActiveStandby: {
            mReportHeartbeatReplyReceivedFuncPtr = boost::bind(
                &LinkProberSw::reportHeartbeatReplyReceivedActiveStandby,
                this,
                boost::placeholders::_1
            );
            mReportHeartbeatReplyNotReceivedFuncPtr = boost::bind(
                &LinkProberSw::reportHeartbeatReplyNotReceivedActiveStandby,
                this,
                boost::placeholders::_1
            );
            break;
        }
        default: {
            break;
        }
    }
}

//
// ---> initialize();
//
// initialize link prober sockets and builds ICMP packet
//
void LinkProberSw::initialize()
{
    SockAddrLinkLayer addr = {0};
    addr.sll_ifindex = if_nametoindex(mMuxPortConfig.getPortName().c_str());
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);

    mSocket = socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, IPPROTO_ICMP);
    if (mSocket < 0) {
        std::ostringstream errMsg;
        errMsg << "Failed to open socket with '" << strerror(errno) << "'"
               << std::endl;
        throw MUX_ERROR(SocketError, errMsg.str());
    }

    if (bind(mSocket, (struct sockaddr *) &addr, sizeof(addr))) {
        std::ostringstream errMsg;
        errMsg << "Failed to bind to interface '" << mMuxPortConfig.getPortName() << "' with '"
               << strerror(errno) << "'" << std::endl;
        throw MUX_ERROR(SocketError, errMsg.str());
    }

    mSockFilterPtr.get()[3].k = mMuxPortConfig.getBladeIpv4Address().to_v4().to_uint();
    if (setsockopt(mSocket, SOL_SOCKET, SO_ATTACH_FILTER, &mSockFilterProg, sizeof(mSockFilterProg)) != 0) {
        std::ostringstream errMsg;
        errMsg << "Failed to attach filter with '" << strerror(errno) << "'"
               << std::endl;
        throw MUX_ERROR(SocketError, errMsg.str());
    }

    mStream.assign(mSocket);
    initializeSendBuffer();

    startInitRecv();
}

//
// ---> initializeSendBuffer();
//
// initialize ICMP packet once
//
void LinkProberSw::initializeSendBuffer()
{
    ether_header *ethHeader = reinterpret_cast<ether_header *> (mTxBuffer.data());
    memcpy(ethHeader->ether_dhost, mMuxPortConfig.getBladeMacAddress().data(), sizeof(ethHeader->ether_dhost));
    if (mMuxPortConfig.ifEnableUseTorMac()) {
        memcpy(ethHeader->ether_shost, mMuxPortConfig.getTorMacAddress().data(), sizeof(ethHeader->ether_shost));
    } else {
        memcpy(ethHeader->ether_shost, mMuxPortConfig.getVlanMacAddress().data(), sizeof(ethHeader->ether_shost));
    }
    ethHeader->ether_type = htons(ETHERTYPE_IP);

    iphdr *ipHeader = reinterpret_cast<iphdr *> (mTxBuffer.data() + sizeof(ether_header));
    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (mTxBuffer.data() + sizeof(ether_header) + sizeof(iphdr));

    IcmpPayload *payloadPtr  = new (mTxBuffer.data() + mPacketHeaderSize) IcmpPayload();
    memcpy(payloadPtr->uuid, mSelfUUID.data, sizeof(payloadPtr->uuid));
    resetTxBufferTlv();
    appendTlvSentinel();
    size_t totalPayloadSize = mTxPacketSize - mPacketHeaderSize;

    ipHeader->ihl = sizeof(iphdr) >> 2;
    ipHeader->version = IPVERSION;
    ipHeader->tos = 0xb8;
    ipHeader->tot_len = htons(sizeof(iphdr) + sizeof(icmphdr) + totalPayloadSize);
    ipHeader->id = static_cast<uint16_t> (rand());
    ipHeader->frag_off = 0;
    ipHeader->ttl = 64;
    ipHeader->protocol = IPPROTO_ICMP;
    ipHeader->check = 0;
    ipHeader->saddr = htonl(mMuxPortConfig.getLoopbackIpv4Address().to_v4().to_uint());
    ipHeader->daddr = htonl(mMuxPortConfig.getBladeIpv4Address().to_v4().to_uint());
    computeChecksum(ipHeader, ipHeader->ihl << 2);

    icmpHeader->type = ICMP_ECHO;
    icmpHeader->code = 0;
    icmpHeader->un.echo.id = htons(mMuxPortConfig.getServerId());
    icmpHeader->un.echo.sequence = htons(mTxSeqNo);

    computeChecksum(icmpHeader, sizeof(icmphdr) + totalPayloadSize);
}

//
// ---> appendTlvSentinel
//
// Append TlvSentinel to the end of txBuffer
//
size_t LinkProberSw::appendTlvSentinel()
{
    size_t tlvSize = sizeof(TlvHead);
    assert(mTxPacketSize + tlvSize <= MUX_MAX_ICMP_BUFFER_SIZE);
    Tlv *tlvPtr = reinterpret_cast<Tlv *> (mTxBuffer.data() + mTxPacketSize);
    tlvPtr->tlvhead.type = TlvType::TLV_SENTINEL;
    tlvPtr->tlvhead.length = 0;
    mTxPacketSize += tlvSize;
    return tlvSize;
}


//
// ---> startProbing();
//
// start sending ICMP ECHOREQUEST packets
//
void LinkProberSw::startProbing()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    mStream.cancel();
    sendHeartbeat();
    startRecv();
    startTimer();
}

//
// ---> suspendTxProbes(uint32_t suspendTime_msec);
//
// suspend sending ICMP ECHOREQUEST packets
//
void LinkProberSw::suspendTxProbes(uint32_t suspendTime_msec)
{
    MUXLOGWARNING(boost::format("%s: suspend ICMP heartbeat probing %dms") % mMuxPortConfig.getPortName() % suspendTime_msec);

    // NOTE: the timer reset also cancels any pending async ops with ec as boost::asio::error::operation_aborted
    mSuspendTimer.expires_from_now(boost::posix_time::milliseconds(suspendTime_msec));
    mSuspendTimer.async_wait(mStrand.wrap(boost::bind(
        &LinkProberSw::handleSuspendTimeout,
        this,
        boost::asio::placeholders::error
    )));

    mSuspendTx = true;
    mCancelSuspend = false;
}

//
// ---> resumeTxProbes();
//
// resume sending ICMP ECHOREQUEST packets
//
void LinkProberSw::resumeTxProbes()
{
    MUXLOGWARNING(boost::format("%s: resume ICMP heartbeat probing") % mMuxPortConfig.getPortName());

    mSuspendTimer.cancel();
    mCancelSuspend = true;
}

//
// ---> updateEthernetFrame();
//
// update Ethernet frame of Tx Buffer
//
void LinkProberSw::updateEthernetFrame()
{
    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(&LinkProberSw::handleUpdateEthernetFrame, this)));
}

//
// ---> probePeerTor();
//
// send an early HB to peer ToR
//
void LinkProberSw::probePeerTor()
{
    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(&LinkProberSw::sendHeartbeat, this, false)));
}

//
// ---> detectLink();
//
// send HBs to detect the link status
//
void LinkProberSw::detectLink()
{
    boost::asio::io_service &ioService = mStrand.context();
    for (uint32_t i = 0; i < mMuxPortConfig.getPositiveStateChangeRetryCount(); ++i)
    {
        ioService.post(mStrand.wrap(boost::bind(&LinkProberSw::sendHeartbeat, this, true)));
    }
}

//
// ---> sendPeerSwitchCommand();
//
// send send peer switch command
//
void LinkProberSw::sendPeerSwitchCommand()
{
    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(&LinkProberSw::handleSendSwitchCommand, this)));
}

//
// ---> sendPeerProbeCommand();
//
// send peer probe command
//
void LinkProberSw::sendPeerProbeCommand()
{
    boost::asio::post(mStrand, boost::bind(&LinkProberSw::handleSendProbeCommand, this));
}

//
// ---> handleUpdateEthernetFrame();
//
// update Ethernet frame of Tx Buffer
//
void LinkProberSw::handleUpdateEthernetFrame()
{
    initializeSendBuffer();
}

//
// ---> handleSendSwitchCommand();
//
// send switch command to peer ToR
//
void LinkProberSw::handleSendSwitchCommand()
{
    initTxBufferTlvSendSwitch();

    sendHeartbeat();

    initTxBufferTlvSentinel();

    // inform the composite state machine about command send completion
    boost::asio::io_service::strand &strand = mLinkProberStateMachinePtr->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (LinkProberStateMachineBase::*) (SwitchActiveCommandCompleteEvent&)>
            (&LinkProberStateMachineBase::processEvent),
        mLinkProberStateMachinePtr,
        LinkProberStateMachineBase::getSwitchActiveCommandCompleteEvent()
    )));
}

//
// ---> handleSendProbeCommand();
//
// send probe command to peer ToR
//
void LinkProberSw::handleSendProbeCommand()
{
    initTxBufferTlvSendProbe();

    sendHeartbeat();

    initTxBufferTlvSentinel();
}

//
// ---> sendHeartbeat(bool forceSend)
//
// send ICMP ECHOREQUEST packet
//
void LinkProberSw::sendHeartbeat(bool forceSend)
{
    MUXLOGTRACE(mMuxPortConfig.getPortName());

    updateIcmpSequenceNo();
    // check if suspend timer is running
    if (forceSend || ((!mSuspendTx) && (!mShutdownTx))) {
        boost::system::error_code errorCode;
        mStream.write_some(boost::asio::buffer(mTxBuffer.data(), mTxPacketSize), errorCode);

        if (errorCode) {
            MUXLOGTRACE(mMuxPortConfig.getPortName() + ": Failed to send heartbeat! Error code: " + errorCode.message());
        } else {
            MUXLOGTRACE(mMuxPortConfig.getPortName() + ": Done sending data");
        }
    }
}

//
// ---> handleTimeout(boost::system::error_code ec);
//
// handle ICMP packet reception timeout
//
void LinkProberSw::handleTimeout(boost::system::error_code errorCode)
{
    MUXLOGTRACE(boost::format("%s: server: %d, mRxSelfSeqNo: %d, mRxPeerSeqNo: %d, mTxSeqNo: %d") %
        mMuxPortConfig.getPortName() %
        mMuxPortConfig.getServerId() %
        mRxSelfSeqNo %
        mRxPeerSeqNo %
        mTxSeqNo
    );

    mStream.cancel();
    mReportHeartbeatReplyNotReceivedFuncPtr(HeartbeatType::HEARTBEAT_SELF);

    mIcmpPacketCount++;
    if (mIcmpPacketCount % mMuxPortConfig.getLinkProberStatUpdateIntervalCount() == 0) {
        boost::asio::io_service::strand &strand = mLinkProberStateMachinePtr->getStrand();
        boost::asio::io_service &ioService = strand.context();
        ioService.post(strand.wrap(boost::bind(
            &LinkProberStateMachineBase::handlePckLossRatioUpdate,
            mLinkProberStateMachinePtr,
            mIcmpUnknownEventCount,
            mIcmpPacketCount
        )));
    }

    // start another cycle of send/recv
    startProbing();
}

//
// ---> handleSuspendTimeout(boost::system::error_code errorCode);
//
// handle suspend timer timeout
//
void LinkProberSw::handleSuspendTimeout(boost::system::error_code errorCode)
{
    MUXLOGWARNING(boost::format("%s: suspend timeout, resume ICMP heartbeat probing") % mMuxPortConfig.getPortName());

    mSuspendTx = false;

    if (errorCode == boost::system::errc::success || mCancelSuspend) {
        // inform the composite state machine about Suspend timer expiry or cancel
        boost::asio::io_service::strand &strand = mLinkProberStateMachinePtr->getStrand();
        boost::asio::io_service &ioService = strand.context();
        ioService.post(strand.wrap(boost::bind(
            static_cast<void (LinkProberStateMachineBase::*) (SuspendTimerExpiredEvent&)>
                (&LinkProberStateMachineBase::processEvent),
            mLinkProberStateMachinePtr,
            LinkProberStateMachineBase::getSuspendTimerExpiredEvent()
        )));
    }
    mCancelSuspend = false;
}


//
// ---> calculateChecksum(uint16_t *data, size_t size);
//
// calculate ICMP payload checksum
//
uint32_t LinkProberSw::calculateChecksum(uint16_t *data, size_t size)
{
    uint32_t sum = 0;

    while (size > 1) {
        sum += ntohs(*data++);
        size -= sizeof(uint16_t);
    }

    if (size) {
        sum += ntohs(static_cast<uint16_t> ((*reinterpret_cast<uint8_t *> (data))));
    }

    return sum;
}

//
// ---> addChecksumCarryover(uint16_t *checksum, uint32_t sum);
//
// add checksum carryover
//
void LinkProberSw::addChecksumCarryover(uint16_t *checksum, uint32_t sum)
{
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    *checksum = htons(~sum);
}

//
// ---> computeChecksum(icmphdr *icmpHeader, size_t size);
//
// compute ICMP checksum
//
void LinkProberSw::computeChecksum(icmphdr *icmpHeader, size_t size)
{
    icmpHeader->checksum = 0;
    mIcmpChecksum = calculateChecksum(
        reinterpret_cast<uint16_t *> (icmpHeader), size
    );
    addChecksumCarryover(&icmpHeader->checksum, mIcmpChecksum);
}

//
// ---> computeChecksum(iphdr *ipHeader, size_t size);
//
// compute IPv4 checksum
//
void LinkProberSw::computeChecksum(iphdr *ipHeader, size_t size)
{
    ipHeader->check = 0;
    mIpChecksum = calculateChecksum(
        reinterpret_cast<uint16_t *> (ipHeader), size
    );
    addChecksumCarryover(&ipHeader->check, mIpChecksum);
}



//
// ---> updateIcmpSequenceNo();
//
// update ICMP packet checksum, used before sending new heartbeat
//
void LinkProberSw::updateIcmpSequenceNo()
{
    // update received sequence to avoid reporting invalid ICMP event when sequence number rolls over
    mRxPeerSeqNo = mTxSeqNo;
    mRxSelfSeqNo = mTxSeqNo;

    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (mTxBuffer.data() + sizeof(ether_header) + sizeof(iphdr));
    icmpHeader->un.echo.sequence = htons(++mTxSeqNo);
    mIcmpChecksum += mTxSeqNo ? 1 : 0;
    addChecksumCarryover(&icmpHeader->checksum, mIcmpChecksum);
}

//
// ---> appendTlvCommand
//
// Append TlvCommand to the end of txBuffer
//
size_t LinkProberSw::appendTlvCommand(Command commandType)
{
    size_t tlvSize = sizeof(TlvHead) + sizeof(Command);
    assert(mTxPacketSize + tlvSize <= MUX_MAX_ICMP_BUFFER_SIZE);
    Tlv *tlvPtr = reinterpret_cast<Tlv *> (mTxBuffer.data() + mTxPacketSize);
    tlvPtr->tlvhead.type = TlvType::TLV_COMMAND;
    tlvPtr->tlvhead.length = htons(sizeof(Command));
    tlvPtr->command = static_cast<uint8_t> (commandType);
    mTxPacketSize += tlvSize;
    return tlvSize;
}



//
// ---> appendTlvDummy
//
// Append a dummy TLV, test purpose only
//
size_t LinkProberSw::appendTlvDummy(size_t paddingSize, int seqNo)
{
    size_t tlvSize = sizeof(TlvHead) + paddingSize + sizeof(uint32_t);
    assert(mTxPacketSize + tlvSize <= MUX_MAX_ICMP_BUFFER_SIZE);
    Tlv *tlvPtr = reinterpret_cast<Tlv *> (mTxBuffer.data() + mTxPacketSize);
    tlvPtr->tlvhead.type = TlvType::TLV_DUMMY;
    tlvPtr->tlvhead.length = htons(paddingSize + sizeof(uint32_t));
    memset(tlvPtr->data, 0, paddingSize);
    *(reinterpret_cast<uint32_t *> (tlvPtr->data + paddingSize)) = htonl(seqNo);
    mTxPacketSize += tlvSize;
    return tlvSize;
}

//
// ---> initTxBufferTlvSendSwitch
//
// Initialize TX buffer TLVs to send switch command to peer
//
void LinkProberSw::initTxBufferTlvSendSwitch()
{
    resetTxBufferTlv();
    appendTlvCommand(Command::COMMAND_SWITCH_ACTIVE);
    appendTlvSentinel();

    calculateTxPacketChecksum();
}

//
// ---> initTxBufferTlvSendProbe
//
// Initialize TX buffer TLVs to send probe command to peer
//
void LinkProberSw::initTxBufferTlvSendProbe()
{
    resetTxBufferTlv();
    appendTlvCommand(Command::COMMAND_MUX_PROBE);
    appendTlvSentinel();

    calculateTxPacketChecksum();
}

//
// ---> initTxBufferTlvSentinel
//
// Initialize TX buffer to have only TLV sentinel
//
void LinkProberSw::initTxBufferTlvSentinel()
{
    resetTxBufferTlv();
    appendTlvSentinel();

    calculateTxPacketChecksum();
}

//
// ---> calculateChecksum
//
// Calculate TX packet checksums in both IP header and ICMP header
//
void LinkProberSw::calculateTxPacketChecksum()
{
    size_t totalPayloadSize = mTxPacketSize - mPacketHeaderSize;
    iphdr *ipHeader = reinterpret_cast<iphdr *> (mTxBuffer.data() + sizeof(ether_header));
    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (mTxBuffer.data() + sizeof(ether_header) + sizeof(iphdr));
    computeChecksum(icmpHeader, sizeof(icmphdr) + totalPayloadSize);
    ipHeader->tot_len = htons(sizeof(iphdr) + sizeof(icmphdr) + totalPayloadSize);
    computeChecksum(ipHeader, ipHeader->ihl << 2);
}

// 
// ---> resetIcmpPacketCounts
//
// reset Icmp packet counts, post a pck loss ratio update immediately 
//
void LinkProberSw::resetIcmpPacketCounts()
{
    mIcmpUnknownEventCount = 0;
    mIcmpPacketCount = 0;

    boost::asio::io_service::strand &strand = mLinkProberStateMachinePtr->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        &LinkProberStateMachineBase::handlePckLossRatioUpdate,
        mLinkProberStateMachinePtr,
        mIcmpUnknownEventCount,
        mIcmpPacketCount
    )));
}

// 
// ---> shutdownTxProbes();
//
// stop sending ICMP ECHOREQUEST packets
//
void LinkProberSw::shutdownTxProbes()
{
    MUXLOGWARNING(boost::format("%s: shutdown ICMP heartbeat probing") % mMuxPortConfig.getPortName());

    mShutdownTx = true;
}

// 
// ---> restartTxProbes();
//
// first stop sending and then again resume sending ICMP ECHOREQUEST packets
//
void LinkProberSw::restartTxProbes()
{
    MUXLOGWARNING(boost::format("%s: restart ICMP heartbeat probing") % mMuxPortConfig.getPortName());

    mShutdownTx = false;
}

//
// ---> decreaseProbeIntervalAfterSwitch(uint32_t switchTime_msec);
//
//  adjust link prober interval to 10 ms after switchover to better measure the switchover overhead.
//
void LinkProberSw::decreaseProbeIntervalAfterSwitch(uint32_t switchTime_msec)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    mSwitchoverTimer.expires_from_now(boost::posix_time::milliseconds(switchTime_msec));
    mSwitchoverTimer.async_wait(mStrand.wrap(boost::bind(
        &LinkProberSw::handleSwitchoverTimeout,
        this,
        boost::asio::placeholders::error
    )));

    mDecreaseProbingInterval = true;
}

// ---> revertProbeIntervalAfterSwitchComplete();
//
// revert probe interval change after switchover is completed
// 
void LinkProberSw::revertProbeIntervalAfterSwitchComplete()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    mSwitchoverTimer.cancel();
    mDecreaseProbingInterval = false;
}

//
// ---> handleSwitchoverTimeout(boost::system::error_code errorCode)
//
// handle switchover time out 
// 
void LinkProberSw::handleSwitchoverTimeout(boost::system::error_code errorCode)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    mDecreaseProbingInterval = false;
    if (errorCode == boost::system::errc::success) {
        MUXLOGWARNING(boost::format("%s: link prober timeout on waiting for expected ICMP event after switchover is triggered ") % mMuxPortConfig.getPortName());
    }
}

//
// ---> reportHeartbeatReplyReceivedActiveStandby(HeartbeatType heartbeatType)
//
// report heartbeat reply received to active-standby mode link prober state machine
//
void LinkProberSw::reportHeartbeatReplyReceivedActiveStandby(HeartbeatType heartbeatType)
{
    if (mTxSeqNo == mRxSelfSeqNo) {
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpSelfEvent());
    } else if (mTxSeqNo == mRxPeerSeqNo) {
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerEvent());
    }
}

//
// ---> reportHeartbeatReplyNotReceivedActiveStandby
//
// report heartbeat reply received to active-active mode link prober state machine
//
void LinkProberSw::reportHeartbeatReplyNotReceivedActiveStandby(HeartbeatType heartbeatType)
{
    if (mTxSeqNo != mRxSelfSeqNo && mTxSeqNo != mRxPeerSeqNo) {
        // post unknown event
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpUnknownEvent());
        mIcmpUnknownEventCount++;
    }
}

//
// ---> reportHeartbeatReplyReceivedActiveActive(HeartbeatType heartbeatType)
//
// report heartbeat reply not received to active-standby mode link prober state machine
//
void LinkProberSw::reportHeartbeatReplyReceivedActiveActive(HeartbeatType heartbeatType)
{
    if (heartbeatType == HeartbeatType::HEARTBEAT_SELF && mTxSeqNo == mRxSelfSeqNo) {
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpSelfEvent());
    }
    if (heartbeatType == HeartbeatType::HEARTBEAT_PEER && mTxSeqNo == mRxPeerSeqNo) {
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerActiveEvent());
    }
}

//
// ---> reportHeartbeatReplyNotReceivedActiveActive
//
// report heartbeat reply not received to active-active mode link prober state machine
//
void LinkProberSw::reportHeartbeatReplyNotReceivedActiveActive(HeartbeatType heartbeatType)
{
    if (mTxSeqNo != mRxSelfSeqNo) {
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpUnknownEvent());
        mIcmpUnknownEventCount++;
    }
    if (mTxSeqNo != mRxPeerSeqNo) {
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerUnknownEvent());
    }
}

//
// ---> startTimer();
//
// start ICMP ECHOREPLY timeout timer
//
void LinkProberSw::startTimer()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());
    // time out these heartbeats
    mDeadlineTimer.expires_from_now(boost::posix_time::milliseconds(getProbingInterval()));
    mDeadlineTimer.async_wait(mStrand.wrap(boost::bind(
        &LinkProberSw::handleTimeout,
        this,
        boost::asio::placeholders::error
    )));
}

} /* namespace link_prober */
