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
 * StateMachine.h
 *
 *  Created on: Oct 4, 2020
 *      Author: Tamer Ahmed
 */

#ifndef STATEMACHINE_H_
#define STATEMACHINE_H_

#include <memory>

#include <common/BoostAsioBehavior.h>
#include <boost/asio.hpp>

#include "common/MuxPortConfig.h"

namespace link_manager {
class LinkManagerStateMachineBase;
class ActiveStandbyStateMachine;
class ActiveActiveStateMachine;
}

namespace link_prober {
class LinkProberStateMachineBase;
class LinkProberStateMachineActiveStandby;
class LinkProberStateMachineActiveActive;
class LinkProberSessionStateMachine;
}

namespace mux_state {
class MuxStateMachine;
}

namespace link_state {
class LinkStateMachine;
}

namespace test {
class MockLinkManagerStateMachine;
}

namespace common
{
class State;

/**
 *@class StateMachine
 *
 *@brief Maintains common state machine functionality; current state,
 *       serialization object (strand,) and MuxPortConfig object
 */
class StateMachine
{
public:
    /**
    *@method StateMachine
    *
    *@brief class default constructor
    */
    StateMachine() = delete;

    /**
    *@method StateMachine
    *
    *@brief class copy constructor
    *
    *@param StateMachine (in)  reference to StateMachine object to be copied
    */
    StateMachine(const StateMachine &) = delete;

    /**
    *@method StateMachine
    *
    *@brief class constructor
    *
    *@param strand (in)         boost serialization object
    *@param muxPortConfig (in)  reference to MuxPortConfig object
    */
    StateMachine(
        boost::asio::io_service::strand &strand,
        MuxPortConfig &muxPortConfig
    );

    /**
    *@method ~StateMachine
    *
    *@brief class destructor
    */
    virtual ~StateMachine() = default;

    /**
    *@method getStrand
    *
    *@brief getter for boost serialization object
    *
    *@return reference to boost serialization object
    */
    boost::asio::io_service::strand& getStrand() {return mStrand;};

    /**
    *@method getMuxPortConfig
    *
    *@brief getter MuxPortConfig object
    *
    *@return reference to MuxPortConfig object
    */
    const MuxPortConfig& getMuxPortConfig() const {return mMuxPortConfig;};

private:
    friend class link_manager::LinkManagerStateMachineBase;
    friend class link_manager::ActiveStandbyStateMachine;
    friend class link_manager::ActiveActiveStateMachine;
    friend class link_prober::LinkProberStateMachineActiveStandby;
    friend class link_prober::LinkProberStateMachineBase;
    friend class link_prober::LinkProberStateMachineActiveActive;
    friend class link_prober::LinkProberSessionStateMachine;
    friend class mux_state::MuxStateMachine;
    friend class link_state::LinkStateMachine;
    friend class test::MockLinkManagerStateMachine;

    /**
    *@method setCurrentState
    *
    *@brief setter for current state
    *
    *@param state (in)  current state of the state machine
    *
    *@return none
    */
    void setCurrentState(State* state);

    /**
    *@method getCurrentState
    *
    *@brief getter for current state
    *
    *@return current state of the state machine
    */
    State* getCurrentState() {return mCurrentState;};

private:
    boost::asio::io_service::strand mStrand;
    State *mCurrentState = nullptr;
    MuxPortConfig &mMuxPortConfig;
};

} /* namespace common */

#endif /* STATEMACHINE_H_ */
