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
 * StandbyState.h
 *
 *  Created on: Oct 7, 2020
 *      Author: Tamer Ahmed
 */

#ifndef LINK_PROBER_STANDBYSTATE_H_
#define LINK_PROBER_STANDBYSTATE_H_

#include "LinkProberState.h"

namespace link_prober
{
class LinkProberStateMachineBase;

/**
 *@class StandbyState
 *
 *@brief maintains Standby state of LinkProber
 */
class StandbyState : public LinkProberState
{
public:
    /**
    *@method StandbyState
    *
    *@brief class default constructor
    */
    StandbyState() = delete;

    /**
    *@method StandbyState
    *
    *@brief class copy constructor
    *
    *@param StandbyState (in)  reference to StandbyState object to be copied
    */
    StandbyState(const StandbyState &) = delete;

    /**
    *@method StandbyState
    *
    *@brief class constructor
    *
    *@param stateMachine (in)   reference to LinkProberStateMachineBase
    *@param muxPortConfig (in)  reference to MuxPortConfig object
    */
    StandbyState(
        LinkProberStateMachineBase &stateMachine,
        common::MuxPortConfig &muxPortConfig
    );

    /**
    *@method ~StandbyState
    *
    *@brief class destructor
    */
    virtual ~StandbyState() = default;

    /**
    *@method handleEvent
    *
    *@brief handle IcmpPeerEvent from LinkProber
    *
    *@param event (in)  reference to IcmpPeerEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpPeerEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle IcmpSelfEvent from LinkProber
    *
    *@param event (in)  reference to IcmpSelfEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpSelfEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle IcmpUnknownEvent from LinkProber
    *
    *@param event (in)  reference to IcmpUnknownEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpUnknownEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle LinkProberSelfUpEvent from LinkProber
    *
    *@param event (in)  reference to LinkProberSelfUpEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(LinkProberSelfUpEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle LinkProberSelfDownEvent from LinkProber
    *
    *@param event (in)  reference to LinkProberSelfDownEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(LinkProberSelfDownEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle LinkProberPeerUpEvent from LinkProber
    *
    *@param event (in)  reference to LinkProberPeerUpEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(LinkProberPeerUpEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle LinkProberPeerDownEvent from LinkProber
    *
    *@param event (in)  reference to LinkProberPeerDownEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(LinkProberPeerDownEvent &event) override;

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
    *@return LinkProberState Standby label
    */
    virtual LinkProberState::Label getStateLabel() override {return LinkProberState::Label::Standby;};

private:
    uint8_t mSelfEventCount = 0;
    uint8_t mUnknownEventCount = 0;
};

} /* namespace link_prober */

#endif /* LINK_PROBER_STANDBYSTATE_H_ */
