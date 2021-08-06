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
 * LinkState.h
 *
 *  Created on: Oct 18, 2020
 *      Author: Tamer Ahmed
 */

#ifndef LINK_STATE_LINKSTATE_H_
#define LINK_STATE_LINKSTATE_H_

#include <common/State.h>

namespace link_state
{
class LinkStateMachine;
class UpEvent;
class DownEvent;

/**
 *@class LinkState
 *
 *@brief base class for different LinkState states
 */
class LinkState: public common::State
{
public:
    /**
     *@enum Label
     *
     *@brief Label corresponding to each LinkState State
     */
    enum Label {
        Up,
        Down,

        Count
    };

public:
    /**
    *@method LinkState
    *
    *@brief class default constructor
    */
    LinkState() = delete;

    /**
    *@method LinkState
    *
    *@brief class copy constructor
    *
    *@param LinkState (in)  reference to LinkState object to be copied
    */
    LinkState(const LinkState &) = delete;

    /**
    *@method LinkState
    *
    *@brief class constructor
    *
    *@param stateMachine (in)   reference to LinkProberStateMachine object
    *@param muxPortConfig (in)  reference to MuxPortConfig object
    */
    LinkState(
        LinkStateMachine &stateMachine,
        common::MuxPortConfig &muxPortConfig
    );

    /**
    *@method ~LinkProber
    *
    *@brief class destructor
    */
    virtual ~LinkState() = default;

    /**
    *@method handleEvent
    *
    *@brief handle UpEvent from state db
    *
    *@param event (in)  reference to UpEvent
    *
    *@return pointer to next LinkState
    */
    virtual LinkState* handleEvent(UpEvent &event) = 0;

    /**
    *@method handleEvent
    *
    *@brief handle DownEvent from state db
    *
    *@param event (in)  reference to DownEvent
    *
    *@return pointer to next LinkState
    */
    virtual LinkState* handleEvent(DownEvent &event) = 0;

    /**
    *@method getStateLabel
    *
    *@brief getter for LinkState label
    *
    *@return LinkState Down label
    */
    virtual LinkState::Label getStateLabel() = 0;
};

} /* namespace link_state */

#endif /* LINK_STATE_LINKSTATE_H_ */
