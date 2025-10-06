/*
 * LinkProberHw.cpp
 *
 *  Created on: Oct 4, 2020
 *      Author: Harjot Singh
 */

#include <netpacket/packet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <malloc.h>
#include <vector>

#include <boost/bind/bind.hpp>

#include "common/MuxLogger.h"
#include "LinkProberHw.h"
#include "common/MuxException.h"
#include "LinkProberStateMachineActiveActive.h"
#include "LinkProberStateMachineActiveStandby.h"
#include "../MuxPort.h"

namespace link_prober
{

std::string LinkProberHw::mIcmpTableName   = "ICMP_ECHO_SESSION";
std::string LinkProberHw::mSessionCookie   = "0x58767e7a";
std::string LinkProberHw::mDefaultVrfName  = "default";
std::string LinkProberHw::mSessionTypeSelf = "NORMAL";
std::string LinkProberHw::mSessionTypePeer = "RX";
std::string LinkProberHw::mKeySeparator    = ":";
std::string LinkProberHw::mUpState = "Up";
std::string LinkProberHw::mDownState = "Down";
//
// ---> LinkProberHw::LinkProberHw(
//    common::MuxPortConfig &muxPortConfig,
//    boost::asio::io_service &ioService,
//    LinkProberStateMachineBase *linkProberStateMachinePtr,
//    mux::MuxPort* muxPort
// ) :
//
// class constructor
//
LinkProberHw::LinkProberHw(
    common::MuxPortConfig &muxPortConfig,
    boost::asio::io_service &ioService,
    LinkProberStateMachineBase *linkProberStateMachinePtr,
    mux::MuxPort* muxPort
) :
    LinkProberBase(muxPortConfig, ioService, linkProberStateMachinePtr),
    mMuxPortPtr(muxPort),
    mSuspendTimer(mIoService),
    mPositiveProbingTimer(mIoService),
    mPositiveProbingPeerTimer(mIoService)
{
    mReportHeartbeatReplyReceivedFuncPtr = boost::bind(
        &LinkProberHw::reportHeartbeatReplyReceivedActiveActive,
        this,
        boost::placeholders::_1
    );
    mReportHeartbeatReplyNotReceivedFuncPtr = boost::bind(
        &LinkProberHw::reportHeartbeatReplyNotReceivedActiveActive,
        this,
        boost::placeholders::_1
    );
}

//
// ---> reportHeartbeatReplyReceivedActiveActive(HeartbeatType heartbeatType);
//
// Used only in case of software peer sessions
//
void LinkProberHw::reportHeartbeatReplyReceivedActiveActive(HeartbeatType heartbeatType)
{
    // sequence numbers are not used by hardware prober
    if (heartbeatType == HeartbeatType::HEARTBEAT_SELF) {
        MUXLOGWARNING(boost::format("Invalid NORMAL hardware session packet recieved, check the cookie!"));
    }
    if (heartbeatType == HeartbeatType::HEARTBEAT_PEER) {
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerActiveEvent());
    }
}

//
// ---> reportHeartbeatReplyNotReceivedActiveActive(HeartbeatType heartbeatType);
//
// Used only for software peer
//
void LinkProberHw::reportHeartbeatReplyNotReceivedActiveActive(HeartbeatType heartbeatType)
{
    mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerUnknownEvent());
}

//
// ---> handleTimeout(boost::system::error_code ec);
//
// handle software peer ICMP timeout
//
void LinkProberHw::handleTimeout(boost::system::error_code errorCode)
{
    MUXLOGTRACE(boost::format("%s: server: %d") %
        mMuxPortConfig.getPortName() %
        mMuxPortConfig.getServerId()
    );

    switch (mPeerType) {
        case SessionType::UNKNOWN:
            // start another cycle of recv
            break;

        case SessionType::SOFTWARE:
            mStream.cancel();
            mReportHeartbeatReplyNotReceivedFuncPtr(HeartbeatType::HEARTBEAT_PEER);

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
            break;

        case SessionType::HARDWARE:
            break;
    }
    startRecv();
    startTimer();
}

