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
 * ActiveState.cpp
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
// ---> ActiveState(LinkProberStateMachineBase &stateMachine, common::MuxPortConfig &muxPortConfig);
//
// class constructor
//
ActiveState::ActiveState(
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
LinkProberState* ActiveState::handleEvent(IcmpPeerEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    mUnknownEventCount = 0;
    if (++mPeerEventCount >= getMuxPortConfig().getPositiveStateChangeRetryCount()) {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getStandbyState());
    }
    else {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getActiveState());
    }

    return nextState;
}

//
// ---> handleEvent(IcmpSelfEvent &event);
//
// handle IcmpSelfEvent from LinkProber
//
LinkProberState* ActiveState::handleEvent(IcmpSelfEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState = dynamic_cast<LinkProberState *> (stateMachine->getActiveState());

    resetState();

    return nextState;
}

//
// ---> handleEvent(IcmpUnknownEvent &event);
//
// handle IcmpUnknownEvent from LinkProber
//
LinkProberState* ActiveState::handleEvent(IcmpUnknownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    mPeerEventCount = 0;
    if (++mUnknownEventCount >= getMuxPortConfig().getNegativeStateChangeRetryCount())
    {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getUnknownState());
    } else {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getActiveState());
    }

    return nextState;
}

LinkProberState* ActiveState::handleEvent(IcmpWaitEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    nextState = dynamic_cast<LinkProberState *> (stateMachine->getWaitState());
    return nextState;

}

//
// ---> handleEvent(IcmpHwPeerEvent &event);
//
// handle IcmpHwPeerEvent from LinkProber
//
LinkProberState* ActiveState::handleEvent(IcmpHwPeerEvent &event)
{
    // applicable for active-standby state machine
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    mUnknownEventCount = 0;
    nextState = dynamic_cast<LinkProberState *> (stateMachine->getStandbyState());

    return nextState;
}

//
// ---> handleEvent(IcmpHwSelfEvent &event);
//
// handle IcmpHwSelfEvent from LinkProber
//
LinkProberState* ActiveState::handleEvent(IcmpHwSelfEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState = dynamic_cast<LinkProberState *> (stateMachine->getActiveState());

    resetState();

    return nextState;
}

//
// ---> handleEvent(IcmpHwUnknownEvent &event);
//
// handle IcmpHwUnknownEvent from LinkProber
//
LinkProberState* ActiveState::handleEvent(IcmpHwUnknownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    // detection timer in hardware prober takes into account retry count
    // and will move to Unknown state directly
    mPeerEventCount = 0;
    nextState = dynamic_cast<LinkProberState *> (stateMachine->getUnknownState());
    return nextState;
}

LinkProberState* ActiveState::handleEvent(IcmpHwWaitEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    nextState = dynamic_cast<LinkProberState *> (stateMachine->getWaitState());
    return nextState;

}

//
// ---> resetState();
//
// reset current state attributes
//
void ActiveState::resetState()
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    mPeerEventCount = 0;
    mUnknownEventCount = 0;
}

} /* namespace link_prober */
