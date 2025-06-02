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
 * FakeLinkProber.cpp
 *
 *  Created on: Oct 23, 2020
 *      Author: tamer
 */

#include <common/BoostAsioBehavior.h>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

#include "FakeLinkProber.h"
#include "common/MuxLogger.h"

namespace test
{

FakeLinkProber::FakeLinkProber(
    link_prober::LinkProberStateMachineBase *linkProberStateMachine
) :
    mLinkProberStateMachine(linkProberStateMachine)
{
}

template<class E>
void FakeLinkProber::postLinkProberEvent(E &e)
{
    boost::asio::io_service::strand& strand = mLinkProberStateMachine->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(
        strand.wrap(
            [this, &e]() { mLinkProberStateMachine->processEvent(e); }
        )
    );
}

template
void FakeLinkProber::postLinkProberEvent<link_prober::IcmpSelfEvent&>(link_prober::IcmpSelfEvent &event);

template
void FakeLinkProber::postLinkProberEvent<link_prober::IcmpPeerEvent&>(link_prober::IcmpPeerEvent &event);

template
void FakeLinkProber::postLinkProberEvent<link_prober::IcmpUnknownEvent&>(link_prober::IcmpUnknownEvent &event);

template
void FakeLinkProber::postLinkProberEvent<link_prober::IcmpPeerActiveEvent&>(link_prober::IcmpPeerActiveEvent &event);

template
void FakeLinkProber::postLinkProberEvent<link_prober::IcmpPeerUnknownEvent&>(link_prober::IcmpPeerUnknownEvent &event);

void FakeLinkProber::postSuspendTimerExpiredEvent()
{
    // inform the composite state machine about Suspend timer expiry
    boost::asio::io_service::strand& strand = mLinkProberStateMachine->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (link_prober::LinkProberStateMachineBase::*) (link_prober::SuspendTimerExpiredEvent&)>
            (&link_prober::LinkProberStateMachineBase::processEvent),
        mLinkProberStateMachine,
        link_prober::LinkProberStateMachineBase::getSuspendTimerExpiredEvent()
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

void FakeLinkProber::detectLink()
{
    MUXLOGINFO("");

    mDetectLinkCallCount++;
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

void FakeLinkProber::sendPeerProbeCommand()
{
    MUXLOGINFO("");

    mSendPeerProbeCommand++;
}

void FakeLinkProber::handleStateDbStateUpdate()
{
    MUXLOGINFO("");

    mIcmpEchoSessionStateUpdateCallCount++;
}

void FakeLinkProber::handleSendSwitchCommand()
{
    // inform the composite state machine about command send completion
    boost::asio::io_service::strand& strand = mLinkProberStateMachine->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (link_prober::LinkProberStateMachineBase::*) (link_prober::SwitchActiveCommandCompleteEvent&)>
            (&link_prober::LinkProberStateMachineBase::processEvent),
        mLinkProberStateMachine,
        link_prober::LinkProberStateMachineBase::getSwitchActiveCommandCompleteEvent()
    )));
}

void FakeLinkProber::handleSwitchCommandRecv()
{
    boost::asio::io_service::strand& strand = mLinkProberStateMachine->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (link_prober::LinkProberStateMachineBase::*) (link_prober::SwitchActiveRequestEvent&)>
            (&link_prober::LinkProberStateMachineBase::processEvent),
        mLinkProberStateMachine,
        link_prober::LinkProberStateMachineBase::getSwitchActiveRequestEvent()
    )));
}

void FakeLinkProber::handleMuxProbeCommandRecv()
{
    boost::asio::io_service::strand& strand = mLinkProberStateMachine->getStrand();
    boost::asio::post(strand, boost::bind(
        static_cast<void (link_prober::LinkProberStateMachineBase::*) (link_prober::MuxProbeRequestEvent&)>
            (&link_prober::LinkProberStateMachineBase::processEvent),
        mLinkProberStateMachine,
        link_prober::LinkProberStateMachineBase::getMuxProbeRequestEvent()
    ));
}

void FakeLinkProber::resetIcmpPacketCounts()
{
    MUXLOGINFO("");

    mIcmpUnknownEventCount = 0;
    mIcmpPacketCount = 0;

    boost::asio::io_service::strand &strand = mLinkProberStateMachine->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        &link_prober::LinkProberStateMachineBase::handlePckLossRatioUpdate,
        mLinkProberStateMachine,
        mIcmpUnknownEventCount,
        mIcmpPacketCount
    )));
}

void FakeLinkProber::shutdownTxProbes()
{
    MUXLOGINFO("");

    mShutdownTxProbeCallCount++;
}

void FakeLinkProber::restartTxProbes()
{
    MUXLOGINFO("");
    
    mRestartTxProbeCallCount++;
}

void FakeLinkProber::decreaseProbeIntervalAfterSwitch(uint32_t switchTime_msec)
{
    MUXLOGINFO("");

    mDecreaseIntervalCallCount++;
}

void FakeLinkProber::revertProbeIntervalAfterSwitchComplete()
{
    MUXLOGINFO("");

    mRevertIntervalCallCount++;
}

} /* namespace test */