//
// ---> startTimer();
//
// start ICMP ECHOREPLY timeout timer for software peer
//
void LinkProberHw::startTimer()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());
    // time out these heartbeats
    mDeadlineTimer.expires_from_now(boost::posix_time::milliseconds(getProbingInterval()));
    mDeadlineTimer.async_wait(mStrand.wrap(boost::bind(
        &LinkProberHw::handleTimeout,
        this,
        boost::asio::placeholders::error
    )));
}

//
// ---> initialize();
//
// intializing reciving thread and triggers creation of new icmp_echo session
//
void LinkProberHw::initialize()
{
    setupSocket();
    setupSocket_pc();
    createIcmpEchoSession(mSessionTypeSelf, getSelfGuidData());
}

//
// ---> startPositiveProbingTimer(std::string hwSessionType);
//
// triggers handling of expiration of positive probing timer
//
void LinkProberHw::startPositiveProbingTimer(std::string hwSessionType)
{
    MUXLOGDEBUG(boost::format("%s: Postive Probing Timer Started, session type-%s") %
            mMuxPortConfig.getPortName() % hwSessionType);
    // time out these heartbeats
    if(hwSessionType == mSessionTypeSelf){
        mPositiveProbingTimer.expires_from_now(boost::posix_time::milliseconds(getProbingInterval() * mMuxPortConfig.getPositiveStateChangeRetryCount()));
        mPositiveProbingTimer.async_wait(mStrand.wrap(boost::bind(
        &LinkProberHw::handlePositiveProbingTimeout,
        this,
        hwSessionType
        )));
    } else if(hwSessionType == mSessionTypePeer){
        mPositiveProbingPeerTimer.expires_from_now(boost::posix_time::milliseconds(getProbingInterval() * mMuxPortConfig.getPositiveStateChangeRetryCount()));
        mPositiveProbingPeerTimer.async_wait(mStrand.wrap(boost::bind(
        &LinkProberHw::handlePositiveProbingTimeout,
        this,
        hwSessionType
        )));
    }
}

//
// ---> handlePositiveProbingTimeout(std::string hwSessionType)();
//
// bring Link prober state machine to active when positive probing timer expires
//
void LinkProberHw::handlePositiveProbingTimeout(std::string hwSessionType)
{
    MUXLOGWARNING(boost::format("%s: Postive Probing Timer Expired, session type-%s") %
            mMuxPortConfig.getPortName() % hwSessionType);
    if(hwSessionType == mSessionTypeSelf)
    {
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpHwSelfEvent());
    } else if(hwSessionType == mSessionTypePeer) {
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpHwPeerActiveEvent());
    }
}

//
// ---> startProbing();
//
// triggers creation of ICMP_ECHO_SESSION_TABLE in APP_DB table to start sending/recieving of ICMP session packets
//
void LinkProberHw::startProbing()
{
    if(mStream.is_open()){
        mStream.cancel();
    }
    if((!mSuspendTx) && (!mShutdownTx))
    {
        createIcmpEchoSession(mSessionTypeSelf, getSelfGuidData());
    }
    startRecv();
    startRecv_pc();
}

//
// ---> handleStateDbStateUpdate(const std::string& session_state, const std::string hwSessionType);
//
// handle state change notification from STATE_DB:ICMP_ECHO_SESSION_TABLE
//
void LinkProberHw::handleStateDbStateUpdate(const std::string& session_state, const std::string hwSessionType)
{
    MUXLOGWARNING(boost::format("%s: Recieved New state %s for icmp_Echo mSuspendTx = %b and mShutdownTx = %b ") %
            mMuxPortConfig.getPortName() % session_state  % mSuspendTx % mShutdownTx);

    if((!mSuspendTx) && (!mShutdownTx))
    {
        if(hwSessionType == mSessionTypeSelf) {
            if(session_state == mUpState) {
                startPositiveProbingTimer(hwSessionType);
            } else if(session_state == mDownState) {
                mPositiveProbingTimer.cancel();
                mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpHwUnknownEvent());
            } else {
                MUXLOGWARNING(boost::format("%s: not a valid state %s for Link Prober") %
                        mMuxPortConfig.getPortName() % session_state);
            }
        } else if (hwSessionType == mSessionTypePeer) {
            if(session_state == mUpState) {
                startPositiveProbingTimer(hwSessionType);
            } else if(session_state == mDownState) {
                mPositiveProbingPeerTimer.cancel();
                mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpHwPeerUnknownEvent());
            } else {
                MUXLOGWARNING(boost::format("%s: not a valid peer state %s for Link Prober ") %
                        mMuxPortConfig.getPortName() % session_state);
            }
        }
    }
    else
    {
        MUXLOGWARNING(boost::format("%s: Recieved State Update %s Even when we are in suspend or shutdown  ") %
        mMuxPortConfig.getPortName() % session_state);
    }
}

