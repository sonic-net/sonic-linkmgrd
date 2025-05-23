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
 * UnknownState.h
 *
 *  Created on: Oct 7, 2020
 *      Author: Tamer Ahmed
 */

#ifndef LINK_PROBER_UNKNOWNSTATE_H_
#define LINK_PROBER_UNKNOWNSTATE_H_

#include <stdint.h>

#include "LinkProberState.h"

namespace link_prober
{
class LinkProberStateMachineBase;

/**
 *@class UnknownState
 *
 *@brief maintains Unknown state of LinkProber
 */
class UnknownState : public LinkProberState
{
public:
    /**
    *@method UnknownState
    *
    *@brief class default constructor
    */
    UnknownState() = delete;

    /**
    *@method UnknownState
    *
    *@brief class copy constructor
    *
    *@param UnknownState (in)  reference to UnknownState object to be copied
    */
    UnknownState(const UnknownState &) = delete;

    /**
    *@method UnknownState
    *
    *@brief class constructor
    *
    *@param stateMachine (in)   reference to LinkProberStateMachineBase
    *@param muxPortConfig (in)  reference to MuxPortConfig object
    */
    UnknownState(
        LinkProberStateMachineBase &stateMachine,
        common::MuxPortConfig &muxPortConfig
    );

    /**
    *@method ~UnknownState
    *
    *@brief class destructor
    */
    virtual ~UnknownState() = default;

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
    *@brief handle IcmpWaitEvent from LinkProber
    *
    *@param event (in)  reference to IcmpWaitEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpWaitEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle IcmpHwPeerEvent from LinkProber
    *
    *@param event (in)  reference to IcmpHwPeerEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpHwPeerEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle IcmpHwSelfEvent from LinkProber
    *
    *@param event (in)  reference to IcmpHwSelfEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpHwSelfEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle IcmpHwUnknownEvent from LinkProber
    *
    *@param event (in)  reference to IcmpHwUnknownEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpHwUnknownEvent &event) override;

    /**
    *@method handleEvent
    *
    *@brief handle IcmpHwWaitEvent from LinkProber
    *
    *@param event (in)  reference to IcmpHwWaitEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpHwWaitEvent &event) override;


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
    virtual LinkProberState::Label getStateLabel() override {return LinkProberState::Label::Unknown;};

private:
    uint8_t mSelfEventCount = 0;
    uint8_t mPeerEventCount = 0;
};

} /* namespace link_prober */

#endif /* LINK_PROBER_UNKNOWNSTATE_H_ */
