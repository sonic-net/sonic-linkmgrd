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
 * UpState.cpp
 *
 *  Created on: Oct 20, 2020
 *      Author: Tamer Ahmed
 */

#include "common/MuxLogger.h"
#include "link_state/DownState.h"
#include "link_state/UpState.h"
#include "link_state/LinkStateMachine.h"

namespace link_state
{

//
// ---> UpState(LinkStateMachine &stateMachine, common::MuxPortConfig &muxPortConfig);
//
// class constructor
//
UpState::UpState(
    LinkStateMachine &stateMachine,
    common::MuxPortConfig &muxPortConfig
) :
    LinkState(stateMachine, muxPortConfig)
{
}

//
// --->handleEvent(UpEvent &event);
//
// handle UpEvent from state db
//
LinkState* UpState::handleEvent(UpEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkStateMachine *stateMachine = dynamic_cast<LinkStateMachine *> (getStateMachine());
    LinkState *nextState = dynamic_cast<LinkState *> (stateMachine->getUpState());

    resetState();

    return nextState;
}

//
// --->handleEvent(DownEvent &event);
//
// handle UpEvent from state db
//
LinkState* UpState::handleEvent(DownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkStateMachine *stateMachine = dynamic_cast<LinkStateMachine *> (getStateMachine());
    LinkState *nextState;

    if (++mDownEventCount >= getMuxPortConfig().getLinkStateChangeRetryCount()) {
        nextState = dynamic_cast<LinkState *> (stateMachine->getDownState());
    }
    else {
        nextState = dynamic_cast<LinkState *> (stateMachine->getUpState());
    }

    return nextState;
}

//
// --->resetState();
//
// reset current state attributes
//
void UpState::resetState()
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());
    mDownEventCount = 0;
}

} /* namespace link_state */
