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
 * LinkProberState.cpp
 *
 *  Created on: Oct 8, 2020
 *      Author: Tamer Ahmed
 */

#include "common/MuxLogger.h"

#include <link_prober/LinkProberState.h>
#include <link_prober/LinkProberStateMachineBase.h>

namespace link_prober
{

std::vector<std::string> LinkProberState::mLinkProberStateName = {
    "Active",
    "Standby",
    "Unknown",
    "Wait",
    "PeerWait",
    "PeerActive",
    "PeerUnknown",
    "Init",
    "Up",
    "Down",
    "Init",
    "Up",
    "Down"
};

//
// ---> LinkProberState(LinkProberStateMachineBase &stateMachine, common::MuxPortConfig &muxPortConfig);
//
// class constructor
//
LinkProberState::LinkProberState(
    LinkProberStateMachineBase &stateMachine,
    common::MuxPortConfig &muxPortConfig
) :
    common::State(
        *dynamic_cast<common::StateMachine *> (&stateMachine),
        muxPortConfig
    )
{
}

LinkProberState* LinkProberState::handleEvent(IcmpPeerEvent &event)
{
    MUXLOGERROR(getMuxPortConfig().getPortName());
    return nullptr;
}

LinkProberState* LinkProberState::handleEvent(IcmpSelfEvent &event)
{
    MUXLOGERROR(getMuxPortConfig().getPortName());
    return nullptr;
}

LinkProberState* LinkProberState::handleEvent(IcmpUnknownEvent &event)
{
    MUXLOGERROR(getMuxPortConfig().getPortName());
    return nullptr;
}

LinkProberState* LinkProberState::handleEvent(IcmpPeerActiveEvent &event)
{
    MUXLOGERROR(getMuxPortConfig().getPortName());
    return nullptr;
}

LinkProberState* LinkProberState::handleEvent(IcmpPeerUnknownEvent &event)
{
    MUXLOGERROR(getMuxPortConfig().getPortName());
    return nullptr;
}

LinkProberState * LinkProberState::handleEvent(LinkProberSelfUpEvent &event)
{
    MUXLOGERROR(getMuxPortConfig().getPortName());
    return nullptr;
}

LinkProberState * LinkProberState::handleEvent(LinkProberSelfDownEvent &event)
{
    MUXLOGERROR(getMuxPortConfig().getPortName());
    return nullptr;
}

LinkProberState * LinkProberState::handleEvent(LinkProberPeerUpEvent &event)
{
    MUXLOGERROR(getMuxPortConfig().getPortName());
    return nullptr;
}

LinkProberState * LinkProberState::handleEvent(LinkProberPeerDownEvent &event)
{
    MUXLOGERROR(getMuxPortConfig().getPortName());
    return nullptr;
}

} /* namespace link_prober */
