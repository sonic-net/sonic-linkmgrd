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
// ---> LinkProberStateMachine(
//          link_manager::LinkManagerStateMachineBase &linkManagerStateMachinePtr,
//          boost::asio::io_service::strand &strand,
//          common::MuxPortConfig &muxPortConfig,
//          LinkProberState::Label label
//      );
//
// class constructor
//
LinkProberStateMachine::LinkProberStateMachine(
    link_manager::LinkManagerStateMachineBase *linkManagerStateMachinePtr,
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig,
    LinkProberState::Label label
) :
    LinkProberStateMachineBase(linkManagerStateMachinePtr, strand, muxPortConfig),
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
// ---> processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent);
//
// process LinkProberState suspend timer expiry event
//
void LinkProberStateMachine::processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent)
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
void LinkProberStateMachine::processEvent(SwitchActiveCommandCompleteEvent &switchActiveCommandCompleteEvent)
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
void LinkProberStateMachine::processEvent(SwitchActiveRequestEvent &switchActiveRequestEvent)
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
void LinkProberStateMachine::handleMackAddressUpdate(const std::array<uint8_t, ETHER_ADDR_LEN> address)
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
void LinkProberStateMachine::handlePckLossRatioUpdate(const uint64_t unknownEventCount, const uint64_t expectedPacketCount) 
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