//
// ---> createIcmpEchoSession(std::string hwSessionType, std::string guid);
//
//  triggers creation of new icmp_echo_session
//
void LinkProberHw::createIcmpEchoSession(std::string hwSessionType, std::string guid)
{
    MUXLOGDEBUG(boost::format("%s: Creating the Icmp session of type %s with guid {%s}")
                % mMuxPortConfig.getPortName() % hwSessionType % guid);
    auto entries =  std::make_unique<mux::IcmpHwOffloadEntries>();
    std::string portName = mMuxPortConfig.getPortName();
    std::string tx_interval = std::to_string(mMuxPortConfig.getTimeoutIpv4_msec());
    std::string rx_interval  = std::to_string(mMuxPortConfig.getTimeoutIpv4_msec() * mMuxPortConfig.getNegativeStateChangeRetryCount());
    std::string src_ip       = mMuxPortConfig.getLoopbackIpv4Address().to_string();
    std::string dst_ip       = mMuxPortConfig.getBladeIpv4Address().to_string();
    std::string src_mac      = "";
    std::string dst_mac      = etherMacArrayToString(mMuxPortConfig.getBladeMacAddress());

    if (mMuxPortConfig.ifEnableUseTorMac())
    {
        src_mac = etherMacArrayToString(mMuxPortConfig.getTorMacAddress());
    } else {
        src_mac = etherMacArrayToString(mMuxPortConfig.getVlanMacAddress());
    }

    std::string key = mDefaultVrfName +
        mKeySeparator + portName + mKeySeparator + guid + mKeySeparator;

    if (hwSessionType == mSessionTypeSelf) {
        key += mSessionTypeSelf;
    } else {
        key += mSessionTypePeer;
    }

    entries->emplace_back("tx_interval", tx_interval);
    entries->emplace_back("rx_interval", rx_interval);
    entries->emplace_back("session_guid", guid);
    entries->emplace_back("session_cookie", mSessionCookie);
    entries->emplace_back("src_ip", src_ip);
    entries->emplace_back("dst_ip", dst_ip);
    entries->emplace_back("src_mac", src_mac);
    entries->emplace_back("dst_mac", dst_mac);

    mMuxPortPtr->createIcmpEchoSession(key, std::move(entries));
}

//
// ---> initialize();
//
// triggers deletion of icmp_echo_session
//
void LinkProberHw::deleteIcmpEchoSession(std::string hwSessionType, std::string guid)
{
    MUXLOGWARNING(boost::format("%s: Deleting the Icmp session of type %s with guid {%s} ")
                  % mMuxPortConfig.getPortName() % hwSessionType %guid);
    std::string portName = mMuxPortConfig.getPortName();
    std::string key = mDefaultVrfName +
        mKeySeparator + portName + mKeySeparator + guid + mKeySeparator;
    if (hwSessionType == mSessionTypeSelf) {
        key += mSessionTypeSelf;
    } else {
        key += mSessionTypePeer;
    }
    mMuxPortPtr->deleteIcmpEchoSession(key);
}

//
// ---> suspendTxProbes(uint32_t suspendTime_msec);
//
// suspend sending ICMP ECHOREQUEST packets
//
void LinkProberHw::suspendTxProbes(uint32_t suspendTime_msec)
{
    auto guid = getSelfGuidData();
    deleteIcmpEchoSession(mSessionTypeSelf, guid);
    if(mStream_pc.is_open()){
        mStream_pc.cancel();
    }
    mSuspendTx = true;
    mCancelSuspend = false;
    MUXLOGWARNING(boost::format("%s: suspend ICMP heartbeat probing") % mMuxPortConfig.getPortName());
    mSuspendTimer.expires_from_now(boost::posix_time::milliseconds(suspendTime_msec));
    mSuspendTimer.async_wait(mStrand.wrap(boost::bind(
        &LinkProberHw::handleSuspendTimeout,
        this,
        boost::asio::placeholders::error
    )));
}

