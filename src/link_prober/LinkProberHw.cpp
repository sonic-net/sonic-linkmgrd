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
// first time recieve a new type guid reply packet
//
void LinkProberHw::reportHeartbeatReplyReceivedActiveActive(HeartbeatType heartbeatType)
{
    if (heartbeatType == HeartbeatType::HEARTBEAT_SELF) {
        MUXLOGWARNING(boost::format("NORMAL session packet recieved not a valid case."));
    }
    if (heartbeatType == HeartbeatType::HEARTBEAT_PEER) {
        // duplicate sessions creation will be taken care by IcmpOrch
        createIcmpEchoSession(mSessionTypePeer, getPeerGuidData());
    }
}

// 
// ---> reportHeartbeatReplyNotReceivedActiveActive(HeartbeatType heartbeatType);
//
// first time recieve a new type guid reply packet
//
void LinkProberHw::reportHeartbeatReplyNotReceivedActiveActive(HeartbeatType heartbeatType)
{
    MUXLOGWARNING(boost::format("In Hardware Prober we dont get any reply packet"));
}

// 
// ---> initialize();
//
// intializing reciving thread and triggers creation of new icmp_echo session
//
void LinkProberHw::initialize()
{
    setupSocket();
    createIcmpEchoSession(mSessionTypeSelf, getSelfGuidData());
}

// 
// ---> startPositiveProbingTimer(std::string hwSessionType);
//
// triggers handling of expiration of positive probing timer 
//
void LinkProberHw::startPositiveProbingTimer(std::string hwSessionType)
{
    
    MUXLOGDEBUG(mMuxPortConfig.getPortName());
    MUXLOGWARNING(boost::format("%s: Postive Probing Timer Started") % hwSessionType);
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
void LinkProberHw::handlePositiveProbingTimeout(std::string hwSessionType){
     MUXLOGWARNING(boost::format("%s: Postive Probing Timer Expired") % hwSessionType);
    if(hwSessionType == mSessionTypeSelf){
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpSelfEvent());
    } else if(hwSessionType == mSessionTypePeer){
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerActiveEvent());
    }
}

// 
// ---> startProbing();
//
// triggers creation of ICMP_ECHO_SESSION_TABLE in APP_DB table to start sending/recieving of ICMP session packets
//
void LinkProberHw::startProbing()
{
    mStream.cancel();
    if((!mSuspendTx) && (!mShutdownTx)){
       createIcmpEchoSession(mSessionTypeSelf, getSelfGuidData());
    }
    startRecv();
}

// 
// ---> initialize();
//
// handle state change notification from STATE_DB:ICMP_ECHO_SESSION_TABLE
//
void LinkProberHw::handleStateDbStateUpdate(const std::string& session_state, const std::string hwSessionType)
{
    MUXLOGWARNING(boost::format("Recieved New state %s for icmp_Echo mSuspendTx = %b and mShutdownTx = %b ") % session_state  % mSuspendTx % mShutdownTx);

    if((!mSuspendTx) && (!mShutdownTx))
    {
        if(hwSessionType == mSessionTypeSelf) {
            if(session_state == mUpState) {
                startPositiveProbingTimer(hwSessionType);
            } else if(session_state == mDownState) {
                mPositiveProbingTimer.cancel();
                mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpUnknownEvent());
            } else {
                MUXLOGWARNING(boost::format("%s: not a valid state for Link Prober") % session_state);
            }
        } else if (hwSessionType == mSessionTypePeer) {
            if(session_state == mUpState) {
                startPositiveProbingTimer(hwSessionType);
            } else if(session_state == mDownState) {
                mPositiveProbingPeerTimer.cancel();
                mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerUnknownEvent());
            } else {
                MUXLOGWARNING(boost::format("%s: not a valid peer state for Link Prober ") % session_state);
            }
        }
    } 
    else 
    {
        MUXLOGWARNING(boost::format("Stateb: {%s} : Recieved State Update Even when we are in suspend or shutdown  ") %
        session_state);
    }
}

// 
// ---> initialize();
//
// intializing reciving thread and triggers creation of new icmp_echo session
//
void LinkProberHw::createIcmpEchoSession(std::string hwSessionType, std::string guid)
{
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

    mMuxPortConfig.setHardwareSessionKey(key);

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
// intializing reciving thread and triggers creation of new icmp_echo session
//
void LinkProberHw::deleteIcmpEchoSession(std::string hwSessionType, std::string guid)
{
    std::string portName = mMuxPortConfig.getPortName();
    std::string key = mDefaultVrfName +
        mKeySeparator + portName + mKeySeparator + guid + mKeySeparator;
    if (hwSessionType == mSessionTypeSelf) {
        key += mSessionTypeSelf;
    } else {
        key += mSessionTypePeer;
    }
    mMuxPortConfig.removeHardwareSessionKey(key);
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
     MUXLOGWARNING(boost::format("Resume ICMP Probing"));
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
    MUXLOGWARNING(boost::format("Shutdown ICMP Probing"));
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
// when suspendTimer expires start probing again and notify LinkProberStateMachine to get into origal state
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

}
