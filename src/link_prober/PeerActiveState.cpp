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
#include "link_prober/PeerUnknownState.h"
#include "link_prober/LinkProberStateMachineBase.h"

#include "common/MuxLogger.h"

namespace link_prober
{

//
// ---> PeerActiveState(LinkProberStateMachineBase &stateMachine, common::MuxPortConfig &muxPortConfig);
//
// class constructor
//
PeerActiveState::PeerActiveState(
    LinkProberStateMachineBase &stateMachine,
    common::MuxPortConfig &muxPortConfig
) :
    LinkProberState(stateMachine, muxPortConfig)
{
}

//
// ---> handleEvent(IcmpPeerActiveEvent &event);
//
// handle IcmpPeerActiveEvent from LinkProber
//
LinkProberState* PeerActiveState::handleEvent(IcmpPeerActiveEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState = dynamic_cast<LinkProberState *> (stateMachine->getPeerActiveState());

    resetState();

    return nextState;
}

//
// ---> handleEvent(IcmpPeerUnknownEvent &event);
//
// handle IcmpPeerUnknownEvent from LinkProber
//
LinkProberState* PeerActiveState::handleEvent(IcmpPeerUnknownEvent &event)
{
    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    if (++mUnknownEventCount >= getMuxPortConfig().getNegativeStateChangeRetryCount()) {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getPeerUnknownState());
    }
    else {
        nextState = dynamic_cast<LinkProberState *> (stateMachine->getPeerActiveState());
    }

    return nextState;
}

//
// ---> handleEvent(IcmpPeerWaitEvent &event);
//
// handle IcmpPeerWaitEvent from LinkProber
//
LinkProberState* PeerActiveState::handleEvent(IcmpPeerWaitEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    nextState = dynamic_cast<LinkProberState *> (stateMachine->getPeerWaitState());
    return nextState;

}

//
// ---> handleEvent(IcmpHwPeerActiveEvent &event);
//
// handle IcmpHwPeerActiveEvent from LinkProber
//
LinkProberState* PeerActiveState::handleEvent(IcmpHwPeerActiveEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState = dynamic_cast<LinkProberState *> (stateMachine->getPeerActiveState());

    resetState();

    return nextState;
}

//
// ---> handleEvent(IcmpHwPeerUnknownEvent &event);
//
// handle IcmpHwPeerUnknownEvent from LinkProber
//
LinkProberState* PeerActiveState::handleEvent(IcmpHwPeerUnknownEvent &event)
{
    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    // detection timer in hardware prober takes into account retry count
    // and will move to Unknown state directly
    nextState = dynamic_cast<LinkProberState *> (stateMachine->getPeerUnknownState());
    return nextState;
}

//
// ---> handleEvent(IcmpHwPeerWaitEvent &event);
//
// handle IcmpHwPeerWaitEvent from LinkProber
//
LinkProberState* PeerActiveState::handleEvent(IcmpHwPeerWaitEvent &event)
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    LinkProberStateMachineBase *stateMachine = dynamic_cast<LinkProberStateMachineBase *> (getStateMachine());
    LinkProberState *nextState;

    nextState = dynamic_cast<LinkProberState *> (stateMachine->getPeerWaitState());
    return nextState;

}

//
// ---> resetState();
//
// reset current state attributes
//
void PeerActiveState::resetState()
{
    MUXLOGDEBUG(getMuxPortConfig().getPortName());

    mUnknownEventCount = 0;
}

} /* namespace link_prober */