//
// ---> resumeTxProbes();
//
// resumes sending ICMP ECHOREQUEST packets
//
void LinkProberHw::resumeTxProbes()
{
    mSuspendTimer.cancel();
     MUXLOGWARNING(boost::format("%s: Resume ICMP Probing") %
             mMuxPortConfig.getPortName());
    mCancelSuspend = true;
    startProbing();
}

//
// ---> shutdownTxProbes();
//
// stop sending ICMP ECHOREQUEST packets
//
void LinkProberHw::shutdownTxProbes()
{
    auto guid = getSelfGuidData();
    MUXLOGWARNING(boost::format("%s: Shutdown ICMP Probing") %
            mMuxPortConfig.getPortName());
    deleteIcmpEchoSession(mSessionTypeSelf, guid);
    mShutdownTx = true;
}

//
// ---> restartTxProbes();
//
// first stop sending and then again resume sending ICMP ECHOREQUEST packets
//
void LinkProberHw::restartTxProbes()
{
   auto guid = getSelfGuidData();
   MUXLOGWARNING(boost::format("Restart ICMP Probing"));
   deleteIcmpEchoSession(mSessionTypeSelf, guid);
   mShutdownTx = false;
   startProbing();
}

//
// ---> handleSuspendTimeout(boost::system::error_code errorCode);
//
// when suspendTimer expires start probing again and notify LinkProberStateMachine to get into orignal state
//
void LinkProberHw::handleSuspendTimeout(boost::system::error_code errorCode)
{
    MUXLOGWARNING(boost::format("%s: suspend timeout, resume ICMP heartbeat probing") % mMuxPortConfig.getPortName());
    mSuspendTx = false;
    if (errorCode == boost::system::errc::success || mCancelSuspend) {
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
    mCancelSuspend = false;
}

//
// ---> updateEthernetFrame();
//
// update Ethernet frame of Tx Buffer
//
void LinkProberHw::updateEthernetFrame()
{
    deleteIcmpEchoSession(mSessionTypeSelf, getSelfGuidData());
    createIcmpEchoSession(mSessionTypeSelf, getSelfGuidData());
    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(&LinkProberHw::handleUpdateEthernetFrame, this)));
}

//
// ---> etherMacArrayToString(const std::array<uint8_t, 6>& macAddress);
//
// helper to convert ether array structure of mac address to string format
//
std::string LinkProberHw::etherMacArrayToString(const std::array<uint8_t, 6>& macAddress)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0'); // Set hex formatting with leading zeros

    for (size_t i = 0; i < macAddress.size(); ++i) {
        oss << std::setw(2) << static_cast<int>(macAddress[i]); // Convert each byte to hex
        if (i != macAddress.size() - 1) {
            oss << ":"; // Add colon between bytes except after the last byte
        }
    }

    return oss.str();
}

