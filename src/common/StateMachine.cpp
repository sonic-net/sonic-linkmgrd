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
 * StateMachine.cpp
 *
 *  Created on: Oct 4, 2020
 *      Author: Tamer Ahmed
 */

#include <common/State.h>
#include <common/StateMachine.h>
#include "MuxLogger.h"


namespace common
{

//
// ---> StateMachine(boost::asio::io_service::strand &strand, MuxPortConfig &muxPortConfig);
//
// class constructor
//
StateMachine::StateMachine(
    boost::asio::io_service::strand &strand,
    MuxPortConfig &muxPortConfig
) :
    mStrand(strand),
    mMuxPortConfig(muxPortConfig)
{
}

//
// ---> setCurrentState(common::State* state);
//
// setter for current state, reset state when changed
//
void StateMachine::setCurrentState(common::State* state)
{
    if (mCurrentState != state) {
        mCurrentState = state;
        mCurrentState->resetState();
    }
};

} /* namespace common */
