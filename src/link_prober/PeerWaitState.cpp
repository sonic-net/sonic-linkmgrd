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

#include "link_prober/PeerActiveState.h"
#include "link_prober/PeerWaitState.h"
#include "link_prober/PeerUnknownState.h"
#include "link_prober/LinkProberStateMachineBase.h"

#include "common/MuxLogger.h"

namespace link_prober
{

//
// ---> PeerWaitState(LinkProberStateMachineBase &stateMachine, common::MuxPortConfig &muxPortConfig);
//
// class constructor
//
PeerWaitState::PeerWaitState(
    LinkProberStateMachineBase &stateMachine,
    common::MuxPortConfig &muxPortConfig
)
    : LinkProberState(stateMachine, muxPortConfig)
{
}

//
// ---> handleEvent(IcmpPeerActiveEvent &event);
//
// handle IcmpPeerActiveEvent from LinkProber
//
LinkProberState *PeerWaitState::handleEvent(IcmpPeerActiveEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *>(getStateMachine());
    LinkProberState *nextState;

    mPeerUnknownEvent = 0;
    if (++mPeerActiveEvent >= getMuxPortConfig().getPositiveStateChangeRetryCount()) {
        nextState = dynamic_cast<LinkProberState *>(stateMachine->getPeerActiveState());
    } else {
        nextState = dynamic_cast<LinkProberState *>(stateMachine->getPeerWaitState());
    }

    return nextState;
}

//
// ---> handleEvent(IcmpPeerUnknownEvent &event);
//
// handle IcmpPeerUnknownEvent from LinkProber
//
LinkProberState *PeerWaitState::handleEvent(IcmpPeerUnknownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *>(getStateMachine());
    LinkProberState *nextState;

    mPeerActiveEvent = 0;
    if (++mPeerUnknownEvent >= getMuxPortConfig().getNegativeStateChangeRetryCount()) {
        nextState = dynamic_cast<LinkProberState *>(stateMachine->getPeerUnknownState());
    } else {
        nextState = dynamic_cast<LinkProberState *>(stateMachine->getPeerWaitState());
    }

    return nextState;
}

//
// ---> handleEvent(IcmpHwPeerActiveEvent &event);
//
// handle IcmpPeerActiveEvent from LinkProber
//
LinkProberState *PeerWaitState::handleEvent(IcmpHwPeerActiveEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *>(getStateMachine());
    LinkProberState *nextState;

    // moving to Active state directly here,
    // only after positive probing timer expiry
    mPeerUnknownEvent = 0;
    nextState = dynamic_cast<LinkProberState *>(stateMachine->getPeerActiveState());

    return nextState;
}

//
// ---> handleEvent(IcmpHwPeerUnknownEvent &event);
//
// handle IcmpHwPeerUnknownEvent from LinkProber
//
LinkProberState *PeerWaitState::handleEvent(IcmpHwPeerUnknownEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *>(getStateMachine());
    LinkProberState *nextState;

    mPeerActiveEvent = 0;
    nextState = dynamic_cast<LinkProberState *>(stateMachine->getPeerUnknownState());

    return nextState;
}

//
// ---> resetState();
//
// reset current state attributes
//
void PeerWaitState::resetState()
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    mPeerActiveEvent = 0;
    mPeerUnknownEvent = 0;
}

} /* namespace link_prober */
