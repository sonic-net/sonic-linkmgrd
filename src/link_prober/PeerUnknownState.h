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

#ifndef LINK_PROBER_PEERUNKNOWNSTATE_H_
#define LINK_PROBER_PEERUNKNOWNSTATE_H_

#include <stdint.h>

#include "LinkProberState.h"

namespace link_prober
{
class LinkProberStateMachineBase;

/**
 *@class PeerUnknownState
 *
 *@brief maintains peer Unknown state of LinkProber
 */
class PeerUnknownState : public LinkProberState {
public:
    /**
     *@method PeerUnknownState
     *
     *@brief class default constructor
     */
    PeerUnknownState() = delete;

    /**
     *@method PeerUnknownState
     *
     *@brief class copy constructor
     *
     *@param PeerUnknownState (in)  reference to PeerUnknownState object to be copied
     */
    PeerUnknownState(const PeerUnknownState &) = delete;

    /**
     *@method PeerUnknownState
     *
     *@brief class constructor
     *
     *@param stateMachine (in)   reference to LinkProberStateMachineBase
     *@param muxPortConfig (in)  reference to MuxPortConfig object
     */
    PeerUnknownState(
        LinkProberStateMachineBase &stateMachine,
        common::MuxPortConfig &muxPortConfig
    );

    /**
     *@method ~PeerUnknownState
     *
     *@brief class destructor
     */
    virtual ~PeerUnknownState() = default;

    /**
     *@method handleEvent
     *
     *@brief handle IcmpPeerActiveEvent from LinkProber
     *
     *@param event (in)  reference to IcmpPeerActiveEvent
     *
     *@return pointer to next LinkProberState
     */
    virtual LinkProberState *handleEvent(IcmpPeerActiveEvent &event) override;

    /**
     *@method handleEvent
     *
     *@brief handle IcmpPeerUnknownEvent from LinkProber
     *
     *@param event (in)  reference to IcmpPeerUnknownEvent
     *
     *@return pointer to next LinkProberState
     */
    virtual LinkProberState *handleEvent(IcmpPeerUnknownEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle LinkProberPeerUpEvent from LinkProber
    *
    *@param event (in)  reference to LinkProberPeerUpEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(LinkProberPeerUpEvent &event);

    /**
    *@method handleEvent
    *
    *@brief handle LinkProberPeerDownEvent from LinkProber
    *
    *@param event (in)  reference to LinkProberPeerDownEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(LinkProberPeerDownEvent &event);

    /**
     *@method resetState
     *
     *@brief reset current state attributes
     *
     *@return none
     */
    virtual void resetState() override;

    /**
     *@method getStateLabel
     *
     *@brief getter for LinkeProberState label
     *
     *@return LinkProberState Unknown label
     */
    virtual LinkProberState::Label getStateLabel() override { return LinkProberState::Label::PeerUnknown; };

private:
    uint8_t mPeerEventCount = 0;
};

} /* namespace link_prober */

#endif /* LINK_PROBER_PEERUNKNOWNSTATE_H_ */
