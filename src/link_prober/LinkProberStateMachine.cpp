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
 * LinkProberStateMachine.cpp
 *
 *  Created on: Oct 7, 2020
 *      Author: tamer
 */

#include <boost/bind/bind.hpp>

#include "link_prober/LinkProberStateMachine.h"
#include "link_manager/LinkManagerStateMachine.h"
#include "common/MuxLogger.h"
#include "LinkProberState.h"

namespace link_prober
{

//
// static members
//
IcmpSelfEvent LinkProberStateMachine::mIcmpSelfEvent;
IcmpPeerEvent LinkProberStateMachine::mIcmpPeerEvent;
IcmpUnknownEvent LinkProberStateMachine::mIcmpUnknownEvent;
SuspendTimerExpiredEvent LinkProberStateMachine::mSuspendTimerExpiredEvent;
SwitchActiveCommandCompleteEvent LinkProberStateMachine::mSwitchActiveCommandCompleteEvent;
SwitchActiveRequestEvent LinkProberStateMachine::mSwitchActiveRequestEvent;

//
// ---> LinkProberStateMachine(
//          link_manager::LinkManagerStateMachine &linkManagerStateMachine,
//          boost::asio::io_service::strand &strand,
//          common::MuxPortConfig &muxPortConfig,
//          LinkProberState::Label label
//      );
//
// class constructor
//
LinkProberStateMachine::LinkProberStateMachine(
    link_manager::LinkManagerStateMachine &linkManagerStateMachine,
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig,
    LinkProberState::Label label
) :
    StateMachine(strand, muxPortConfig),
    mLinkManagerStateMachine(linkManagerStateMachine),
    mActiveState(*this, muxPortConfig),
    mStandbyState(*this, muxPortConfig),
    mUnknownState(*this, muxPortConfig),
    mWaitState(*this, muxPortConfig)
{
    enterState(label);
}

//
// ---> enterState(LinkProberState::Label label);
//
// force the state machine to enter a given state
//
void LinkProberStateMachine::enterState(LinkProberState::Label label)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    switch (label) {
    case LinkProberState::Label::Active:
        setCurrentState(dynamic_cast<LinkProberState *> (getActiveState()));
        break;
    case LinkProberState::Label::Standby:
        setCurrentState(dynamic_cast<LinkProberState *> (getStandbyState()));
        break;
    case LinkProberState::Label::Unknown:
        setCurrentState(dynamic_cast<LinkProberState *> (getUnknownState()));
        break;
    case LinkProberState::Label::Wait:
        setCurrentState(dynamic_cast<LinkProberState *> (getWaitState()));
        break;
    default:
        break;
    }
}

//
// ---> postLinkManagerEvent(LinkProberState* linkProberState);
//
// post LinkProberState change event to LinkManager state machine
//
inline
void LinkProberStateMachine::postLinkManagerEvent(LinkProberState* linkProberState)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachine.getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (link_manager::LinkManagerStateMachine::*) (link_manager::LinkProberEvent&, LinkProberState::Label)>
            (&link_manager::LinkManagerStateMachine::handleStateChange),
        &mLinkManagerStateMachine,
        link_manager::LinkManagerStateMachine::getLinkProberEvent(),
        linkProberState->getStateLabel()
    )));
}

//
// ---> LinkProberStateMachine::postLinkProberStateEvent(E &e);
//
// post LinkProberState event to the state machine
//
template<class E>
void LinkProberStateMachine::postLinkProberStateEvent(E &e)
{
    boost::asio::io_service::strand &strand = getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (LinkProberStateMachine::*) (decltype(e))>
            (&LinkProberStateMachine::processEvent<decltype(e)>),
        this,
        e
    )));
}

//
// ---> LinkProberStateMachine::postLinkProberStateEvent(IcmpSelfEvent &e);
//
// post LinkProberState IcmpSelfEvent to the state machine
//
template
void LinkProberStateMachine::postLinkProberStateEvent<IcmpSelfEvent>(IcmpSelfEvent &event);

//
// ---> LinkProberStateMachine::postLinkProberStateEvent(IcmpPeerEvent &e);
//
// post LinkProberState IcmpPeerEvent to the state machine
//
template
void LinkProberStateMachine::postLinkProberStateEvent<IcmpPeerEvent>(IcmpPeerEvent &event);

//
// ---> LinkProberStateMachine::postLinkProberStateEvent(IcmpUnknownEvent &e);
//
// post LinkProberState IcmpUnknownEvent to the state machine
//
template
void LinkProberStateMachine::postLinkProberStateEvent<IcmpUnknownEvent>(IcmpUnknownEvent &event);

//
// ---> LinkProberStateMachine::processEvent(T &t);
//
// process LinkProberState event
//
template <typename T>
void LinkProberStateMachine::processEvent(T &t)
{
    LinkProberState *currentLinkProberState = dynamic_cast<LinkProberState *> (getCurrentState());
    LinkProberState *nextLinkProberState = currentLinkProberState->handleEvent(t);
    if (nextLinkProberState != currentLinkProberState) {
        postLinkManagerEvent(nextLinkProberState);
    }
    setCurrentState(nextLinkProberState);
}

//
// ---> LinkProberStateMachine::processEvent(IcmpSelfEvent &t);
//
// process LinkProberState IcmpSelfEvent
//
template
void LinkProberStateMachine::processEvent<IcmpSelfEvent&>(IcmpSelfEvent &event);

//
// ---> LinkProberStateMachine::processEvent(IcmpPeerEvent &t);
//
// process LinkProberState IcmpPeerEvent
//
template
void LinkProberStateMachine::processEvent<IcmpPeerEvent&>(IcmpPeerEvent &event);

//
// ---> LinkProberStateMachine::processEvent(IcmpUnknownEvent &t);
//
// process LinkProberState IcmpUnknownEvent
//
template
void LinkProberStateMachine::processEvent<IcmpUnknownEvent&>(IcmpUnknownEvent &event);

//
// ---> processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent);
//
// process LinkProberState suspend timer expiry event
//
void LinkProberStateMachine::processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachine.getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachine::handleSuspendTimerExpiry,
        &mLinkManagerStateMachine
    )));
}

//
// ---> processEvent(SwitchActiveCommandCompleteEvent &switchActiveCommandCompleteEvent);
//
// process LinkProberState switch active complete event
//
void LinkProberStateMachine::processEvent(SwitchActiveCommandCompleteEvent &switchActiveCommandCompleteEvent)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachine.getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachine::handleSwitchActiveCommandCompletion,
        &mLinkManagerStateMachine
    )));
}

//
// ---> processEvent(SwitchActiveRequestEvent &switchActiveRequestEvent);
//
// process LinkProberState switch active request event
//
void LinkProberStateMachine::processEvent(SwitchActiveRequestEvent &switchActiveRequestEvent)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachine.getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachine::handleSwitchActiveRequestEvent,
        &mLinkManagerStateMachine
    )));
}

//
// ---> handleMackAddressUpdate(const std::array<uint8_t, ETHER_ADDR_LEN> address);
//
// process LinkProberState MAC address update event
//
void LinkProberStateMachine::handleMackAddressUpdate(const std::array<uint8_t, ETHER_ADDR_LEN> address)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachine.getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachine::handleGetServerMacAddressNotification,
        &mLinkManagerStateMachine,
        address
    )));
}

} /* namespace link_prober */
