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

#include "common/MuxLogger.h"
#include "MuxPort.h"
#include "LinkProberState.h"
#include "link_manager/LinkManagerStateMachineBase.h"
#include "link_prober/LinkProberSessionStateMachine.h"

namespace link_prober
{

//
// ---> LinkProberSessionStateMachine(
//          link_manager::LinkManagerStateMachineBase &linkManagerStateMachinePtr,
//          mux::MuxPort *muxPort,
//          boost::asio::io_service::strand &strand,
//          common::MuxPortConfig &muxPortConfig,
//          LinkProberState::Label selfLabel,
//          LinkProberState::Label peerLabel
//      );
//
// class constructor
//
LinkProberSessionStateMachine::LinkProberSessionStateMachine(
    link_manager::LinkManagerStateMachineBase *linkManagerStateMachinePtr,
    mux::MuxPort *muxPort,
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig,
    LinkProberState::Label selfLabel,
    LinkProberState::Label peerLabel
) : LinkProberStateMachineBase(linkManagerStateMachinePtr, strand, muxPortConfig), mMuxPort(muxPort)
{
    enterState(selfLabel);
    enterPeerState(peerLabel);

    mSelfSessionId = getMuxPortConfig().getPortName() + "|" + "SELF";
    mPeerSessionId = getMuxPortConfig().getPortName() + "|" + "PEER";
}

//
// ---> enterState(LinkProberState::Label label);
//
// force the state machine to enter a given state
//
void LinkProberSessionStateMachine::enterState(LinkProberState::Label label)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    switch (label)
    {
        case LinkProberState::Label::SelfInit:
            setCurrentState(dynamic_cast<LinkProberState *>(getSelfInitState()));
            break;
        case LinkProberState::Label::SelfUp:
            setCurrentState(dynamic_cast<LinkProberState *>(getSelfUpState()));
            break;
        case LinkProberState::Label::SelfDown:
            setCurrentState(dynamic_cast<LinkProberState *>(getSelfDownState()));
            break;
        default:
            break;
    }
}

