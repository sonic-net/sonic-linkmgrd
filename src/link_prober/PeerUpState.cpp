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

#include "link_prober/PeerInitState.h"
#include "link_prober/PeerUpState.h"
#include "link_prober/PeerDownState.h"
#include "link_prober/LinkProberStateMachineBase.h"

#include "common/MuxLogger.h"

namespace link_prober
{

//
// ---> PeerUpState(LinkProberStateMachineBase &stateMachine, common::MuxPortConfig &muxPortConfig);
//
// class constructor
//
PeerUpState::PeerUpState(
    LinkProberStateMachineBase &stateMachine,
    common::MuxPortConfig &muxPortConfig
) :
    LinkProberState(stateMachine, muxPortConfig)
{
}

//
// ---> handleEvent(IcmpPeerActiveEvent &event);
//
// handle IcmpPeerActiveEvent from LinkProber
//
LinkProberState* PeerUpState::handleEvent(IcmpPeerActiveEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState = dynamic_cast<LinkProberState *> (stateMachine->getPeerUpState());

    resetState();

    return nextState;
}

//
// ---> handleEvent(IcmpPeerUnknownEvent &event);
//
// handle IcmpPeerUnknownEvent from LinkProber
//
LinkProberState* PeerUpState::handleEvent(IcmpPeerUnknownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    if (++mPeerUnknownEvent >= getMuxPortConfig().getNegativeStateChangeRetryCount()) {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getPeerDownState());
    }
    else {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getPeerUpState());
    }

    return nextState;
}

//
// ---> resetState();
//
// reset current state attributes
//
void PeerUpState::resetState()
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mPeerUnknownEvent = 0;
}

} /* namespace link_prober */
