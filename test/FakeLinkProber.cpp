/*
 * FakeLinkProber.cpp
 *
 *  Created on: Oct 23, 2020
 *      Author: tamer
 */

#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include "FakeLinkProber.h"
#include "common/MuxLogger.h"

namespace test
{

FakeLinkProber::FakeLinkProber(
    link_prober::LinkProberStateMachine *linkProberStateMachine
) :
    mLinkProberStateMachine(linkProberStateMachine)
{
}

template<class E>
void FakeLinkProber::postLinkProberEvent(E &e)
{
    boost::asio::io_service::strand& strand = mLinkProberStateMachine->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (link_prober::LinkProberStateMachine::*) (decltype(e))>
            (&link_prober::LinkProberStateMachine::processEvent<decltype(e)>),
        mLinkProberStateMachine,
        e
    )));
}

template
void FakeLinkProber::postLinkProberEvent<link_prober::IcmpSelfEvent&>(link_prober::IcmpSelfEvent &event);

template
void FakeLinkProber::postLinkProberEvent<link_prober::IcmpPeerEvent&>(link_prober::IcmpPeerEvent &event);

template
void FakeLinkProber::postLinkProberEvent<link_prober::IcmpUnknownEvent&>(link_prober::IcmpUnknownEvent &event);

void FakeLinkProber::postSuspendTimerExpiredEvent()
{
    // inform the composite state machine about Suspend timer expiry
    boost::asio::io_service::strand& strand = mLinkProberStateMachine->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (link_prober::LinkProberStateMachine::*) (link_prober::SuspendTimerExpiredEvent&)>
            (&link_prober::LinkProberStateMachine::processEvent),
        mLinkProberStateMachine,
        link_prober::LinkProberStateMachine::getSuspendTimerExpiredEvent()
    )));
}

void FakeLinkProber::initialize()
{
    MUXLOGINFO("");

    mInitializeCallCount++;
}

void FakeLinkProber::startProbing()
{
    MUXLOGINFO("");

    mStartProbingCallCount++;
}

void FakeLinkProber::updateEthernetFrame()
{
    MUXLOGINFO("");

    mUpdateEthernetFrameCallCount++;
}

void FakeLinkProber::probePeerTor()
{
    MUXLOGINFO("");

    mProbePeerTorCallCount++;
}

void FakeLinkProber::suspendTxProbes(uint32_t suspendTime_msec)
{
    MUXLOGINFO("");

    mSuspendTxProbeCallCount++;
}

void FakeLinkProber::resumeTxProbes()
{
    MUXLOGINFO("");

    mResumeTxProbeCallCount++;
}

void FakeLinkProber::sendPeerSwitchCommand()
{
    MUXLOGINFO("");

    mSendPeerSwitchCommand++;
}

} /* namespace test */
