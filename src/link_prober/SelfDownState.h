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

#ifndef LINK_PROBER_SELFDOWNSTATE_H_
#define LINK_PROBER_SELFDOWNSTATE_H_

#include "LinkProberState.h"

namespace link_prober
{
class LinkProberStateMachineBase;

/**
 *@class SelfDownState
 *
 *@brief maintains Down state of LinkProber self session
 */
class SelfDownState: public LinkProberState
{
public:
    /**
    *@method SelfDownState
    *
    *@brief class default constructor
    */
    SelfDownState() = delete;

    /**
    *@method SelfDownState
    *
    *@brief class copy constructor
    *
    *@param SelfDownState (in)  reference to SelfDownState object to be copied
    */
    SelfDownState(const SelfDownState &) = delete;

    /**
    *@method SelfDownState
    *
    *@brief class constructor
    *
    *@param stateMachine (in)   reference to LinkProberStateMachineBase
    *@param muxPortConfig (in)  reference to MuxPortConfig object
    */
    SelfDownState(
        LinkProberStateMachineBase &stateMachine,
        common::MuxPortConfig &muxPortConfig
    );

    /**
    *@method ~SelfDownState
    *
    *@brief class destructor
    */
    virtual ~SelfDownState() = default;

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
    virtual LinkProberState::Label getStateLabel() override {return LinkProberState::Label::SelfDown;};

private:
    uint8_t mSelfEventCount = 0;
};

} /* namespace link_prober */

#endif /* LINK_PROBER_SELFDOWNSTATE_H_ */
