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
std::string LinkProberHw::mSessionCookie   = "1197829481";
std::string LinkProberHw::mDefaultVrfName  = "Default";
std::string LinkProberHw::mSessionTypeSelf = "NORMAL";
std::string LinkProberHw::mSessionTypePeer = "RX";
std::string LinkProberHw::mKeySeparator    = ":";

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
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpSelfEvent());
    }
    if (heartbeatType == HeartbeatType::HEARTBEAT_PEER) {
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerActiveEvent());
        // duplicate sessions creation will be taken care by IcmpOrch
        createIcmpEchoSession("peer", getPeerGuidData());
    }
}

// 
// ---> reportHeartbeatReplyNotReceivedActiveActive(HeartbeatType heartbeatType);
//
// first time recieve a new type guid reply packet
//
void LinkProberHw::reportHeartbeatReplyNotReceivedActiveActive(HeartbeatType heartbeatType)
{
    if (heartbeatType == HeartbeatType::HEARTBEAT_SELF) {
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpUnknownEvent());
    }
    if (heartbeatType == HeartbeatType::HEARTBEAT_PEER) {
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerUnknownEvent());
    }
}

// 
// ---> initialize();
//
// intializing reciving thread and triggers creation of new icmp_echo session
//
void LinkProberHw::initialize()
{
    startInitRecv();
    startRecv();
    startProbing();
}

// 
// ---> startPositiveProbingTimer(std::string torType);
//
// triggers handling of expiration of positive probing timer 
//
void LinkProberHw::startPositiveProbingTimer(std::string torType)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());
    // time out these heartbeats
    if(torType == "self"){
        mPositiveProbingTimer.expires_from_now(boost::posix_time::milliseconds(getProbingInterval() * mMuxPortConfig.getPositiveStateChangeRetryCount()));
        mPositiveProbingTimer.async_wait(mStrand.wrap(boost::bind(
        &LinkProberHw::handlePositiveProbingTimeout,
        this,
        torType
        )));
    } else if(torType == "peer"){
        mPositiveProbingPeerTimer.expires_from_now(boost::posix_time::milliseconds(getProbingInterval() * mMuxPortConfig.getPositiveStateChangeRetryCount()));
        mPositiveProbingPeerTimer.async_wait(mStrand.wrap(boost::bind(
        &LinkProberHw::handlePositiveProbingTimeout,
        this,
        torType
        )));
    }
}

// 
// ---> handlePositiveProbingTimeout(std::string torType)();
//
// bring Link prober state machine to active when positive probing timer expires
// 
void LinkProberHw::handlePositiveProbingTimeout(std::string torType){
    if(torType == "self"){
        mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpSelfEvent());
    } else if(torType == "peer"){
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
    createIcmpEchoSession("self", getSelfGuidData());
}

// 
// ---> initialize();
//
// handle state change notification from STATE_DB:ICMP_ECHO_SESSION_TABLE
//
void LinkProberHw::handleStateDbStateUpdate(const std::string& state, const std::string torType)
{
    if(torType == "self") {
        if(state == "up") {
            mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpWaitEvent());
            startPositiveProbingTimer(torType);
        } else if(state == "down") {
            mPositiveProbingTimer.cancel();
            mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpUnknownEvent());
        } else {
            MUXLOGWARNING(boost::format("%s: not a valid state for Link Prober") % state);
        }
    } else if (torType == "peer") {
        if(state == "up") {
            mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerWaitEvent());
            startPositiveProbingTimer(torType);
        } else if(state == "down") {
            mPositiveProbingPeerTimer.cancel();
            mLinkProberStateMachinePtr->postLinkProberStateEvent(LinkProberStateMachineBase::getIcmpPeerUnknownEvent());
        } else {
            MUXLOGWARNING(boost::format("%s: not a valid peer state for Link Prober ") % state);
        }
    }
}

// 
// ---> initialize();
//
// intializing reciving thread and triggers creation of new icmp_echo session
//
void LinkProberHw::createIcmpEchoSession(std::string torType, std::string guid)
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

    std::string key = mIcmpTableName + mKeySeparator + mDefaultVrfName +
        mKeySeparator + portName + mKeySeparator + guid + mKeySeparator;

    if (torType == "self") {
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
// intializing reciving thread and triggers creation of new icmp_echo session
//
void LinkProberHw::deleteIcmpEchoSession(std::string torType, std::string guid)
{
    std::string portName = mMuxPortConfig.getPortName();
    std::string key = mIcmpTableName + mKeySeparator + mDefaultVrfName +
        mKeySeparator + portName + mKeySeparator + guid + mKeySeparator;
    if (torType == "self") {
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
    shutdownTxProbes();
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
    startProbing();
    mSuspendTx = false;
}

// 
// ---> shutdownTxProbes();
//
// stop sending ICMP ECHOREQUEST packets
//
void LinkProberHw::shutdownTxProbes()
{
    auto guid = getSelfGuidData();
    deleteIcmpEchoSession("self", guid);
    mSuspendTx = true;
}

// 
// ---> restartTxProbes();
//
// first stop sending and then again resume sending ICMP ECHOREQUEST packets
//
void LinkProberHw::restartTxProbes()
{
    auto curr_guid = getSelfGuidData();
    deleteIcmpEchoSession("self", curr_guid);
    createIcmpEchoSession("self", curr_guid);
}

// 
// ---> handleSuspendTimeout(boost::system::error_code errorCode);
//
// when suspendTimer expires start probing again and notify LinkProberStateMachine to get into origal state
//
void LinkProberHw::handleSuspendTimeout(boost::system::error_code errorCode)
{
    MUXLOGWARNING(boost::format("%s: suspend timeout, resume ICMP heartbeat probing") % mMuxPortConfig.getPortName());
    createIcmpEchoSession("self", getSelfGuidData());
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
// ---> updateEthernetFrame();
//
// update Ethernet frame of Tx Buffer
//
void LinkProberHw::updateEthernetFrame()
{
    deleteIcmpEchoSession("self", getSelfGuidData());
    startProbing();
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
