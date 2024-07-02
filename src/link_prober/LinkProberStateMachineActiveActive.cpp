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

#include <boost/bind/bind.hpp>

#include "LinkProberState.h"
#include "common/MuxLogger.h"
#include "link_manager/LinkManagerStateMachineBase.h"
#include "link_prober/LinkProberStateMachineActiveActive.h"

namespace link_prober
{
//
// ---> LinkProberStateMachineActiveActive(
//          link_manager::LinkManagerStateMachineBase &linkManagerStateMachinePtr,
//          boost::asio::io_service::strand &strand,
//          common::MuxPortConfig &muxPortConfig,
//          LinkProberState::Label label
//      );
//
// class constructor
//
LinkProberStateMachineActiveActive::LinkProberStateMachineActiveActive(
    link_manager::LinkManagerStateMachineBase *linkManagerStateMachinePtr,
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig,
    LinkProberState::Label label
)
    : LinkProberStateMachineBase(linkManagerStateMachinePtr, strand, muxPortConfig)
{
    enterState(label);
    enterPeerState(LinkProberState::Label::PeerWait);
}

//
// ---> enterState(LinkProberState::Label label);
//
// force the state machine to enter a given state
//
void LinkProberStateMachineActiveActive::enterState(LinkProberState::Label label)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    switch (label) {
        case LinkProberState::Label::Active:
            setCurrentState(dynamic_cast<LinkProberState *>(getActiveState()));
            break;
        case LinkProberState::Label::Unknown:
            setCurrentState(dynamic_cast<LinkProberState *>(getUnknownState()));
            break;
        case LinkProberState::Label::Wait:
            setCurrentState(dynamic_cast<LinkProberState *>(getWaitState()));
            break;
        default:
            break;
    }
}

//
// ---> getCurrentPeerState();
//
// getter for current peer state
//
LinkProberState *LinkProberStateMachineActiveActive::getCurrentPeerState()
{
    return mCurrentPeerState;
}

//
// ---> enterPeerState(LinkProberState::Label label);
//
// force the state machine to enter a given peer state
//
void LinkProberStateMachineActiveActive::enterPeerState(LinkProberState::Label label)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    switch (label) {
        case LinkProberState::Label::PeerActive:
            setCurrentPeerState(dynamic_cast<LinkProberState *>(getPeerActiveState()));
            break;
        case LinkProberState::Label::PeerUnknown:
            setCurrentPeerState(dynamic_cast<LinkProberState *>(getPeerUnknownState()));
            break;
        case LinkProberState::Label::PeerWait:
            setCurrentPeerState(dynamic_cast<LinkProberState *>(getPeerWaitState()));
            break;
        default:
            break;
    }
}

//
// ---> setCurrentPeerState(LinkProberState *state);
//
// setter for current peer state
//
void LinkProberStateMachineActiveActive::setCurrentPeerState(LinkProberState *state)
{
    if (mCurrentPeerState != state) {
        mCurrentPeerState = state;
        mCurrentPeerState->resetState();
    }
}

//
// ---> processEvent(IcmpPeerActiveEvent &icmpPeerActiveEvent);
//
// process IcmpPeerActiveEvent
//
void LinkProberStateMachineActiveActive::processEvent(IcmpPeerActiveEvent &icmpPeerActiveEvent)
{
    LinkProberState *currentPeerState = getCurrentPeerState();
    LinkProberState *nextPeerState = currentPeerState->handleEvent(icmpPeerActiveEvent);
    if (nextPeerState == nullptr) {
        MUXLOGERROR(
            boost::format(
                "%s: link prober state %d could not handle event"
            ) %
            mMuxPortConfig.getPortName() %
            currentPeerState->getStateLabel()
        );
    } else {
        if (nextPeerState != currentPeerState) {
            postLinkManagerPeerEvent(nextPeerState);
        }
        setCurrentPeerState(nextPeerState);
    }
}

