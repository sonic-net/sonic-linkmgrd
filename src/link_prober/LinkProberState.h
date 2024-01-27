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
 * LinkProberState.h
 *
 *  Created on: Oct 8, 2020
 *      Author: Tamer Ahmed
 */

#ifndef LINK_PROBER_LINKPROBERSTATE_H_
#define LINK_PROBER_LINKPROBERSTATE_H_

#include <common/State.h>

namespace link_prober
{
class LinkProberStateMachineBase;
class IcmpPeerEvent;
class IcmpSelfEvent;
class IcmpUnknownEvent;
class SuspendTimerExpiredEvent;
class IcmpPeerActiveEvent;
class IcmpPeerUnknownEvent;
class LinkProberSelfUpEvent;
class LinkProberSelfDownEvent;
class LinkProberPeerUpEvent;
class LinkProberPeerDownEvent;

/**
 *@class LinkProberState
 *
 *@brief base class for different LinkProber states
 */
class LinkProberState: public common::State
{
public:
    /**
     *@enum Label
     *
     *@brief Label corresponding to each LinkProber State
     */
    enum Label {
        Active,
        Standby,
        Unknown,
        Wait,
        PeerWait,
        PeerActive,
        PeerUnknown,

        // session states
        SelfInit,
        SelfUp,
        SelfDown,
        PeerInit,
        PeerUp,
        PeerDown,
        Count
    };

    static std::vector<std::string> mLinkProberStateName;

public:
    /**
    *@method LinkProberState
    *
    *@brief class default constructor
    */
    LinkProberState() = delete;

    /**
    *@method LinkProberState
    *
    *@brief class copy constructor
    *
    *@param LinkProberState (in)  reference to LinkProberState object to be copied
    */
    LinkProberState(const LinkProberState &) = delete;

    /**
    *@method LinkProberState
    *
    *@brief class constructor
    *
    *@param stateMachine (in)   reference to LinkProberStateMachineBase object
    *@param muxPortConfig (in)  reference to MuxPortConfig object
    */
    LinkProberState(
        LinkProberStateMachineBase &stateMachine,
        common::MuxPortConfig &muxPortConfig
    );

    /**
    *@method ~LinkProber
    *
    *@brief class destructor
    */
    virtual ~LinkProberState() = default;

    /**
    *@method handleEvent
    *
    *@brief handle IcmpPeerEvent from LinkProber
    *
    *@param event (in)  reference to IcmpPeerEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpPeerEvent &event);

    /**
    *@method handleEvent
    *
    *@brief handle IcmpSelfEvent from LinkProber
    *
    *@param event (in)  reference to IcmpSelfEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpSelfEvent &event);

    /**
    *@method handleEvent
    *
    *@brief handle IcmpUnknownEvent from LinkProber
    *
    *@param event (in)  reference to IcmpUnknownEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpUnknownEvent &event);

    /**
    *@method handleEvent
    *
    *@brief handle IcmpPeerActiveEvent from LinkProber
    *
    *@param event (in)  reference to IcmpPeerActiveEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpPeerActiveEvent &event);

    /**
    *@method handleEvent
    *
    *@brief handle IcmpPeerUnknownEvent from LinkProber
    *
    *@param event (in)  reference to IcmpPeerUnknownEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(IcmpPeerUnknownEvent &event);

    /**
    *@method handleEvent
    *
    *@brief handle LinkProberSelfUpEvent from LinkProber
    *
    *@param event (in)  reference to LinkProberSelfUpEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(LinkProberSelfUpEvent &event);

    /**
    *@method handleEvent
    *
    *@brief handle LinkProberSelfDownEvent from LinkProber
    *
    *@param event (in)  reference to LinkProberSelfDownEvent
    *
    *@return pointer to next LinkProberState
    */
    virtual LinkProberState* handleEvent(LinkProberSelfDownEvent &event);

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
    *@method getStateLabel
    *
    *@brief getter for LinkeProberState label
    *
    *@return LinkProberState Active label
    */
    virtual LinkProberState::Label getStateLabel() = 0;
};

} /* namespace link_prober */

#endif /* LINK_PROBER_LINKPROBERSTATE_H_ */