//
// ---> handleIcmpPayload(size_t bytesTransferred, icmphdr *icmpHeader, IcmpPayload *icmpPayload);
//
// handle Icmp packet recieved on socket
//
void LinkProberHw::handleIcmpPayload(size_t bytesTransferred, icmphdr *icmpHeader, IcmpPayload *icmpPayload)
{
    bool isHwCookie = ntohl(icmpPayload->cookie) == IcmpPayload::getHardwareCookie();
    bool isSwCookie = ntohl(icmpPayload->cookie) == IcmpPayload::getSoftwareCookie();

    if (isHwCookie)
    {
        if ((ntohl(icmpPayload->version) <= IcmpPayload::getVersion()) &&
           (ntohs(icmpHeader->un.echo.id) == 0))
        {
            // echo.id in hw prober must not be set
            MUXLOGTRACE(boost::format("%s: Valid ICMP Packet from %s") %
                mMuxPortConfig.getPortName() %
                mMuxPortConfig.getBladeIpv4Address().to_string()
            );

            std::string guidDataStr;
            getGuidStr(icmpPayload, guidDataStr);

            if (guidDataStr == "0x0") {
                MUXLOGWARNING(boost::format("%s: Received 0x0 GUID") %
                        mMuxPortConfig.getPortName());
                //ignore this and get more packets
                startRecv();
                return;
            }
            bool isSelfGuid = getSelfGuidData() == guidDataStr;
            // sequence numbers are not used by hw prober
            if (!isSelfGuid)
            {
                // Received a peer guid
                MUXLOGWARNING(boost::format("%s: Peer Guid Detected %s") %
                        mMuxPortConfig.getPortName() % guidDataStr);
                // existing peer guid session needs to be deleted, when we learn a new peer session
                if ((mPeerType == SessionType::HARDWARE) && (mPeerGuid != "") &&
                        (mPeerGuid != guidDataStr))
                {
                    deleteIcmpEchoSession(mSessionTypePeer, mPeerGuid);
                    mPositiveProbingPeerTimer.cancel();
                    mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerUnknownEvent());
                }
                // insert new peer guid
                if(mGuidSet.find(guidDataStr) != mGuidSet.end())
                {
                    mGuidSet.erase(mPeerGuid);
                    mGuidSet.insert(guidDataStr);
                }
                mPeerType = LinkProberBase::HARDWARE;
                setPeerGuidData(guidDataStr);
                createIcmpEchoSession(mSessionTypePeer, getPeerGuidData());
            }
        } else {
            MUXLOGWARNING(boost::format("%s: Received invalid packet with hwcookie") %
                    mMuxPortConfig.getPortName());
        }
    } else if (isSwCookie) {
        if ((ntohl(icmpPayload->version) <= IcmpPayload::getVersion()) &&
           (ntohs(icmpHeader->un.echo.id) == mMuxPortConfig.getServerId()))
        {
            MUXLOGTRACE(boost::format("%s: Valid ICMP Packet from %s") %
                mMuxPortConfig.getPortName() %
                mMuxPortConfig.getBladeIpv4Address().to_string()
            );

            // we can get software cookie from only from peer
            std::string guidDataStr;
            getGuidStr(icmpPayload, guidDataStr);
            if (guidDataStr == "0x0") {
                MUXLOGWARNING(boost::format("%s: Received 0x0 GUID from") %
                        mMuxPortConfig.getPortName());
                startRecv();
                return;
            }

            if (guidDataStr == mSelfGuid)
            {
                // ignore this, software generated TLV packets are coming back.
                startRecv();
                return;
            }

            // check peer TLV packets
            bool isTlvPkt = false;
            size_t nextTlvSize = 0;
            auto *tlvPtr = getNextTLVPtr(mTlvStartOffset, bytesTransferred, nextTlvSize);
            if (tlvPtr && tlvPtr->tlvhead.type != TlvType::TLV_SENTINEL)
                isTlvPkt = true;

            // peer transitioned to software we need to delete peer HW session
            if (!isTlvPkt && (mPeerType == SessionType::HARDWARE))
            {
                deleteIcmpEchoSession(mSessionTypePeer, mPeerGuid);
                mGuidSet.erase(mPeerGuid);
                mPositiveProbingPeerTimer.cancel();
                mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpHwPeerUnknownEvent());
                mPeerType = SessionType::SOFTWARE;
            }

            // new peer guid
            if (mPeerGuid != guidDataStr)
            {
                if(mGuidSet.find(guidDataStr) != mGuidSet.end())
                {
                    mGuidSet.insert(guidDataStr);
                }
                setPeerGuidData(guidDataStr);
                mPositiveProbingPeerTimer.cancel();
                mDeadlineTimer.cancel();
                mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerUnknownEvent());
                startRecv();
                startTimer();
                return;
            }
            // seq numbers are not incremented for hw prober
            mReportHeartbeatReplyReceivedFuncPtr(HeartbeatType::HEARTBEAT_PEER);
            handleTlvRecv(bytesTransferred, false);
        } else {
            MUXLOGWARNING(boost::format("%s: Received invalid packet with swcookie") %
                    mMuxPortConfig.getPortName());
        }
    }
    startRecv();
}

