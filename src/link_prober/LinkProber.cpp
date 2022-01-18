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
 * LinkProber.cpp
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
#include "LinkProber.h"
#include "common/MuxException.h"

namespace link_prober
{

//
// Berkeley Packet Filter program that captures incoming ICMP traffic
//
SockFilter LinkProber::mIcmpFilter[] = {
    [0]  = {.code = 0x28, .jt = 0, .jf = 0,  .k = 0x0000000c},
    [1]  = {.code = 0x15, .jt = 0, .jf = 10, .k = 0x00000800},
    [2]  = {.code = 0x20, .jt = 0, .jf = 0,  .k = 0x0000001a},
    [3]  = {.code = 0x15, .jt = 0, .jf = 8,  .k = 0x00000000},
    [4]  = {.code = 0x30, .jt = 0, .jf = 0,  .k = 0x00000017},
    [5]  = {.code = 0x15, .jt = 0, .jf = 6,  .k = 0x00000001},
    [6]  = {.code = 0x28, .jt = 0, .jf = 0,  .k = 0x00000014},
    [7]  = {.code = 0x45, .jt = 4, .jf = 0,  .k = 0x00001fff},
    [8]  = {.code = 0xb1, .jt = 0, .jf = 0,  .k = 0x0000000e},
    [9]  = {.code = 0x50, .jt = 0, .jf = 0,  .k = 0x0000000e},
    [10] = {.code = 0x15, .jt = 0, .jf = 1,  .k = 0x00000000},
    [11] = {.code = 0x6,  .jt = 0, .jf = 0,  .k = 0x00040000},
    [12] = {.code = 0x6,  .jt = 0, .jf = 0,  .k = 0x00000000},
};

//
// ---> LinkProber(
//          common::MuxPortConfig &muxPortConfig,
//          boost::asio::io_service &ioService,
//          LinkProberStateMachine &linkProberStateMachine
//      );
//
// class constructor
//
LinkProber::LinkProber(
    common::MuxPortConfig &muxPortConfig,
    boost::asio::io_service &ioService,
    LinkProberStateMachine &linkProberStateMachine
) :
    mMuxPortConfig(muxPortConfig),
    mIoService(ioService),
    mLinkProberStateMachine(linkProberStateMachine),
    mStrand(mIoService),
    mDeadlineTimer(mIoService),
    mSuspendTimer(mIoService),
    mStream(mIoService)
{
    try {
        mSockFilterPtr = std::shared_ptr<SockFilter> (
            new SockFilter[sizeof(mIcmpFilter) / sizeof(*mIcmpFilter)],
            std::default_delete<SockFilter[]>()
        );
        memcpy(mSockFilterPtr.get(), mIcmpFilter, sizeof(mIcmpFilter));

        mSockFilterProg.len = sizeof(mIcmpFilter) / sizeof(*mIcmpFilter);
        mSockFilterProg.filter = mSockFilterPtr.get();
    }
    catch (const std::bad_alloc& ex) {
        std::ostringstream errMsg;
        errMsg << "Failed allocate memory. Exception details: " << ex.what();

        throw MUX_ERROR(BadAlloc, errMsg.str());
    }
}

//
// ---> initialize();
//
// initialize link prober sockets and builds ICMP packet
//
void LinkProber::initialize()
{
    mSocket = socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, IPPROTO_ICMP);
    if (mSocket < 0) {
        std::ostringstream errMsg;
        errMsg << "Failed to open socket with '" << strerror(errno) << "'"
               << std::endl;
        throw MUX_ERROR(SocketError, errMsg.str());
    }

    SockAddrLinkLayer addr;
    memset(&addr, 0, sizeof(addr));

    addr.sll_ifindex = if_nametoindex(mMuxPortConfig.getPortName().c_str());
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);
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
// ---> startProbing();
//
// start sending ICMP ECHOREQUEST packets
//
void LinkProber::startProbing()
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
void LinkProber::suspendTxProbes(uint32_t suspendTime_msec)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    mSuspendTimer.expires_from_now(boost::posix_time::milliseconds(suspendTime_msec));
    mSuspendTimer.async_wait(mStrand.wrap(boost::bind(
        &LinkProber::handleSuspendTimeout,
        this,
        boost::asio::placeholders::error
    )));

    mSuspendTx = true;
}

//
// ---> resumeTxProbes();
//
// resume sending ICMP ECHOREQUEST packets
//
void LinkProber::resumeTxProbes()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    mSuspendTimer.cancel();
}

//
// ---> updateEthernetFrame();
//
// update Ethernet frame of Tx Buffer
//
void LinkProber::updateEthernetFrame()
{
    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(&LinkProber::handleUpdateEthernetFrame, this)));
}

