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
 * WaitState.h
 *
 *  Created on: Oct 20, 2020
 *      Author: Tamer Ahmed
 */

#ifndef MUX_STATE_WAITSTATE_H_
#define MUX_STATE_WAITSTATE_H_

#include <mux_state/MuxState.h>

namespace mux_state
{

/**
 *@class WaitState
 *
 *@brief maintains WaitState state of MuxState
 */
class WaitState: public MuxState
{
public:
    /**
     *@enum WaitStateCause
     *
     *@brief WaitStateCause extends Wait state with cause attribute
     */
    enum WaitStateCause {
        CauseUnknown,
        SwssUpdate,
        DriverUpdate
    };

public:
    /**
    *@method WaitState
    *
    *@brief class default constructor
    */
    WaitState() = delete;

    /**
    *@method WaitState
    *
    *@brief class copy constructor
    *
    *@param WaitState (in)  reference to WaitState object to be copied
    */
    WaitState(const WaitState &) = delete;

    /**
    *@method WaitState
    *
    *@brief class constructor
    *
    *@param stateMachine (in)   reference to LinkStateMachine
    *@param muxPortConfig (in)  reference to MuxPortConfig object
    */
    WaitState(
        MuxStateMachine &stateMachine,
        common::MuxPortConfig &muxPortConfig
    );

    /**
    *@method ~WaitState
    *
    *@brief class destructor
    */
    virtual ~WaitState() = default;

    /**
    *@method handleEvent
    *
    *@brief handle ActiveEvent from state db/xcvrd
    *
    *@param event (in)  reference to ActiveEvent
    *
    *@return pointer to next MuxState
    */
    virtual MuxState* handleEvent(ActiveEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle StandbyEvent from state db/xcvrd
    *
    *@param event (in)  reference to StandbyEvent
    *
    *@return pointer to next MuxState
    */
    virtual MuxState* handleEvent(StandbyEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle UnknownEvent from state db/xcvrd
    *
    *@param event (in)  reference to UnknownEvent
    *
    *@return pointer to next MuxState
    */
    virtual MuxState* handleEvent(UnknownEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle ErrorEvent from state db
    *
    *@param event (in)  reference to ErrorEvent
    *
    *@return pointer to next MuxState
    */
    virtual MuxState* handleEvent(ErrorEvent &event) override;

    /**
    *@method resetState
    *
    *@brief reset current state attributes
    *
    *@return none
    */
    virtual void resetState() override;

    /**
    *@method getStateLabel
    *
    *@brief getter for MuxState label
    *
    *@return MuxState Wait label
    */
    virtual MuxState::Label getStateLabel() override {return MuxState::Label::Wait;};

    /**
    *@method setWaitStateCause
    *
    *@brief setter Wait Cause
    *
    *@param waitStateCause (in)  cause for entering wait state
    *
    *@return none
    */
    void setWaitStateCause(WaitStateCause waitStateCause) {mWaitStateCause = waitStateCause;};

    /**
    *@method getWaitOnSwssNotification
    *
    *@brief getter Wait Cause
    *
    *@return cause for entering wait state
    */
    WaitStateCause getWaitStateCause() const {return mWaitStateCause;};

private:
    uint8_t mActiveEventCount = 0;
    uint8_t mStandbyEventCount = 0;
    uint8_t mUnknownEventCount = 0;
    uint8_t mErrorEventCount = 0;

    WaitStateCause mWaitStateCause = CauseUnknown;
};

} /* namespace mux_state */

#endif /* MUX_STATE_WAITSTATE_H_ */
