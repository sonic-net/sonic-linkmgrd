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
 * MuxState.cpp
 *
 *  Created on: Oct 18, 2020
 *      Author: Tamer Ahmed
 */

#include <mux_state/MuxState.h>
#include <mux_state/MuxStateMachine.h>

namespace mux_state
{

//
// ---> MuxState(MuxStateMachine &stateMachine, common::MuxPortConfig &muxPortConfig);
//
// class constructor
//
MuxState::MuxState(
    MuxStateMachine &stateMachine,
    common::MuxPortConfig &muxPortConfig
):
    common::State(
        *dynamic_cast<common::StateMachine *> (&stateMachine),
        muxPortConfig
    )
{
}

} /* namespace mux_state */