//
// ---> probePeerTor();
//
// send an early HB to peer ToR
//
void LinkProber::probePeerTor()
{
    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(&LinkProber::sendHeartbeat, this)));
}

//
// ---> sendPeerSwitchCommand();
//
// send send peer switch command
//
void LinkProber::sendPeerSwitchCommand()
{
    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(&LinkProber::handleSendSwitchCommand, this)));
}

//
// ---> handleUpdateEthernetFrame();
//
// update Ethernet frame of Tx Buffer
//
void LinkProber::handleUpdateEthernetFrame()
{
    initializeSendBuffer();
}

//
// ---> handleSendSwitchCommand();
//
// send switch command to peer ToR
//
void LinkProber::handleSendSwitchCommand()
{
    IcmpPayload *icmpPayload = reinterpret_cast<IcmpPayload *> (
        mTxBuffer.data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr)
    );
    icmpPayload->command = htonl(static_cast<uint32_t> (Command::COMMAND_SWITCH_ACTIVE));

    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (mTxBuffer.data() + sizeof(ether_header) + sizeof(iphdr));
    computeChecksum(icmpHeader, sizeof(icmphdr) + sizeof(*icmpPayload));

    sendHeartbeat();

    icmpPayload->command = htonl(static_cast<uint32_t> (Command::COMMAND_NONE));
    computeChecksum(icmpHeader, sizeof(icmphdr) + sizeof(*icmpPayload));

    // inform the composite state machine about commend send completion
    boost::asio::io_service::strand &strand = mLinkProberStateMachine.getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (LinkProberStateMachine::*) (SwitchActiveCommandCompleteEvent&)>
            (&LinkProberStateMachine::processEvent),
        &mLinkProberStateMachine,
        LinkProberStateMachine::getSwitchActiveCommandCompleteEvent()
    )));
}

//
// ---> sendHeartbeat()
//
// send ICMP ECHOREQUEST packet
//
void LinkProber::sendHeartbeat()
{
    MUXLOGTRACE(mMuxPortConfig.getPortName());

    // check if suspend timer is running
    if (!mSuspendTx) {
        updateIcmpSequenceNo();
        boost::system::error_code errorCode;
        mStream.write_some(
            boost::asio::buffer(
                mTxBuffer.data(), sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr) + sizeof(IcmpPayload)
            ),
            errorCode
        );

        if (errorCode) {
            MUXLOGTRACE(mMuxPortConfig.getPortName() + ": Failed to send heartbeat!");
        } else {
            MUXLOGTRACE(mMuxPortConfig.getPortName() + ": Done sending data");
        }
    }
}

//
// ---> handleRecv(const boost::system::error_code& errorCode, size_t bytesTransferred);
//
// handle packet reception
//
void LinkProber::handleRecv(
    const boost::system::error_code& errorCode,
    size_t bytesTransferred
)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    if (errorCode != boost::asio::error::operation_aborted) {
        iphdr *ipHeader = reinterpret_cast<iphdr *> (mRxBuffer.data() + sizeof(ether_header));
        icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (
            mRxBuffer.data() + sizeof(ether_header) + sizeof(iphdr)
        );

        IcmpPayload *icmpPayload = reinterpret_cast<IcmpPayload *> (
            mRxBuffer.data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr)
        );

        if (ntohl(icmpPayload->cookie) == IcmpPayload::getCookie() &&
            ntohl(icmpPayload->version) <= IcmpPayload::getVersion() &&
            ntohs(icmpHeader->un.echo.id) == mMuxPortConfig.getServerId()) {
            MUXLOGTRACE(boost::format("%s: Valid ICMP Packet from %s") %
                mMuxPortConfig.getPortName() %
                mMuxPortConfig.getBladeIpv4Address().to_string()
            );
            if (memcmp(icmpPayload->un.uuid.data, IcmpPayload::getGuidData(),
                       IcmpPayload::getGuid().size()) == 0) {
                MUXLOGTRACE(boost::format("%s: Matching Guid") % mMuxPortConfig.getPortName());
                // echo reply for an echo request generated by this/active ToR
                mRxSelfSeqNo = mTxSeqNo;
                mLinkProberStateMachine.postLinkProberStateEvent(LinkProberStateMachine::getIcmpSelfEvent());
            } else {
                // echo reply for an echo request generated by peer ToR
                mRxPeerSeqNo = mTxSeqNo;
                mLinkProberStateMachine.postLinkProberStateEvent(LinkProberStateMachine::getIcmpPeerEvent());
                if (ntohl(icmpPayload->command) == static_cast<uint32_t> (Command::COMMAND_SWITCH_ACTIVE)) {
                    boost::asio::io_service::strand &strand = mLinkProberStateMachine.getStrand();
                    boost::asio::io_service &ioService = strand.context();
                    ioService.post(strand.wrap(boost::bind(
                        static_cast<void (LinkProberStateMachine::*) (SwitchActiveRequestEvent&)>
                            (&LinkProberStateMachine::processEvent),
                        &mLinkProberStateMachine,
                        LinkProberStateMachine::getSwitchActiveRequestEvent()
                    )));
                }
            }
        } else {
            // Unknown ICMP packet, ignore.
        }

        MUXLOGTRACE(boost::format("%s: Got data from: %s, size: %d") %
            mMuxPortConfig.getPortName() %
            boost::asio::ip::address_v4(ntohl(ipHeader->saddr)).to_string() %
            (bytesTransferred - sizeof(iphdr) - sizeof(ether_header))
        );
        // start another receive to consume as much as possible of backlog packets if any
        startRecv();
    }
}

