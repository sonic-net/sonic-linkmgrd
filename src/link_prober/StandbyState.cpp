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
 * StandbyState.cpp
 *
 *  Created on: Oct 7, 2020
 *      Author: Tamer Ahmed
 */

#include "link_prober/ActiveState.h"
#include "link_prober/StandbyState.h"
#include "link_prober/UnknownState.h"
#include "link_prober/LinkProberStateMachineBase.h"

#include "common/MuxLogger.h"

namespace link_prober
{

//
// ---> StandbyState(LinkProberStateMachineBase &stateMachine, common::MuxPortConfig &muxPortConfig);
//
// class constructor
//
StandbyState::StandbyState(
    LinkProberStateMachineBase &stateMachine,
    common::MuxPortConfig &muxPortConfig
) :
    LinkProberState(stateMachine, muxPortConfig)
{
}

//
// ---> handleEvent(IcmpPeerEvent &event);
//
// handle IcmpPeerEvent from LinkProber
//
LinkProberState* StandbyState::handleEvent(IcmpPeerEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState =
        dynamic_cast<LinkProberState *> (stateMachine->getStandbyState());

    resetState();

    return nextState;
}

//
// ---> handleEvent(IcmpSelfEvent &event);
//
// handle IcmpSelfEvent from LinkProber
//
LinkProberState* StandbyState::handleEvent(IcmpSelfEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    mUnknownEventCount = 0;
    if (++mSelfEventCount >= getMuxPortConfig().getPositiveStateChangeRetryCount()) {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getActiveState());
    }
    else {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getStandbyState());
    }

    return nextState;
}

//
// ---> handleEvent(IcmpUnknownEvent &event);
//
// handle IcmpUnknownEvent from LinkProber
//
LinkProberState* StandbyState::handleEvent(IcmpUnknownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    mSelfEventCount = 0;
    if (++mUnknownEventCount >= getMuxPortConfig().getNegativeStateChangeRetryCount()) {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getUnknownState());
    }
    else {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getStandbyState());
    }

    return nextState;
}

//
// ---> handleEvent(LinkProberSelfUpEvent &event);
//
// handle LinkProberSelfUpEvent from LinkProber
//
LinkProberState* StandbyState::handleEvent(LinkProberSelfUpEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState = dynamic_cast<LinkProberState *> (stateMachine->getActiveState());

    return nextState;

}

//
// ---> handleEvent(LinkProberSelfDownEvent &event);
//
// handle LinkProberSelfDownEvent from LinkProber
//
LinkProberState* StandbyState::handleEvent(LinkProberSelfDownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState = dynamic_cast<LinkProberState *> (stateMachine->getStandbyState());

    return nextState;
}

//
// ---> handleEvent(LinkProberPeerUpEvent &event);
//
// handle LinkProberPeerUpEvent from LinkProber
//
LinkProberState* StandbyState::handleEvent(LinkProberPeerUpEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState = dynamic_cast<LinkProberState *> (stateMachine->getStandbyState());

    return nextState;
}

//
// ---> handleEvent(LinkProberPeerDownEvent &event);
//
// handle LinkProberPeerDownEvent from LinkProber
//
LinkProberState* StandbyState::handleEvent(LinkProberPeerDownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState = dynamic_cast<LinkProberState *> (stateMachine->getUnknownState());

    return nextState;
}

//
// ---> resetState();
//
// reset current state attributes
//
void StandbyState::resetState()
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    mSelfEventCount = 0;
    mUnknownEventCount = 0;
}

} /* namespace link_prober */
