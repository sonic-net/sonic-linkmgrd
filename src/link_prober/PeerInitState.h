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

#ifndef LINK_PROBER_PEERINITSTATE_H_
#define LINK_PROBER_PEERINITSTATE_H_

#include "LinkProberState.h"

namespace link_prober
{
class LinkProberStateMachineBase;

/**
 *@class PeerInitState
 *
 *@brief maintains Init state of LinkProber peer session
 */
class PeerInitState: public LinkProberState
{
public:
    /**
    *@method PeerInitState
    *
    *@brief class default constructor
    */
    PeerInitState() = delete;

    /**
    *@method PeerInitState
    *
    *@brief class copy constructor
    *
    *@param PeerInitState (in)  reference to PeerInitState object to be copied
    */
    PeerInitState(const PeerInitState &) = delete;

    /**
    *@method PeerInitState
    *
    *@brief class constructor
    *
    *@param stateMachine (in)   reference to LinkProberStateMachineBase
    *@param muxPortConfig (in)  reference to MuxPortConfig object
    */
    PeerInitState(
        LinkProberStateMachineBase &stateMachine,
        common::MuxPortConfig &muxPortConfig
    );

    /**
    *@method ~PeerInitState
    *
    *@brief class destructor
    */
    virtual ~PeerInitState() = default;

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
    virtual LinkProberState::Label getStateLabel() override {return LinkProberState::Label::PeerInit;};

private:
    uint8_t mPeerActiveEvent = 0;
    uint8_t mPeerUnknownEvent = 0;

};

} /* namespace link_prober */

#endif /* LINK_PROBER_PEERINITSTATE_H_ */
