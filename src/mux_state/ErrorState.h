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
 * ErrorState.h
 *
 *  Created on: Mar 1, 2021
 *      Author: Tamer Ahmed
 */

#ifndef MUX_STATE_ERRORSTATE_H_
#define MUX_STATE_ERRORSTATE_H_

#include <mux_state/MuxState.h>

namespace mux_state
{
/**
 *@class ErrorState
 *
 *@brief maintains ErrorState state of MuxState
 */
class ErrorState: public MuxState
{
public:
    /**
    *@method ErrorState
    *
    *@brief class default constructor
    */
    ErrorState() = delete;

    /**
    *@method ErrorState
    *
    *@brief class copy constructor
    *
    *@param ErrorState (in)  reference to ErrorState object to be copied
    */
    ErrorState(const ErrorState &) = delete;

    /**
    *@method ErrorState
    *
    *@brief class constructor
    *
    *@param stateMachine (in)   reference to LinkStateMachine
    *@param muxPortConfig (in)  reference to MuxPortConfig object
    */
    ErrorState(
        MuxStateMachine &stateMachine,
        common::MuxPortConfig &muxPortConfig
    );

    /**
    *@method ~ErrorState
    *
    *@brief class destructor
    */
    virtual ~ErrorState() = default;

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
    *@method ErrorEvent
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
    virtual MuxState::Label getStateLabel() override {return MuxState::Label::Error;};

private:
    uint8_t mActiveEventCount = 0;
    uint8_t mStandbyEventCount = 0;
    uint8_t mUnknownEventCount = 0;
};

} /* namespace mux_state */

#endif /* MUX_STATE_ERRORSTATE_H_ */
