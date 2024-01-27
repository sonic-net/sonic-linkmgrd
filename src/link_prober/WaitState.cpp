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
 * WaitState.cpp
 *
 *  Created on: Oct 18, 2020
 *      Author: Tamer Ahmed
 */

#include "link_prober/ActiveState.h"
#include "link_prober/StandbyState.h"
#include "link_prober/WaitState.h"
#include "link_prober/LinkProberStateMachineBase.h"

#include "common/MuxLogger.h"

namespace link_prober
{

//
// ---> WaitState(LinkProberStateMachineBase &stateMachine, common::MuxPortConfig &muxPortConfig);
//
// class constructor
//
WaitState::WaitState(
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
LinkProberState* WaitState::handleEvent(IcmpPeerEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    mSelfEventCount = 0;
    mUnknownEventCount = 0;
    if (++mPeerEventCount >= getMuxPortConfig().getPositiveStateChangeRetryCount()) {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getStandbyState());
    }
    else {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getWaitState());
    }

    return nextState;
}

//
// ---> handleEvent(IcmpSelfEvent &event);
//
// handle IcmpSelfEvent from LinkProber
//
LinkProberState* WaitState::handleEvent(IcmpSelfEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    mPeerEventCount = 0;
    mUnknownEventCount = 0;
    if (++mSelfEventCount >= getMuxPortConfig().getPositiveStateChangeRetryCount()) {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getActiveState());
    }
    else {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getWaitState());
    }

    return nextState;
}

//
// ---> handleEvent(IcmpUnknownEvent &event);
//
// handle IcmpUnknownEvent from LinkProber
//
LinkProberState* WaitState::handleEvent(IcmpUnknownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState = dynamic_cast<LinkProberState *> (stateMachine->getWaitState());
    common::MuxPortConfig::PortCableType portCableType = stateMachine->getMuxPortConfig().getPortCableType();

    switch (portCableType) {
        case common::MuxPortConfig::PortCableType::ActiveActive:
            if (++mUnknownEventCount >= getMuxPortConfig().getNegativeStateChangeRetryCount()) {
                nextState = dynamic_cast<LinkProberState *> (stateMachine->getUnknownState());
            }
            break;
        case common::MuxPortConfig::PortCableType::ActiveStandby:
            resetState();
            break;
        default:
            break;
    }

    return nextState;
}

//
// ---> handleEvent(LinkProberSelfUpEvent &event);
//
// handle LinkProberSelfUpEvent from LinkProber
//
LinkProberState* WaitState::handleEvent(LinkProberSelfUpEvent &event)
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
LinkProberState* WaitState::handleEvent(LinkProberSelfDownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState = dynamic_cast<LinkProberState *> (stateMachine->getWaitState());
    common::MuxPortConfig::PortCableType portCableType = stateMachine->getMuxPortConfig().getPortCableType();

    switch (portCableType) {
        case common::MuxPortConfig::PortCableType::ActiveActive:
            nextState = dynamic_cast<LinkProberState *> (stateMachine->getUnknownState());
            break;
        case common::MuxPortConfig::PortCableType::ActiveStandby:
            resetState();
            break;
        default:
            break;
    }

    return nextState;
}

//
// ---> handleEvent(LinkProberPeerUpEvent &event);
//
// handle LinkProberPeerUpEvent from LinkProber
//
LinkProberState* WaitState::handleEvent(LinkProberPeerUpEvent &event)
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
LinkProberState* WaitState::handleEvent(LinkProberPeerDownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState = dynamic_cast<LinkProberState *> (stateMachine->getWaitState());

    resetState();

    return nextState;
}

//
// ---> resetState();
//
// reset current state attributes
//
void WaitState::resetState()
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    mSelfEventCount = 0;
    mUnknownEventCount = 0;
    mPeerEventCount = 0;
}

} /* namespace link_prober */