//
// ---> handleInitRecv(const boost::system::error_code& errorCode, size_t bytesTransferred);
//
// handle packet reception
//
void LinkProber::handleInitRecv(
    const boost::system::error_code& errorCode,
    size_t bytesTransferred
)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    if (errorCode != boost::asio::error::operation_aborted) {
        ether_header *ethHeader = reinterpret_cast<ether_header *> (mRxBuffer.data());
        std::array<uint8_t, ETHER_ADDR_LEN> macAddress;

        memcpy(macAddress.data(), ethHeader->ether_shost, macAddress.size());

        boost::asio::io_service::strand &strand = mLinkProberStateMachine.getStrand();
        boost::asio::io_service &ioService = strand.context();
        ioService.post(strand.wrap(boost::bind(
            &LinkProberStateMachine::handleMackAddressUpdate,
            &mLinkProberStateMachine,
            macAddress
        )));
    }
}

//
// ---> handleTimeout(boost::system::error_code ec);
//
// handle ICMP packet reception timeout
//
void LinkProber::handleTimeout(boost::system::error_code errorCode)
{
    MUXLOGTRACE(boost::format("%s: server: %d, mRxSelfSeqNo: %d, mTxSeqNo: %d") %
        mMuxPortConfig.getPortName() %
        mMuxPortConfig.getServerId() %
        mRxSelfSeqNo %
        mTxSeqNo
    );

    mStream.cancel();
    if (mTxSeqNo != mRxSelfSeqNo && mTxSeqNo != mRxPeerSeqNo) {
        // post unknown event
        mLinkProberStateMachine.postLinkProberStateEvent(LinkProberStateMachine::getIcmpUnknownEvent());
    }

    // start another cycle of send/recv
    startProbing();
}

//
// ---> handleSuspendTimeout(boost::system::error_code errorCode);
//
// handle suspend timer timeout
//
void LinkProber::handleSuspendTimeout(boost::system::error_code errorCode)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    mSuspendTx = false;

    if (errorCode == boost::system::errc::success) {
        // inform the composite state machine about Suspend timer expiry
        boost::asio::io_service::strand &strand = mLinkProberStateMachine.getStrand();
        boost::asio::io_service &ioService = strand.context();
        ioService.post(strand.wrap(boost::bind(
            static_cast<void (LinkProberStateMachine::*) (SuspendTimerExpiredEvent&)>
                (&LinkProberStateMachine::processEvent),
            &mLinkProberStateMachine,
            LinkProberStateMachine::getSuspendTimerExpiredEvent()
        )));
    }
}

//
// ---> startRecv();
//
// start ICMP ECHOREPLY reception
//
void LinkProber::startRecv()
{
    MUXLOGTRACE(mMuxPortConfig.getPortName());

    mStream.async_read_some(
        boost::asio::buffer(mRxBuffer, MUX_MAX_ICMP_BUFFER_SIZE),
        mStrand.wrap(boost::bind(
            &LinkProber::handleRecv,
            this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred
        ))
    );
}

//
// ---> startInitRecv();
//
// start ICMP ECHOREPLY reception
//
void LinkProber::startInitRecv()
{
    MUXLOGTRACE(mMuxPortConfig.getPortName());

    mStream.async_read_some(
        boost::asio::buffer(mRxBuffer, MUX_MAX_ICMP_BUFFER_SIZE),
        mStrand.wrap(boost::bind(
            &LinkProber::handleInitRecv,
            this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred
        ))
    );
}