//
// ---> processEvent(IcmpPeerUnknownEvent &icmpPeerActiveEvent);
//
// process IcmpPeerUnknownEvent
//
void LinkProberStateMachineActiveActive::processEvent(IcmpPeerUnknownEvent &IcmpPeerUnknownEvent)
{
    LinkProberState *currentPeerState = getCurrentPeerState();
    LinkProberState *nextPeerState = currentPeerState->handleEvent(IcmpPeerUnknownEvent);
    if (nextPeerState == nullptr) {
        MUXLOGERROR(
            boost::format(
                "%s: link prober state %d could not handle event"
            ) %
            mMuxPortConfig.getPortName() %
            currentPeerState->getStateLabel()
        );
    } else {
        if (nextPeerState != currentPeerState) {
            postLinkManagerPeerEvent(nextPeerState);
        }
        setCurrentPeerState(nextPeerState);
    }
}

//
// ---> processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent);
//
// process LinkProberState suspend timer expiry event
//
void LinkProberStateMachineActiveActive::processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachinePtr->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleSuspendTimerExpiry,
        mLinkManagerStateMachinePtr
    )));
}

//
// ---> processEvent(MuxProbeRequestEvent &muxProbeRequestEvent);
//
// process LinkProberState mux probe request event
//
void LinkProberStateMachineActiveActive::processEvent(MuxProbeRequestEvent &muxProbeRequestEvent)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachinePtr->getStrand();
    boost::asio::post(mStrand, boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleMuxProbeRequestEvent,
        mLinkManagerStateMachinePtr
    ));
}


//
// ---> postLinkManagerPeerEvent(LinkProberState* linkProberState);
//
// post LinkProberState peer change event to LinkManager state machine
//
inline void LinkProberStateMachineActiveActive::postLinkManagerPeerEvent(LinkProberState *linkProberState)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachinePtr->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (link_manager::LinkManagerStateMachineBase::*)(link_manager::LinkProberEvent &, LinkProberState::Label)>(
            &link_manager::LinkManagerStateMachineBase::handlePeerStateChange
        ),
        mLinkManagerStateMachinePtr,
        link_manager::LinkManagerStateMachineBase::getLinkProberEvent(),
        linkProberState->getStateLabel()
    )));
}

// 
// ---> handlePckLossRatioUpdate(const uint64_t unknownEventCount, const uint64_t expectedPacketCount);
//
// post pck loss ratio update to link manager
//
void LinkProberStateMachineActiveActive::handlePckLossRatioUpdate(const uint64_t unknownEventCount, const uint64_t expectedPacketCount) 
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachinePtr->getStrand();
    boost::asio::post(strand, boost::bind(
        &link_manager::LinkManagerStateMachineBase::handlePostPckLossRatioNotification,
        mLinkManagerStateMachinePtr,
        unknownEventCount,
        expectedPacketCount
    ));
}

//
// ---> LinkProberStateMachineBase::processEvent(LinkProberPeerUpEvent &t);
//
// process LinkProberState LinkProberPeerUpEvent
//
void LinkProberStateMachineActiveActive::processEvent(LinkProberPeerUpEvent &linkProberPeerUpEvent)
{
    LinkProberState *currentPeerState = getCurrentPeerState();
    LinkProberState *nextPeerState = currentPeerState->handleEvent(linkProberPeerUpEvent);
    if (__builtin_expect((nextPeerState == nullptr), 1)) {

        MUXLOGERROR(
            boost::format(
                "%s: link prober state %d could not handle event"
            ) %
            mMuxPortConfig.getPortName() %
            currentPeerState->getStateLabel()
        );
    } else {
        if (nextPeerState != currentPeerState) {
            postLinkManagerPeerEvent(nextPeerState);
        }
        setCurrentPeerState(nextPeerState);
    }
}

//
// ---> LinkProberStateMachineBase::processEvent(LinkProberPeerDownEvent &t);
//
// process LinkProberState LinkProberPeerDownEvent
//
void LinkProberStateMachineActiveActive::processEvent(LinkProberPeerDownEvent &linkProberPeerDownEvent)
{
    LinkProberState *currentPeerState = getCurrentPeerState();
    LinkProberState *nextPeerState = currentPeerState->handleEvent(linkProberPeerDownEvent);
    if (__builtin_expect((nextPeerState == nullptr), 1)) {
        MUXLOGERROR(
            boost::format(
                "%s: link prober state %d could not handle event"
            ) %
            mMuxPortConfig.getPortName() %
            currentPeerState->getStateLabel()
        );
    } else {
        if (nextPeerState != currentPeerState) {
            postLinkManagerPeerEvent(nextPeerState);
        }
        setCurrentPeerState(nextPeerState);
    }
}

} /* namespace link_prober */
