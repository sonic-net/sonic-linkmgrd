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

#ifndef LINK_PROBER_PEERUPSTATE_H_
#define LINK_PROBER_PEERUPSTATE_H_

#include "LinkProberState.h"

namespace link_prober
{
class LinkProberStateMachineBase;

/**
 *@class PeerUpState
 *
 *@brief maintains Init state of LinkProber peer session
 */
class PeerUpState: public LinkProberState
{
public:
    /**
    *@method PeerUpState
    *
    *@brief class default constructor
    */
    PeerUpState() = delete;

    /**
    *@method PeerUpState
    *
    *@brief class copy constructor
    *
    *@param PeerUpState (in)  reference to PeerUpState object to be copied
    */
    PeerUpState(const PeerUpState &) = delete;

    /**
    *@method PeerUpState
    *
    *@brief class constructor
    *
    *@param stateMachine (in)   reference to LinkProberStateMachineBase
    *@param muxPortConfig (in)  reference to MuxPortConfig object
    */
    PeerUpState(
        LinkProberStateMachineBase &stateMachine,
        common::MuxPortConfig &muxPortConfig
    );

    /**
    *@method ~PeerUpState
    *
    *@brief class destructor
    */
    virtual ~PeerUpState() = default;

    /**
    *@method handleEvent
    *
    *@brief handle IcmpPeerActiveEvent from LinkProber
    *
    *@param event (in)  reference to IcmpPeerActiveEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpPeerActiveEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle IcmpPeerUnknownEvent from LinkProber
    *
    *@param event (in)  reference to IcmpPeerUnknownEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpPeerUnknownEvent &event) override;

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
    virtual LinkProberState::Label getStateLabel() override {return LinkProberState::Label::PeerUp;};

private:
    uint8_t mPeerUnknownEvent = 0;

};

} /* namespace link_prober */

#endif /* LINK_PROBER_PEERUPSTATE_H_ */