//
// ---> enterPeerState(LinkProberState::Label label);
//
// force the state machine to enter a given peer state
//
void LinkProberSessionStateMachine::enterPeerState(LinkProberState::Label label)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    switch (label)
    {
        case LinkProberState::Label::PeerInit:
            setCurrentPeerState(dynamic_cast<LinkProberState *>(getPeerInitState()));
            break;
        case LinkProberState::Label::PeerUp:
            setCurrentPeerState(dynamic_cast<LinkProberState *>(getPeerUpState()));
            break;
        case LinkProberState::Label::PeerDown:
            setCurrentPeerState(dynamic_cast<LinkProberState *>(getPeerDownState()));
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
void LinkProberSessionStateMachine::setCurrentPeerState(LinkProberState *state)
{
    if (mCurrentPeerState != state) {
        mCurrentPeerState = state;
        mCurrentPeerState->resetState();
    }
}

//
// ---> postLinkProberStateEvent(E &e);
//
// post LinkProberState event to the state machine
//
template<class E>
void LinkProberSessionStateMachine::postLinkProberStateEvent(E &e)
{
    boost::asio::io_service::strand &strand = getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(
        strand.wrap(
            [this, &e]() { processEvent(e); }
        )
    );
}

//
// ---> postLinkProberStateEvent(IcmpSelfEvent &e);
//
// post LinkProberState IcmpSelfEvent to the state machine
//
template
void LinkProberSessionStateMachine::postLinkProberStateEvent<IcmpSelfEvent>(IcmpSelfEvent &event);

//
// ---> postLinkProberStateEvent(IcmpUnknownEvent &e);
//
// post LinkProberState IcmpUnknownEvent to the state machine
//
template
void LinkProberSessionStateMachine::postLinkProberStateEvent<IcmpUnknownEvent>(IcmpUnknownEvent &event);

//
// ---> postLinkProberStateEvent(IcmpPeerActiveEvent &e);
//
// post LinkProberState IcmpPeerActiveEvent to the state machine
//
template
void LinkProberSessionStateMachine::postLinkProberStateEvent<IcmpPeerActiveEvent>(IcmpPeerActiveEvent &event);

//
// ---> postLinkProberStateEvent(IcmpPeerUnknownEvent &e);
//
// post LinkProberState IcmpPeerUnknownEvent to the state machine
//
template
void LinkProberSessionStateMachine::postLinkProberStateEvent<IcmpPeerUnknownEvent>(IcmpPeerUnknownEvent &event);


//
// ---> processEvent(IcmpSelfEvent &icmpSelfEvent);
//
// process LinkProberState IcmpSelfEvent event
//
void LinkProberSessionStateMachine::processEvent(IcmpSelfEvent &icmpSelfEvent)
{
    LinkProberState *currentState = dynamic_cast<LinkProberState *> (getCurrentState());
    LinkProberState *nextState = currentState->handleEvent(icmpSelfEvent);
    if (__builtin_expect((nextState == nullptr), 1)) {
        MUXLOGERROR(
            boost::format(
                "%s: link prober state %d could not handle event"
            ) %
            mMuxPortConfig.getPortName() %
            currentState->getStateLabel()
        );
    } else {
        if (nextState != currentState) {
            postLinkProberSessionNotificationToDb(nextState);
        }
        setCurrentState(nextState);
    }
}

//
// ---> processEvent(IcmpUnknownEvent &icmpUnknownEvent);
//
// process LinkProberState IcmpUnknownEvent event
//
void LinkProberSessionStateMachine::processEvent(IcmpUnknownEvent &icmpUnknownEvent)
{
    LinkProberState *currentState = dynamic_cast<LinkProberState *> (getCurrentState());;
    LinkProberState *nextState = currentState->handleEvent(icmpUnknownEvent);
    if (__builtin_expect((nextState == nullptr), 1)) {
        MUXLOGERROR(
            boost::format(
                "%s: link prober state %d could not handle event"
            ) %
            mMuxPortConfig.getPortName() %
            currentState->getStateLabel()
        );
    } else {
        if (nextState != currentState) {
            postLinkProberSessionNotificationToDb(nextState);
        }
        setCurrentState(nextState);
    }
}

//
// ---> processEvent(IcmpPeerActiveEvent &icmpPeerActiveEvent);
//
// process LinkProberState IcmpPeerActiveEvent event
//
void LinkProberSessionStateMachine::processEvent(IcmpPeerActiveEvent &icmpPeerActiveEvent)
{
    LinkProberState *currentPeerState = getCurrentPeerState();
    LinkProberState *nextPeerState = currentPeerState->handleEvent(icmpPeerActiveEvent);
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
            postLinkProberPeerSessionNotificationToDb(nextPeerState);
        }
        setCurrentPeerState(nextPeerState);
    }
}

//
// ---> processEvent(IcmpPeerUnknownEvent &icmpPeerUnknownEvent);
//
// process LinkProberState IcmpPeerUnknownEvent event
//
void LinkProberSessionStateMachine::processEvent(IcmpPeerUnknownEvent &icmpPeerUnknownEvent)
{
    LinkProberState *currentPeerState = getCurrentPeerState();
    LinkProberState *nextPeerState = currentPeerState->handleEvent(icmpPeerUnknownEvent);
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
            postLinkProberPeerSessionNotificationToDb(nextPeerState);
        }
        setCurrentPeerState(nextPeerState);
    }
}

//
// ---> getCurrentPeerState();
//
// getter for current peer state
//
LinkProberState *LinkProberSessionStateMachine::getCurrentPeerState()
{
    return mCurrentPeerState;
}

//
// ---> postLinkProberSessionNotificationToDb(LinkProberState *state);
//
// post self link prober session change event to DB
//
inline void LinkProberSessionStateMachine::postLinkProberSessionNotificationToDb(LinkProberState *state)
{
    mMuxPort->postLinkProberSessionStateNotificationToDb(mSelfSessionId, state->getStateLabel());
}

//
// ---> postLinkProberPeerSessionNotificationToDb(LinkProberState *state);
//
// post peer link prober session change event to DB
//
inline void LinkProberSessionStateMachine::postLinkProberPeerSessionNotificationToDb(LinkProberState *state)
{
    mMuxPort->postLinkProberSessionStateNotificationToDb(mPeerSessionId, state->getStateLabel());
}

} /* namespace link_prober */