//
// ---> handleIcmpPayload_pc(size_t bytesTransferred, icmphdr *icmpHeader, IcmpPayload *icmpPayload);
//
// handle Icmp packet recieved on PortChannel socket
//
void LinkProberHw::handleIcmpPayload_pc(size_t bytesTransferred, icmphdr *icmpHeader, IcmpPayload *icmpPayload)
{
    bool isHwCookie = ntohl(icmpPayload->cookie) == IcmpPayload::getHardwareCookie();
    bool isSwCookie = ntohl(icmpPayload->cookie) == IcmpPayload::getSoftwareCookie();

    if (isHwCookie)
    {
        if ((ntohl(icmpPayload->version) <= IcmpPayload::getVersion()) &&
           (ntohs(icmpHeader->un.echo.id) == 0))
        {
            // echo.id in hw prober must not be set
            std::string guidDataStr;
            getGuidStr(icmpPayload, guidDataStr);

            if (guidDataStr == "0x0") {
                //ignore this and get more packets
                startRecv_pc();
                return;
            }
            bool isSelfGuid = getSelfGuidData() == guidDataStr;
            // sequence numbers are not used by hw prober
            if (!isSelfGuid)
            {
                // Received a peer guid
                // existing peer guid session needs to be deleted, when we learn a new peer session
                if ((mPeerType == SessionType::HARDWARE) && (mPeerGuid != "") &&
                        (mPeerGuid != guidDataStr))
                {
                    deleteIcmpEchoSession(mSessionTypePeer, mPeerGuid);
                    mPositiveProbingPeerTimer.cancel();
                    mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerUnknownEvent());
                }
                // insert new peer guid
                if(mGuidSet.find(guidDataStr) != mGuidSet.end())
                {
                    mGuidSet.erase(mPeerGuid);
                    mGuidSet.insert(guidDataStr);
                }
                mPeerType = LinkProberBase::HARDWARE;
                setPeerGuidData(guidDataStr);
                createIcmpEchoSession(mSessionTypePeer, getPeerGuidData());
            }
        }
    } else if (isSwCookie) {
        if ((ntohl(icmpPayload->version) <= IcmpPayload::getVersion()) &&
           (ntohs(icmpHeader->un.echo.id) == mMuxPortConfig.getServerId()))
        {
            // we can get software cookie from only from peer
            std::string guidDataStr;
            getGuidStr(icmpPayload, guidDataStr);
            if (guidDataStr == "0x0") {
                startRecv_pc();
                return;
            }

            if (guidDataStr == mSelfGuid)
            {
                // ignore this, software generated TLV packets are coming back.
                startRecv_pc();
                return;
            }

            // check peer TLV packets
            bool isTlvPkt = false;
            size_t nextTlvSize = 0;
            auto *tlvPtr = getNextTLVPtr(mTlvStartOffset, bytesTransferred, nextTlvSize);
            if (tlvPtr && tlvPtr->tlvhead.type != TlvType::TLV_SENTINEL)
                isTlvPkt = true;

            // peer transitioned to software we need to delete peer HW session
            if (!isTlvPkt && (mPeerType == SessionType::HARDWARE))
            {
                deleteIcmpEchoSession(mSessionTypePeer, mPeerGuid);
                mGuidSet.erase(mPeerGuid);
                mPositiveProbingPeerTimer.cancel();
                mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpHwPeerUnknownEvent());
                mPeerType = SessionType::SOFTWARE;
            }

            // new peer guid
            if (mPeerGuid != guidDataStr)
            {
                if(mGuidSet.find(guidDataStr) != mGuidSet.end())
                {
                    mGuidSet.insert(guidDataStr);
                }
                setPeerGuidData(guidDataStr);
                mPositiveProbingPeerTimer.cancel();
                mDeadlineTimer.cancel();
                mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerUnknownEvent());
                startRecv_pc();
                startTimer();
                return;
            }
            // seq numbers are not incremented for hw prober
            mReportHeartbeatReplyReceivedFuncPtr(HeartbeatType::HEARTBEAT_PEER);
            handleTlvRecv(bytesTransferred, false);
        }
    }
    startRecv_pc();
}

}
