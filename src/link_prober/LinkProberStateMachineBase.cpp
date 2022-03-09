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
#include "link_prober/LinkProberStateMachineBase.h"

#include <boost/bind/bind.hpp>

#include "common/MuxLogger.h"
#include "link_manager/LinkManagerStateMachine.h"
#include "link_prober/LinkProberState.h"

namespace link_prober
{

//
// static members
//
IcmpSelfEvent LinkProberStateMachineBase::mIcmpSelfEvent;
IcmpPeerEvent LinkProberStateMachineBase::mIcmpPeerEvent;
IcmpUnknownEvent LinkProberStateMachineBase::mIcmpUnknownEvent;
SuspendTimerExpiredEvent LinkProberStateMachineBase::mSuspendTimerExpiredEvent;
SwitchActiveCommandCompleteEvent LinkProberStateMachineBase::mSwitchActiveCommandCompleteEvent;
SwitchActiveRequestEvent LinkProberStateMachineBase::mSwitchActiveRequestEvent;

LinkProberStateMachineBase::LinkProberStateMachineBase(
    link_manager::LinkManagerStateMachineBase *linkManagerStateMachinePtr,
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig
) :
    StateMachine(strand, muxPortConfig),
    mLinkManagerStateMachinePtr(linkManagerStateMachinePtr)
{
}

//
// ---> LinkProberStateMachineBase::postLinkProberStateEvent(E &e);
//
// post LinkProberState event to the state machine
//
template<class E>
void LinkProberStateMachineBase::postLinkProberStateEvent(E &e)
{
    boost::asio::io_service::strand &strand = getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (LinkProberStateMachineBase::*) (decltype(e))>
            (&LinkProberStateMachineBase::processEvent<decltype(e)>),
        this,
        e
    )));
}

//
// ---> LinkProberStateMachineBase::postLinkProberStateEvent(IcmpSelfEvent &e);
//
// post LinkProberState IcmpSelfEvent to the state machine
//
template
void LinkProberStateMachineBase::postLinkProberStateEvent<IcmpSelfEvent>(IcmpSelfEvent &event);

//
// ---> LinkProberStateMachineBase::postLinkProberStateEvent(IcmpPeerEvent &e);
//
// post LinkProberState IcmpPeerEvent to the state machine
//
template
void LinkProberStateMachineBase::postLinkProberStateEvent<IcmpPeerEvent>(IcmpPeerEvent &event);

//
// ---> LinkProberStateMachineBase::postLinkProberStateEvent(IcmpUnknownEvent &e);
//
// post LinkProberState IcmpUnknownEvent to the state machine
//
template
void LinkProberStateMachineBase::postLinkProberStateEvent<IcmpUnknownEvent>(IcmpUnknownEvent &event);

//
// ---> LinkProberStateMachineBase::processEvent(T &t);
//
// process LinkProberState event
//
template <typename T>
void LinkProberStateMachineBase::processEvent(T &t)
{
    LinkProberState *currentLinkProberState = dynamic_cast<LinkProberState *> (getCurrentState());
    LinkProberState *nextLinkProberState = currentLinkProberState->handleEvent(t);
    if (nextLinkProberState != currentLinkProberState) {
        postLinkManagerEvent(nextLinkProberState);
    }
    setCurrentState(nextLinkProberState);
}

//
// ---> LinkProberStateMachineBase::processEvent(IcmpSelfEvent &t);
//
// process LinkProberState IcmpSelfEvent
//
template
void LinkProberStateMachineBase::processEvent<IcmpSelfEvent&>(IcmpSelfEvent &event);

//
// ---> LinkProberStateMachineBase::processEvent(IcmpPeerEvent &t);
//
// process LinkProberState IcmpPeerEvent
//
template
void LinkProberStateMachineBase::processEvent<IcmpPeerEvent&>(IcmpPeerEvent &event);

//
// ---> LinkProberStateMachineBase::processEvent(IcmpUnknownEvent &t);
//
// process LinkProberState IcmpUnknownEvent
//
template
void LinkProberStateMachineBase::processEvent<IcmpUnknownEvent&>(IcmpUnknownEvent &event);

//
// ---> processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent);
//
// process LinkProberState suspend timer expiry event
//
void LinkProberStateMachineBase::processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
}

//
// ---> processEvent(SwitchActiveCommandCompleteEvent &switchActiveCommandCompleteEvent);
//
// process LinkProberState switch active complete event
//
void LinkProberStateMachineBase::processEvent(SwitchActiveCommandCompleteEvent &switchActiveCommandCompleteEvent)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
}

//
// ---> processEvent(SwitchActiveRequestEvent &switchActiveRequestEvent);
//
// process LinkProberState switch active request event
//
void LinkProberStateMachineBase::processEvent(SwitchActiveRequestEvent &switchActiveRequestEvent)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
}

//
// ---> handleMackAddressUpdate(const std::array<uint8_t, ETHER_ADDR_LEN> address);
//
// process LinkProberState MAC address update event
//
void LinkProberStateMachineBase::handleMackAddressUpdate(const std::array<uint8_t, ETHER_ADDR_LEN> address)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
}

//
// ---> handlePckLossRatioUpdate(const uint64_t unknownEventCount, const uint64_t expectedPacketCount);
//
// post pck loss ratio update to link manager
//
void LinkProberStateMachineBase::handlePckLossRatioUpdate(const uint64_t unknownEventCount, const uint64_t expectedPacketCount)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
}

//
// ---> postLinkManagerEvent(LinkProberState* linkProberState);
//
// post LinkProberState change event to LinkManager state machine
//
inline
void LinkProberStateMachineBase::postLinkManagerEvent(LinkProberState* linkProberState)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachinePtr->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (link_manager::LinkManagerStateMachineBase::*) (link_manager::LinkProberEvent&, LinkProberState::Label)>
            (&link_manager::LinkManagerStateMachineBase::handleStateChange),
        mLinkManagerStateMachinePtr,
        link_manager::LinkManagerStateMachineBase::getLinkProberEvent(),
        linkProberState->getStateLabel()
    )));
}

} /* namespace link_prober */
