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
 * MuxState.h
 *
 *  Created on: Oct 18, 2020
 *      Author: Tamer Ahmed
 */

#ifndef MUX_STATE_MUXSTATE_H_
#define MUX_STATE_MUXSTATE_H_

#include <common/State.h>

namespace mux_state
{
class MuxStateMachine;
class ActiveEvent;
class StandbyEvent;
class UnknownEvent;
class ErrorEvent;

/**
 *@class MuxState
 *
 *@brief base class for different Mux states
 */
class MuxState: public common::State
{
public:
    /**
     *@enum Label
     *
     *@brief Label corresponding to each MuxState State
     */
    enum Label {
        Active,
        Standby,
        Unknown,
        Error,
        Wait,

        Count
    };

public:
    /**
    *@method MuxState
    *
    *@brief class default constructor
    */
    MuxState() = delete;

    /**
    *@method MuxState
    *
    *@brief class copy constructor
    *
    *@param MuxState (in)  reference to MuxState object to be copied
    */
    MuxState(const MuxState &) = delete;

    /**
    *@method MuxState
    *
    *@brief class constructor
    *
    *@param stateMachine (in)   reference to MuxStateMachine object
    *@param muxPortConfig (in)  reference to MuxPortConfig object
    */
    MuxState(
        MuxStateMachine &stateMachine,
        common::MuxPortConfig &muxPortConfig
    );

    /**
    *@method ~MuxState
    *
    *@brief class destructor
    */
    virtual ~MuxState() = default;

    /**
    *@method handleEvent
    *
    *@brief handle ActiveEvent from state db/xcvrd
    *
    *@param event (in)  reference to ActiveEvent
    *
    *@return pointer to next MuxState
    */
    virtual MuxState* handleEvent(ActiveEvent &event) = 0;

    /**
    *@method handleEvent
    *
    *@brief handle StandbyEvent from state db/xcvrd
    *
    *@param event (in)  reference to StandbyEvent
    *
    *@return pointer to next MuxState
    */
    virtual MuxState* handleEvent(StandbyEvent &event) = 0;

    /**
    *@method handleEvent
    *
    *@brief handle UnknownEvent from state db/xcvrd
    *
    *@param event (in)  reference to UnknownEvent
    *
    *@return pointer to next MuxState
    */
    virtual MuxState* handleEvent(UnknownEvent &event) = 0;

    /**
    *@method handleEvent
    *
    *@brief handle ErrorEvent from state db
    *
    *@param event (in)  reference to ErrorEvent
    *
    *@return pointer to next MuxState
    */
    virtual MuxState* handleEvent(ErrorEvent &event) = 0;

    /**
    *@method resetState
    *
    *@brief reset current state attributes
    *
    *@return none
    */
    virtual MuxState::Label getStateLabel() = 0;
};

} /* namespace mux_state */

#endif /* MUX_STATE_MUXSTATE_H_ */
