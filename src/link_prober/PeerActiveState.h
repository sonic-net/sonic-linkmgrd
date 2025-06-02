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

#ifndef LINK_PROBER_PEERACTIVESTATE_H_
#define LINK_PROBER_PEERACTIVESTATE_H_

#include "LinkProberState.h"

namespace link_prober
{
class LinkProberStateMachineBase;

/**
 *@class PeerActiveState
 *
 *@brief maintains peer Active state of LinkProber
 */
class PeerActiveState : public LinkProberState {
public:
    /**
     *@method PeerActiveState
     *
     *@brief class default constructor
     */
    PeerActiveState() = delete;

    /**
     *@method PeerActiveState
     *
     *@brief class copy constructor
     *
     *@param ActiveState (in)  reference to PeerActiveState object to be copied
     */
    PeerActiveState(const PeerActiveState &) = delete;

    /**
     *@method PeerActiveState
     *
     *@brief class constructor
     *
     *@param stateMachine (in)   reference to LinkProberStateMachineBase
     *@param muxPortConfig (in)  reference to MuxPortConfig object
     */
    PeerActiveState(
        LinkProberStateMachineBase &stateMachine,
        common::MuxPortConfig &muxPortConfig
    );

    /**
     *@method ~PeerActiveState
     *
     *@brief class destructor
     */
    virtual ~PeerActiveState() = default;

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
    *@brief handle IcmpPeerWaitEvent from LinkProber
    *
    *@param event (in)  reference to IcmpPeerWaitEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpPeerWaitEvent &event) override;

    /**
     *@method handleEvent
     *
     *@brief handle IcmpHwPeerActiveEvent from LinkProber
     *
     *@param event (in)  reference to IcmpHwPeerActiveEvent
     *
     *@return pointer to next LinkProberState
     */
    virtual LinkProberState *handleEvent(IcmpHwPeerActiveEvent &event) override;

    /**
     *@method handleEvent
     *
     *@brief handle IcmpHwPeerUnknownEvent from LinkProber
     *
     *@param event (in)  reference to IcmpHwPeerUnknownEvent
     *
     *@return pointer to next LinkProberState
     */
    virtual LinkProberState *handleEvent(IcmpHwPeerUnknownEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle IcmpHwPeerWaitEvent from LinkProber
    *
    *@param event (in)  reference to IcmpHwPeerWaitEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpHwPeerWaitEvent &event) override;


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
     *@return LinkProberState Active label
     */
    virtual LinkProberState::Label getStateLabel() override { return LinkProberState::Label::PeerActive; };

private:
    uint8_t mUnknownEventCount = 0;
};

} /* namespace link_prober */

#endif /* LINK_PROBER_PEERACTIVESTATE_H_ */
