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
 * LinkProberStateMachineActiveStandby.cpp
 *
 *  Created on: Oct 7, 2020
 *      Author: tamer
 */

#include <boost/bind/bind.hpp>

#include "link_prober/LinkProberStateMachineActiveStandby.h"
#include "link_manager/LinkManagerStateMachineBase.h"
#include "common/MuxLogger.h"
#include "LinkProberState.h"

namespace link_prober
{
//
// ---> LinkProberStateMachineActiveStandby(
//          link_manager::LinkManagerStateMachineBase &linkManagerStateMachinePtr,
//          boost::asio::io_service::strand &strand,
//          common::MuxPortConfig &muxPortConfig,
//          LinkProberState::Label label
//      );
//
// class constructor
//
LinkProberStateMachineActiveStandby::LinkProberStateMachineActiveStandby(
    link_manager::LinkManagerStateMachineBase *linkManagerStateMachinePtr,
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig,
    LinkProberState::Label label
) :
    LinkProberStateMachineBase(linkManagerStateMachinePtr, strand, muxPortConfig)
{
    enterState(label);
}

//
// ---> enterState(LinkProberState::Label label);
//
// force the state machine to enter a given state
//
void LinkProberStateMachineActiveStandby::enterState(LinkProberState::Label label)
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
// ---> LinkProberStateMachineBase::processEvent(LinkProberPeerUpEvent &t);
//
// process LinkProberState LinkProberPeerUpEvent
//
void LinkProberStateMachineActiveStandby::processEvent(LinkProberPeerUpEvent &linkProberPeerUpEvent)
{
    LinkProberState *currentState = dynamic_cast<LinkProberState *> (getCurrentState());
    LinkProberState *nextState = currentState->handleEvent(linkProberPeerUpEvent);
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
            postLinkManagerEvent(nextState);
        }
        setCurrentState(nextState);
    }
}

//
// ---> LinkProberStateMachineBase::processEvent(LinkProberPeerDownEvent &t);
//
// process LinkProberState LinkProberPeerDownEvent
//
void LinkProberStateMachineActiveStandby::processEvent(LinkProberPeerDownEvent &linkProberPeerDownEvent)
{
    LinkProberState *currentState = dynamic_cast<LinkProberState *> (getCurrentState());
    LinkProberState *nextState = currentState->handleEvent(linkProberPeerDownEvent);
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
            postLinkManagerEvent(nextState);
        }
        setCurrentState(nextState);
    }
}

//
// ---> processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent);
//
// process LinkProberState suspend timer expiry event
//
void LinkProberStateMachineActiveStandby::processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachinePtr->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleSuspendTimerExpiry,
        mLinkManagerStateMachinePtr
    )));
}

//
// ---> processEvent(SwitchActiveCommandCompleteEvent &switchActiveCommandCompleteEvent);
//
// process LinkProberState switch active complete event
//
void LinkProberStateMachineActiveStandby::processEvent(SwitchActiveCommandCompleteEvent &switchActiveCommandCompleteEvent)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachinePtr->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleSwitchActiveCommandCompletion,
        mLinkManagerStateMachinePtr
    )));
}

//
// ---> processEvent(SwitchActiveRequestEvent &switchActiveRequestEvent);
//
// process LinkProberState switch active request event
//
void LinkProberStateMachineActiveStandby::processEvent(SwitchActiveRequestEvent &switchActiveRequestEvent)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachinePtr->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleSwitchActiveRequestEvent,
        mLinkManagerStateMachinePtr
    )));
}

//
// ---> handleMackAddressUpdate(const std::array<uint8_t, ETHER_ADDR_LEN> address);
//
// process LinkProberState MAC address update event
//
void LinkProberStateMachineActiveStandby::handleMackAddressUpdate(const std::array<uint8_t, ETHER_ADDR_LEN> address)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachinePtr->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handleGetServerMacAddressNotification,
        mLinkManagerStateMachinePtr,
        address
    )));
}

// 
// ---> handlePckLossRatioUpdate(const uint64_t unknownEventCount, const uint64_t expectedPacketCount);
//
// post pck loss ratio update to link manager
//
void LinkProberStateMachineActiveStandby::handlePckLossRatioUpdate(const uint64_t unknownEventCount, const uint64_t expectedPacketCount) 
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachinePtr->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        &link_manager::LinkManagerStateMachineBase::handlePostPckLossRatioNotification,
        mLinkManagerStateMachinePtr,
        unknownEventCount,
        expectedPacketCount
    )));
}

} /* namespace link_prober */
