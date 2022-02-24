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
 * MuxStateMachine.cpp
 *
 *  Created on: Oct 20, 2020
 *      Author: Tamer Ahmed
 */

#include <boost/bind/bind.hpp>

#include "mux_state/MuxStateMachine.h"
#include "link_manager/LinkManagerStateMachine.h"
#include "common/MuxLogger.h"
#include "MuxState.h"

namespace mux_state
{
//
// static members
//
ActiveEvent MuxStateMachine::mActiveEvent;
StandbyEvent MuxStateMachine::mStandbyEvent;
UnknownEvent MuxStateMachine::mUnknownEvent;
ErrorEvent MuxStateMachine::mErrorEvent;

//
// ---> MuxStateMachine(
//          link_manager::LinkManagerStateMachineBase &linkManagerStateMachinePtr,
//          boost::asio::io_service::strand &strand,
//          common::MuxPortConfig &muxPortConfig,
//          MuxState::Label label
//      );
//
// class constructor
//
MuxStateMachine::MuxStateMachine(
    link_manager::LinkManagerStateMachineBase *linkManagerStateMachinePtr,
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig,
    MuxState::Label label
) :
    common::StateMachine(strand, muxPortConfig),
    mLinkManagerStateMachinePtr(linkManagerStateMachinePtr),
    mActiveState(*this, muxPortConfig),
    mStandbyState(*this, muxPortConfig),
    mUnknownState(*this, muxPortConfig),
    mErrorState(*this, muxPortConfig),
    mWaitState(*this, muxPortConfig)
{
    enterState(label);
}

//
// ---> enterState(MuxState::Label label);
//
// force the state machine to enter a given state
//
void MuxStateMachine::enterState(MuxState::Label label)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    switch (label) {
    case MuxState::Label::Active:
        setCurrentState(dynamic_cast<MuxState *> (getActiveState()));
        break;
    case MuxState::Label::Standby:
        setCurrentState(dynamic_cast<MuxState *> (getStandbyState()));
        break;
    case MuxState::Label::Unknown:
        setCurrentState(dynamic_cast<MuxState *> (getUnknownState()));
        break;
    case MuxState::Label::Error:
        setCurrentState(dynamic_cast<MuxState *> (getErrorState()));
        break;
    case MuxState::Label::Wait:
        setCurrentState(dynamic_cast<MuxState *> (getWaitState()));
        break;
    default:
        break;
    }
}

//
// ---> postLinkManagerEvent(MuxState* muxState);
//
// post MuxState change event to LinkManager state machine
//
inline
void MuxStateMachine::postLinkManagerEvent(MuxState* muxState)
{
    boost::asio::io_service::strand &strand = mLinkManagerStateMachinePtr->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (link_manager::LinkManagerStateMachineBase::*) (link_manager::MuxStateEvent&, MuxState::Label)>
            (&link_manager::LinkManagerStateMachineBase::handleStateChange),
        mLinkManagerStateMachinePtr,
        link_manager::LinkManagerStateMachineBase::getMuxStateEvent(),
        muxState->getStateLabel()
    )));
}

//
// ---> postMuxStateEvent(E &e);
//
// post MuxState event to the state machine
//
template <class E>
void MuxStateMachine::postMuxStateEvent(E &e)
{
    boost::asio::io_service::strand &strand = getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (MuxStateMachine::*) (decltype(e))>
            (&MuxStateMachine::processEvent),
        this,
        e
    )));
}

//
// ---> postMuxStateEvent<ActiveEvent>(ActiveEvent &e);
//
// post MuxState event to the state machine
//
template
void MuxStateMachine::postMuxStateEvent<ActiveEvent>(ActiveEvent &e);

//
// ---> postMuxStateEvent<StandbyEvent>(StandbyEvent &e);
//
// post MuxState event to the state machine
//
template
void MuxStateMachine::postMuxStateEvent<StandbyEvent>(StandbyEvent &e);

//
// ---> postMuxStateEvent<UnknownEvent>(UnknownEvent &e);
//
// post MuxState event to the state machine
//
template
void MuxStateMachine::postMuxStateEvent<UnknownEvent>(UnknownEvent &e);

//
// ---> postMuxStateEvent<ErrorEvent>(ErrorEvent &e);
//
// post MuxState event to the state machine
//
template
void MuxStateMachine::postMuxStateEvent<ErrorEvent>(ErrorEvent &e);

//
// ---> processEvent(T &t);
//
// process MuxState event
//
template <typename T>
void MuxStateMachine::processEvent(T &t)
{
    MuxState *currentMuxState = dynamic_cast<MuxState *> (getCurrentState());
    MuxState *nextMuxState = currentMuxState->handleEvent(t);
    if (nextMuxState != currentMuxState) {
        postLinkManagerEvent(nextMuxState);
    }
    setCurrentState(nextMuxState);
}

//
// ---> processEvent<ActiveEvent&>(ActiveEvent &event);
//
// process MuxState event
//
template
void MuxStateMachine::processEvent<ActiveEvent&>(ActiveEvent &event);

//
// ---> processEvent<StandbyEvent&>(StandbyEvent &event);
//
// process MuxState event
//
template
void MuxStateMachine::processEvent<StandbyEvent&>(StandbyEvent &event);

//
// ---> processEvent<UnknownEvent&>(UnknownEvent &event);
//
// process MuxState event
//
template
void MuxStateMachine::processEvent<UnknownEvent&>(UnknownEvent &event);

} /* namespace mux_state */
