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

#include "link_prober/SelfInitState.h"
#include "link_prober/SelfUpState.h"
#include "link_prober/SelfDownState.h"
#include "link_prober/LinkProberStateMachineBase.h"

#include "common/MuxLogger.h"

namespace link_prober
{

//
// ---> SelfInitState(LinkProberStateMachineBase &stateMachine, common::MuxPortConfig &muxPortConfig);
//
// class constructor
//
SelfInitState::SelfInitState(
    LinkProberStateMachineBase &stateMachine,
    common::MuxPortConfig &muxPortConfig
) :
    LinkProberState(stateMachine, muxPortConfig)
{
}

//
// ---> handleEvent(IcmpSelfEvent &event);
//
// handle IcmpSelfEvent from LinkProber
//
LinkProberState* SelfInitState::handleEvent(IcmpSelfEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    mUnknownEventCount = 0;
    if (++mSelfEventCount >= getMuxPortConfig().getPositiveStateChangeRetryCount()) {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getSelfUpState());
    }
    else {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getSelfInitState());
    }

    return nextState;
}

//
// ---> handleEvent(IcmpUnknownEvent &event);
//
// handle IcmpUnknownEvent from LinkProber
//
LinkProberState* SelfInitState::handleEvent(IcmpUnknownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    mSelfEventCount = 0;
    if (++mUnknownEventCount >= getMuxPortConfig().getNegativeStateChangeRetryCount()) {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getSelfDownState());
    }
    else {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getSelfInitState());
    }

    return nextState;
}

//
// ---> resetState();
//
// reset current state attributes
//
void SelfInitState::resetState()
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mSelfEventCount = 0;
    mUnknownEventCount = 0;
}

} /* namespace link_prober */
