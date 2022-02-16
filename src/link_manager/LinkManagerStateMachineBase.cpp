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

#include <boost/bind/bind.hpp>

#include "MuxPort.h"
#include "common/MuxException.h"
#include "common/MuxLogger.h"
#include "link_manager/LinkManagerStateMachineBase.h"

namespace link_manager {
LinkProberEvent LinkManagerStateMachineBase::mLinkProberEvent;
MuxStateEvent LinkManagerStateMachineBase::mMuxStateEvent;
LinkStateEvent LinkManagerStateMachineBase::mLinkStateEvent;

std::vector<std::string> LinkManagerStateMachineBase::mLinkProberStateName = {"Active", "Standby", "Unknown", "Wait"};
std::vector<std::string> LinkManagerStateMachineBase::mMuxStateName = {"Active", "Standby", "Unknown", "Error", "Wait"};
std::vector<std::string> LinkManagerStateMachineBase::mLinkStateName = {"Up", "Down"};
std::vector<std::string> LinkManagerStateMachineBase::mLinkHealthName = {"Uninitialized", "Unhealthy", "Healthy"};

//
// ---> LinkManagerStateMachineBase::LinkManagerStateMachineBase(
//             boost::asio::io_service::strand &strand,
//             common::MuxPortConfig &muxPortConfig,
//             CompositeState initialCompositeState);
//
// class constructor
//
LinkManagerStateMachineBase::LinkManagerStateMachineBase(
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig,
    CompositeState initialCompositeState)
    : StateMachine(strand, muxPortConfig),
      mCompositeState(initialCompositeState) {
}

//
// ---> initializeTransitionFunctionTable()
//
// initialize transition function table to NOOP functions
//
void LinkManagerStateMachineBase::initializeTransitionFunctionTable() {
    MUXLOGWARNING("Initialize State Transition Table With NO-OP...");
    for (uint8_t linkProberState = link_prober::LinkProberState::Label::Active;
         linkProberState < link_prober::LinkProberState::Label::Count;
         linkProberState++) {
        for (uint8_t muxState = mux_state::MuxState::Label::Active;
             muxState < mux_state::MuxState::Label::Count; muxState++) {
            for (uint8_t linkState = link_state::LinkState::Label::Up;
                 linkState < link_state::LinkState::Label::Count; linkState++) {
                mStateTransitionHandler[linkProberState][muxState][linkState] = boost::bind(&LinkManagerStateMachineBase::noopTransitionFunction, this, boost::placeholders::_1);
            }
        }
    }
}

//
// ---> noopTransitionFunction(CompositeState &nextState)
//
// NO-OP transition function
//
void LinkManagerStateMachineBase::noopTransitionFunction(CompositeState &nextState) {
    MUXLOGINFO(mMuxPortConfig.getPortName());
}

} /* namespace link_manager */
