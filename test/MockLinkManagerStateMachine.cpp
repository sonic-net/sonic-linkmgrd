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

#include "MockLinkManagerStateMachine.h"

namespace test
{
MockLinkManagerStateMachine::MockLinkManagerStateMachine(
    mux::MuxPort *muxPortPtr,
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig
)
    : LinkManagerStateMachineBase(
          muxPortPtr,
          strand,
          muxPortConfig,
          {link_prober::LinkProberState::Label::Unknown,
           mux_state::MuxState::Label::Wait,
           link_state::LinkState::Label::Down}
      )
{
}

void MockLinkManagerStateMachine::handleStateChange(
    link_manager::LinkProberEvent &event, link_prober::LinkProberState::Label state
)
{
    MUXLOGDEBUG(boost::format("%s: link prober state %d") % getMuxPortConfig().getPortName() % state);
    handleLinkProberStateChange(state);
}

void MockLinkManagerStateMachine::handleStateChange(
    link_manager::MuxStateEvent &event, mux_state::MuxState::Label state
)
{
    MUXLOGDEBUG(boost::format("%s: mux state %d") % getMuxPortConfig().getPortName() % state);
    handleMuxStateChange(state);
}

void MockLinkManagerStateMachine::handleStateChange(
    link_manager::LinkStateEvent &event, link_state::LinkState::Label state
)
{
    MUXLOGDEBUG(boost::format("%s: link state %d") % getMuxPortConfig().getPortName() % state);
    handleLinkStateChange(state);
}

void MockLinkManagerStateMachine::handlePeerStateChange(
    link_manager::LinkProberEvent &event, link_prober::LinkProberState::Label state
)
{
    MUXLOGDEBUG(boost::format("%s: peer link state %d") % getMuxPortConfig().getPortName() % state);
    handlePeerLinkProberStateChange(state);
}

} // namespace test
