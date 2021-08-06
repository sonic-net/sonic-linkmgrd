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
 * MuxStateMachine.h
 *
 *  Created on: Oct 20, 2020
 *      Author: Tamer Ahmed
 */

#ifndef MUX_STATE_MUXSTATEMACHINE_H_
#define MUX_STATE_MUXSTATEMACHINE_H_

#include "common/StateMachine.h"
#include "mux_state/ActiveState.h"
#include "mux_state/ErrorState.h"
#include "mux_state/StandbyState.h"
#include "mux_state/WaitState.h"
#include "mux_state/UnknownState.h"

namespace link_manager {
class LinkManagerStateMachine;
} /* namespace link_manager */

namespace mux_state
{
/**
 *@class ActiveEvent
 *
 *@brief signals a ActiveEvent event to MuxState state machine
 */
class ActiveEvent {
public:
    ActiveEvent() = default;
    ~ActiveEvent() = default;
};

/**
 *@class StandbyEvent
 *
 *@brief signals a StandbyEvent event to MuxState state machine
 */
class StandbyEvent {
public:
    StandbyEvent() = default;
    ~StandbyEvent() = default;
};

/**
 *@class UnknownEvent
 *
 *@brief signals a UnknownEvent event to MuxState state machine
 */
class UnknownEvent {
public:
    UnknownEvent() = default;
    ~UnknownEvent() = default;
};

/**
 *@class ErrorEvent
 *
 *@brief signals a ErrorEvent event to MuxState state machine
 */
class ErrorEvent {
public:
    ErrorEvent() = default;
    ~ErrorEvent() = default;
};

/**
 *@class MuxStateMachine
 *
 *@brief maintains MuxState state machine
 */
class MuxStateMachine: public common::StateMachine
{
public:
    /**
    *@method MuxStateMachine
    *
    *@brief class default constructor
    */
    MuxStateMachine() = delete;

    /**
    *@method MuxStateMachine
    *
    *@brief class copy constructor
    *
    *@param MuxStateMachine (in)  reference to MuxStateMachine object to be copied
    */
    MuxStateMachine(const MuxStateMachine &) = delete;

    /**
    *@method MuxStateMachine
    *
    *@brief class constructor
    *
    *@param linkManagerStateMachine (in)    reference to LinkManagerStateMachine
    *@param strand (in)                     reference to boost serialization object
    *@param muxPortConfig (in)              reference to MuxPortConfig object
    *@param label (in)                      state machine initial state
    */
    MuxStateMachine(
        link_manager::LinkManagerStateMachine &linkManagerStateMachine,
        boost::asio::io_service::strand &strand,
        common::MuxPortConfig &muxPortConfig,
        MuxState::Label label
    );

    /**
    *@method ~MuxStateMachine
    *
    *@brief class destructor
    */
    virtual ~MuxStateMachine() = default;

    /**
    *@method enterState
    *
    *@brief force the state machine to enter a given state
    *
    *@param label (in)  label of target state
    *
    *@return none
    */
    void enterState(MuxState::Label label);

    /**
    *@method postMuxStateEvent
    *
    *@brief post MuxState event to the state machine
    *
    *@param e (in)  reference to the MuxState event
    *
    *@return none
    */
    template <class E>
    void postMuxStateEvent(E &e);

    /**
    *@method processEvent
    *
    *@brief process MuxState event
    *
    *@param t (in)  reference to the MuxState event
    *
    *@return none
    */
    template <typename T>
    void processEvent(T &t);

    /**
    *@method getActiveState
    *
    *@brief getter for ActiveState object
    *
    *@return pointer to ActiveState object
    */
    ActiveState* getActiveState() {return &mActiveState;};

    /**
    *@method getStandbyState
    *
    *@brief getter for StandbyState object
    *
    *@return pointer to StandbyState object
    */
    StandbyState* getStandbyState() {return &mStandbyState;};

    /**
    *@method getUnknownState
    *
    *@brief getter for UnknownState object
    *
    *@return pointer to UnknownState object
    */
    UnknownState* getUnknownState() {return &mUnknownState;};

    /**
    *@method getErrorState
    *
    *@brief getter for ErrorState object
    *
    *@return pointer to ErrorState object
    */
    ErrorState* getErrorState() {return &mErrorState;};

    /**
    *@method getWaitState
    *
    *@brief getter for WaitState object
    *
    *@return pointer to WaitState object
    */
    WaitState* getWaitState() {return &mWaitState;};

    /**
    *@method getActiveEvent
    *
    *@brief getter for ActiveEvent object
    *
    *@return pointer to ActiveEvent object
    */
    static ActiveEvent& getActiveEvent() {return mActiveEvent;};

    /**
    *@method getStandbyEvent
    *
    *@brief getter for StandbyEvent object
    *
    *@return pointer to StandbyEvent object
    */
    static StandbyEvent& getStandbyEvent() {return mStandbyEvent;};

    /**
    *@method getUnknownEvent
    *
    *@brief getter for UnknownEvent object
    *
    *@return pointer to UnknownEvent object
    */
    static UnknownEvent& getUnknownEvent() {return mUnknownEvent;};

    /**
    *@method getErrorEvent
    *
    *@brief getter for ErrorEvent object
    *
    *@return pointer to ErrorEvent object
    */
    static ErrorEvent& getErrorEvent() {return mErrorEvent;};

    /**
    *@method setWaitStateCause
    *
    *@brief setter Wait Cause
    *
    *@param waitStateCause (in)  cause for entering wait state
    *
    *@return none
    */
    void setWaitStateCause(WaitState::WaitStateCause waitStateCause) {mWaitState.setWaitStateCause(waitStateCause);};

    /**
    *@method getWaitOnSwssNotification
    *
    *@brief getter Wait Cause
    *
    *@return cause for entering wait state
    */
    WaitState::WaitStateCause getWaitStateCause() const {return mWaitState.getWaitStateCause();};

private:
    /**
    *@method postLinkManagerEvent
    *
    *@brief post MuxState change event to LinkManager state machine
    *
    *@param muxState (in)    pointer to current MuxState
    *
    *@return none
    */
    inline void postLinkManagerEvent(MuxState* muxState);

private:
    static ActiveEvent mActiveEvent;
    static StandbyEvent mStandbyEvent;
    static UnknownEvent mUnknownEvent;
    static ErrorEvent mErrorEvent;

private:
    link_manager::LinkManagerStateMachine &mLinkManagerStateMachine;
    ActiveState mActiveState;
    StandbyState mStandbyState;
    UnknownState mUnknownState;
    ErrorState mErrorState;
    WaitState mWaitState;
};

} /* namespace mux_state */

#endif /* MUX_STATE_MUXSTATEMACHINE_H_ */
