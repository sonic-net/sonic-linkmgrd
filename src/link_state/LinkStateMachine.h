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
 * LinkStateMachine.h
 *
 *  Created on: Oct 20, 2020
 *      Author: Tamer Ahmed
 */

#ifndef LINK_STATE_LINKSTATEMACHINE_H_
#define LINK_STATE_LINKSTATEMACHINE_H_

#include "common/StateMachine.h"
#include "DownState.h"
#include "UpState.h"

namespace link_manager {
class LinkManagerStateMachine;
} /* namespace link_manager */

namespace link_state
{
/**
 *@class UpEvent
 *
 *@brief signals a UpEvent event to LinkState state machine
 */
class UpEvent {
public:
    UpEvent() = default;
    ~UpEvent() = default;
};

/**
 *@class DownEvent
 *
 *@brief signals a DownEvent event to LinkState state machine
 */
class DownEvent {
public:
    DownEvent() = default;
    ~DownEvent() = default;
};

/**
 *@class LinkStateMachine
 *
 *@brief maintains LineState state machine
 */
class LinkStateMachine: public common::StateMachine
{
public:
    /**
    *@method LinkStateMachine
    *
    *@brief class default constructor
    */
    LinkStateMachine() = delete;

    /**
    *@method LinkStateMachine
    *
    *@brief class copy constructor
    *
    *@param LinkStateMachine (in)  reference to LinkStateMachine object to be copied
    */
    LinkStateMachine(const LinkStateMachine &) = delete;

    /**
    *@method LinkStateMachine
    *
    *@brief class constructor
    *
    *@param linkManagerStateMachine (in)    reference to LinkManagerStateMachine
    *@param strand (in)                     reference to boost serialization object
    *@param muxPortConfig (in)              reference to MuxPortConfig object
    *@param label (in)                      state machine initial state
    */
    LinkStateMachine(
        link_manager::LinkManagerStateMachine &linkManagerStateMachine,
        boost::asio::io_service::strand &strand,
        common::MuxPortConfig &muxPortConfig,
        LinkState::Label label
    );

    /**
    *@method ~LinkStateMachine
    *
    *@brief class destructor
    */
    virtual ~LinkStateMachine() = default;

    /**
    *@method enterState
    *
    *@brief force the state machine to enter a given state
    *
    *@param label (in)  label of target state
    *
    *@return none
    */
    void enterState(LinkState::Label label);

    /**
    *@method postLinkStateEvent
    *
    *@brief post LinkState event to the state machine
    *
    *@param e (in)  reference to the LinkState event
    *
    *@return none
    */
    template <class E>
    void postLinkStateEvent(E &e);

    /**
    *@method processEvent
    *
    *@brief process LinkState event
    *
    *@param t (in)  reference to the LinkState event
    *
    *@return none
    */
    template <typename T>
    void processEvent(T &t);

    /**
    *@method getUpState
    *
    *@brief getter for UpState object
    *
    *@return pointer to UpState object
    */
    UpState* getUpState() {return &mUpState;};

    /**
    *@method getDownState
    *
    *@brief getter for DownState object
    *
    *@return pointer to DownState object
    */
    DownState* getDownState() {return &mDownState;};

    /**
    *@method getUpEvent
    *
    *@brief getter for UpEvent object
    *
    *@return pointer to UpEvent object
    */
    static UpEvent& getUpEvent() {return mUpEvent;};

    /**
    *@method getDownEvent
    *
    *@brief getter for DownEvent object
    *
    *@return pointer to DownEvent object
    */
    static DownEvent& getDownEvent() {return mDownEvent;};

private:
    /**
    *@method postLinkManagerEvent
    *
    *@brief post LinkState change event to LinkManager state machine
    *
    *@param LinkState (in)    pointer to current LinkState
    *
    *@return none
    */
    inline void postLinkManagerEvent(LinkState* linkState);

private:
    static UpEvent mUpEvent;
    static DownEvent mDownEvent;

private:
    link_manager::LinkManagerStateMachine &mLinkManagerStateMachine;
    UpState mUpState;
    DownState mDownState;
};

} /* namespace link_state */

#endif /* LINK_STATE_LINKSTATEMACHINE_H_ */