//
// ---> startTimer();
//
// start ICMP ECHOREPLY timeout timer
//
void LinkProber::startTimer()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());
    // time out these heartbeats
    mDeadlineTimer.expires_from_now(boost::posix_time::milliseconds(mMuxPortConfig.getTimeoutIpv4_msec()));
    mDeadlineTimer.async_wait(mStrand.wrap(boost::bind(
        &LinkProber::handleTimeout,
        this,
        boost::asio::placeholders::error
    )));
}

//
// ---> calculateChecksum(uint16_t *data, size_t size);
//
// calculate ICMP payload checksum
//
uint32_t LinkProber::calculateChecksum(uint16_t *data, size_t size)
{
    uint32_t sum = 0;
    size_t offset = 0;

    do {
        sum += ntohs(data[offset++]);
    } while (offset < size);

    return sum;
}

//
// ---> addChecksumCarryover(uint16_t *checksum, uint32_t sum);
//
// add checksum carryover
//
void LinkProber::addChecksumCarryover(uint16_t *checksum, uint32_t sum)
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
void LinkProber::computeChecksum(icmphdr *icmpHeader, size_t size)
{
    icmpHeader->checksum = 0;
    mIcmpChecksum = calculateChecksum(
        reinterpret_cast<uint16_t *> (icmpHeader), size / 2
    );
    addChecksumCarryover(&icmpHeader->checksum, mIcmpChecksum);
}

//
// ---> computeChecksum(iphdr *ipHeader, size_t size);
//
// compute IPv4 checksum
//
void LinkProber::computeChecksum(iphdr *ipHeader, size_t size)
{
    ipHeader->check = 0;
    mIpChecksum = calculateChecksum(
        reinterpret_cast<uint16_t *> (ipHeader), size / 2
    );
    addChecksumCarryover(&ipHeader->check, mIpChecksum);
}

//
// ---> initializeSendBuffer();
//
// initialize ICMP packet once
//
void LinkProber::initializeSendBuffer()
{
    ether_header *ethHeader = reinterpret_cast<ether_header *> (mTxBuffer.data());
    memcpy(ethHeader->ether_dhost, mMuxPortConfig.getBladeMacAddress().data(), sizeof(ethHeader->ether_dhost));
    memcpy(ethHeader->ether_shost, mMuxPortConfig.getTorMacAddress().data(), sizeof(ethHeader->ether_shost));
    ethHeader->ether_type = htons(ETHERTYPE_IP);

    iphdr *ipHeader = reinterpret_cast<iphdr *> (mTxBuffer.data() + sizeof(ether_header));
    ipHeader->ihl = sizeof(iphdr) >> 2;
    ipHeader->version = IPVERSION;
    ipHeader->tos = 0xb8;
    ipHeader->tot_len = htons(sizeof(iphdr) + sizeof(icmphdr) + sizeof(IcmpPayload));
    ipHeader->id = static_cast<uint16_t> (rand());
    ipHeader->frag_off = 0;
    ipHeader->ttl = 64;
    ipHeader->protocol = IPPROTO_ICMP;
    ipHeader->check = 0;
    ipHeader->saddr = htonl(mMuxPortConfig.getLoopbackIpv4Address().to_v4().to_uint());
    ipHeader->daddr = htonl(mMuxPortConfig.getBladeIpv4Address().to_v4().to_uint());
    computeChecksum(ipHeader, ipHeader->ihl << 2);

    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (mTxBuffer.data() + sizeof(ether_header) + sizeof(iphdr));
    icmpHeader->type = ICMP_ECHO;
    icmpHeader->code = 0;
    icmpHeader->un.echo.id = htons(mMuxPortConfig.getServerId());
    icmpHeader->un.echo.sequence = htons(mTxSeqNo);

    IcmpPayload *icmpPayload = new (mTxBuffer.data() + sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr)) IcmpPayload();
    computeChecksum(icmpHeader, sizeof(icmphdr) + sizeof(*icmpPayload));
}

//
// ---> updateIcmpSequenceNo();
//
// update ICMP packet checksum, used before sending new heartbeat
//
void LinkProber::updateIcmpSequenceNo()
{
    // update received sequence to avoid reporting invalid ICMP event when sequence number rolls over
    mRxPeerSeqNo = mTxSeqNo;
    mRxSelfSeqNo = mTxSeqNo;

    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (mTxBuffer.data() + sizeof(ether_header) + sizeof(iphdr));
    icmpHeader->un.echo.sequence = htons(++mTxSeqNo);
    mIcmpChecksum += mTxSeqNo ? 1 : 0;
    addChecksumCarryover(&icmpHeader->checksum, mIcmpChecksum);
}

} /* namespace link_prober */
