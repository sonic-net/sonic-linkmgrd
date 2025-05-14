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
    setupSocket();
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
    MUXLOGWARNING(boost::format("%s: suspend ICMP heartbeat probing") % mMuxPortConfig.getPortName());

    mSuspendTimer.expires_from_now(boost::posix_time::milliseconds(suspendTime_msec));
    mSuspendTimer.async_wait(mStrand.wrap(boost::bind(
        &LinkProberSw::handleSuspendTimeout,
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
void LinkProberSw::resumeTxProbes()
{
    MUXLOGWARNING(boost::format("%s: resume ICMP heartbeat probing") % mMuxPortConfig.getPortName());

    mSuspendTimer.cancel();
    
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

    if (errorCode == boost::system::errc::success) {
        // inform the composite state machine about Suspend timer expiry
        boost::asio::io_service::strand &strand = mLinkProberStateMachinePtr->getStrand();
        boost::asio::io_service &ioService = strand.context();
        ioService.post(strand.wrap(boost::bind(
            static_cast<void (LinkProberStateMachineBase::*) (SuspendTimerExpiredEvent&)>
                (&LinkProberStateMachineBase::processEvent),
            mLinkProberStateMachinePtr,
            LinkProberStateMachineBase::getSuspendTimerExpiredEvent()
        )));
    }
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

void LinkProberSw::handleIcmpPayload(size_t bytesTransferred, icmphdr *icmpHeader, IcmpPayload *icmpPayload)
{
    bool isHwCookie = ntohl(icmpPayload->cookie) == IcmpPayload::getHardwareCookie();
    bool isSwCookie = ntohl(icmpPayload->cookie) == IcmpPayload::getSoftwareCookie();

    if (ntohl(icmpPayload->version) <= IcmpPayload::getVersion() &&
       ((ntohs(icmpHeader->un.echo.id) == mMuxPortConfig.getServerId()) ||
         isHwCookie)) {
        // echo.id in hw prober will not be set
        MUXLOGTRACE(boost::format("%s: Valid ICMP Packet from %s") %
            mMuxPortConfig.getPortName() %
            mMuxPortConfig.getBladeIpv4Address().to_string()
        );

        uint64_t network_guid = *reinterpret_cast<uint64_t*>(icmpPayload->uuid);
        if (network_guid == 0) {
            MUXLOGWARNING(boost::format("Received invalid Raw GUID: {%d}") % network_guid);
            startRecv();
            return;
        }
        //MUXLOGWARNING(boost::format("Raw GUID: {0x%016llx}") % network_guid);
        uint64_t host_guid = ntohll(network_guid);
        //MUXLOGWARNING(boost::format("ntohll GUID: {0x%016llx}") % host_guid);
        host_guid = static_cast<uint32_t>(host_guid);
        //MUXLOGWARNING(boost::format("uint32_t  GUID: {0x0x%08x}") % host_guid);
        std::stringstream os;
        os << std::hex << std::setw(8) << std::setfill('0') << host_guid;
        auto guidDataStr = os.str();
        guidDataStr = "0x" + guidDataStr;
        bool isSelfGuid = getSelfGuidData() == guidDataStr;
        HeartbeatType heartbeatType;
        if (isSelfGuid) {
            // echo reply for an echo request generated by this/active ToR
            mRxSelfSeqNo = mTxSeqNo;
            heartbeatType = HeartbeatType::HEARTBEAT_SELF;
        } else {
            mRxPeerSeqNo = mTxSeqNo;
            heartbeatType = HeartbeatType::HEARTBEAT_PEER;
            MUXLOGWARNING(boost::format("Peer Guid Detected %s") % guidDataStr);
            // add the peer guid to the set if not present
            setPeerGuidData(guidDataStr);
            if (mGuidSet.find(mPeerGuid) != mGuidSet.end())
            {
                mGuidSet.insert(guidDataStr);
            }
        }
        mReportHeartbeatReplyReceivedFuncPtr(heartbeatType);
        handleTlvRecv(bytesTransferred, isSelfGuid);
    } else {
        MUXLOGWARNING(boost::format("Received invalid packet in software prober"));
    }
    startRecv();
}

} /* namespace link_prober */
