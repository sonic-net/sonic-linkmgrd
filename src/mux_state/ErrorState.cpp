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
 * ErrorState.cpp
 *
 *  Created on: Mar 1, 2021
 *      Author: Tamer Ahmed
 */

#include "mux_state/ActiveState.h"
#include "mux_state/ErrorState.h"
#include "mux_state/StandbyState.h"
#include "mux_state/UnknownState.h"
#include "mux_state/MuxStateMachine.h"

#include "common/MuxLogger.h"

namespace mux_state
{

//
// ---> ErrorState(MuxStateMachine &stateMachine, common::MuxPortConfig &muxPortConfig);
//
// class constructor
//
ErrorState::ErrorState(
    MuxStateMachine &stateMachine,
    common::MuxPortConfig &muxPortConfig
) :
    MuxState(stateMachine, muxPortConfig)
{
}

//
// ---> handleEvent(ActiveEvent &event);
//
// handle ActiveEvent from state db/xcvrd
//
MuxState* ErrorState::handleEvent(ActiveEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    MuxStateMachine *stateMachine =
        dynamic_cast<MuxStateMachine *> (getStateMachine());
    MuxState *nextState;

    mStandbyEventCount = 0;
    mUnknownEventCount = 0;
    if (++mActiveEventCount >= getMuxPortConfig().getMuxStateChangeRetryCount()) {
        nextState = dynamic_cast<MuxState *> (stateMachine->getActiveState());
    }
    else {
        nextState = dynamic_cast<MuxState *> (stateMachine->getErrorState());
    }

    return nextState;
}

//
// ---> handleEvent(StandbyEvent &event);
//
// handle StandbyEvent from state db/xcvrd
//
MuxState* ErrorState::handleEvent(StandbyEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    MuxStateMachine *stateMachine =
        dynamic_cast<MuxStateMachine *> (getStateMachine());
    MuxState *nextState;

    mActiveEventCount = 0;
    mUnknownEventCount = 0;
    if (++mStandbyEventCount >= getMuxPortConfig().getMuxStateChangeRetryCount()) {
        nextState = dynamic_cast<MuxState *> (stateMachine->getStandbyState());
    }
    else {
        nextState = dynamic_cast<MuxState *> (stateMachine->getErrorState());
    }

    return nextState;
}

//
// ---> handleEvent(UnknownEvent &event);
//
// handle UnknownEvent from state db/xcvrd
//
MuxState* ErrorState::handleEvent(UnknownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    MuxStateMachine *stateMachine =
        dynamic_cast<MuxStateMachine *> (getStateMachine());
    MuxState *nextState;

    mActiveEventCount = 0;
    mStandbyEventCount = 0;
    if (++mUnknownEventCount >= getMuxPortConfig().getMuxStateChangeRetryCount()) {
        nextState = dynamic_cast<MuxState *> (stateMachine->getUnknownState());
    }
    else {
        nextState = dynamic_cast<MuxState *> (stateMachine->getErrorState());
    }

    return nextState;
}

//
// ---> handleEvent(ErrorEvent &event);
//
// handle ErrorEvent from state db
//
MuxState* ErrorState::handleEvent(ErrorEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    MuxStateMachine *stateMachine = dynamic_cast<MuxStateMachine *> (getStateMachine());
    MuxState *nextState =
        dynamic_cast<MuxState *> (stateMachine->getErrorState());

    resetState();

    return nextState;
}

//
// ---> resetState();
//
// reset current state attributes
//
void ErrorState::resetState()
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    mActiveEventCount = 0;
    mStandbyEventCount = 0;
    mUnknownEventCount = 0;
}

} /* namespace mux_state */
