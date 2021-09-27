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
 * LinkProberStateMachine.h
 *
 *  Created on: Oct 7, 2020
 *      Author: tamer
 */

#ifndef LINK_PROBER_LINKPROBERSTATEMACHINE_H_
#define LINK_PROBER_LINKPROBERSTATEMACHINE_H_

#include <common/StateMachine.h>
#include "link_prober/ActiveState.h"
#include "link_prober/StandbyState.h"
#include "link_prober/UnknownState.h"
#include "link_prober/WaitState.h"

namespace link_manager {
class LinkManagerStateMachine;
} /* namespace link_manager */

namespace link_prober
{
/**
 *@class IcmpSelfEvent
 *
 *@brief signals a IcmpSelfEvent event to LinkProber state machine
 */
class IcmpSelfEvent {
public:
    IcmpSelfEvent() = default;
    ~IcmpSelfEvent() = default;
};

/**
 *@class IcmpPeerEvent
 *
 *@brief signals a IcmpPeerEvent event to LinkProber state machine
 */
class IcmpPeerEvent {
public:
    IcmpPeerEvent() = default;
    ~IcmpPeerEvent() = default;
};

/**
 *@class IcmpUnknownEvent
 *
 *@brief signals a IcmpUnknownEvent event to LinkProber state machine
 */
class IcmpUnknownEvent {
public:
    IcmpUnknownEvent() = default;
    ~IcmpUnknownEvent() = default;
};

/**
 *@class SuspendTimerExpiredEvent
 *
 *@brief signals a SuspendTimerExpiredEvent event to LinkProber state machine
 */
class SuspendTimerExpiredEvent {
public:
    SuspendTimerExpiredEvent() = default;
    ~SuspendTimerExpiredEvent() = default;
};

/**
 *@class SwitchActiveCommandCompleteEvent
 *
 *@brief signals a SwitchActiveCommandCompleteEvent event to LinkProber state machine
 */
class SwitchActiveCommandCompleteEvent {
public:
    SwitchActiveCommandCompleteEvent() = default;
    ~SwitchActiveCommandCompleteEvent() = default;
};

/**
 *@class SwitchActiveRequestEvent
 *
 *@brief signals a SwitchActiveRequestEvent event to LinkProber state machine
 */
class SwitchActiveRequestEvent {
public:
    SwitchActiveRequestEvent() = default;
    ~SwitchActiveRequestEvent() = default;
};

/**
 *@class LinkProberStateMachine
 *
 *@brief maintains LinkProber state machine
 */
class LinkProberStateMachine:  public common::StateMachine
{
public:
    /**
    *@method LinkProberStateMachine
    *
    *@brief class default constructor
    */
    LinkProberStateMachine() = delete;

    /**
    *@method LinkProberStateMachine
    *
    *@brief class copy constructor
    *
    *@param LinkProberStateMachine (in)  reference to LinkProberStateMachine object to be copied
    */
    LinkProberStateMachine(const LinkProberStateMachine &) = delete;

    /**
    *@method LinkProberStateMachine
    *
    *@brief class constructor
    *
    *@param linkManagerStateMachine (in)    reference to LinkManagerStateMachine
    *@param strand (in)                     reference to boost serialization object
    *@param muxPortConfig (in)              reference to MuxPortConfig object
    *@param label (in)                      state machine initial state
    */
    LinkProberStateMachine(
        link_manager::LinkManagerStateMachine &linkManagerStateMachine,
        boost::asio::io_service::strand &strand,
        common::MuxPortConfig &muxPortConfig,
        LinkProberState::Label label
    );

    /**
    *@method ~LinkProberStateMachine
    *
    *@brief class destructor
    */
    virtual ~LinkProberStateMachine() = default;

    /**
    *@method enterState
    *
    *@brief force the state machine to enter a given state
    *
    *@param label (in)  label of target state
    *
    *@return none
    */
    void enterState(LinkProberState::Label label);

    /**
    *@method postLinkProberStateEvent
    *
    *@brief post LinkProberState event to the state machine
    *
    *@param e (in)  reference to the LinkProberState event
    *
    *@return none
    */
    template <class E>
    void postLinkProberStateEvent(E &e);

    /**
    *@method processEvent
    *
    *@brief process LinkProberState event
    *
    *@param t (in)  reference to the LinkProberState event
    *
    *@return none
    */
    template <typename T>
    void processEvent(T &t);

    /**
    *@method processEvent
    *
    *@brief process LinkProberState suspend timer expiry event
    *
    *@param suspendTimerExpiredEvent (in)  reference to the SuspendTimerExpiredEvent event
    *
    *@return none
    */
    void processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent);

    /**
    *@method processEvent
    *
    *@brief process LinkProberState send switch command completion
    *
    *@param switchActiveCommandCompleteEvent (in)  reference to the SwitchActiveCommandCompleteEvent event
    *
    *@return none
    */
    void processEvent(SwitchActiveCommandCompleteEvent &switchActiveCommandCompleteEvent);

    /**
    *@method processEvent
    *
    *@brief process LinkProberState switch active request
    *
    *@param switchActiveRequestEvent (in)  reference to the SwitchActiveRequestEvent event
    *
    *@return none
    */
    void processEvent(SwitchActiveRequestEvent &switchActiveRequestEvent);

    /**
    *@method handleMackAddressUpdate
    *
    *@brief process LinkProberState MAC address update event
    *
    *@param address (in)    Server MAC address
    *
    *@return none
    */
    void handleMackAddressUpdate(const std::array<uint8_t, ETHER_ADDR_LEN> address);

   /**
    *@method getActiveState
    *
    *@brief getter for ActiveState object
    *
    *@return pointer to ActiveState object
    */
    ActiveState* getActiveState() {return &mActiveState;};

    /**
    *@method getStandbyState
    *
    *@brief getter for StandbyState object
    *
    *@return pointer to StandbyState object
    */
    StandbyState* getStandbyState() {return &mStandbyState;};

    /**
    *@method getUnknownState
    *
    *@brief getter for UnknownState object
    *
    *@return pointer to UnknownState object
    */
    UnknownState* getUnknownState() {return &mUnknownState;};

    /**
    *@method getWaitState
    *
    *@brief getter for WaitState object
    *
    *@return pointer to WaitState object
    */
    WaitState* getWaitState() {return &mWaitState;};

    /**
    *@method getIcmpSelfEvent
    *
    *@brief getter for IcmpSelfEvent object
    *
    *@return pointer to IcmpSelfEvent object
    */
    static IcmpSelfEvent& getIcmpSelfEvent() {return mIcmpSelfEvent;};

    /**
    *@method getIcmpPeerEvent
    *
    *@brief getter for IcmpPeerEvent object
    *
    *@return pointer to IcmpPeerEvent object
    */
    static IcmpPeerEvent& getIcmpPeerEvent() {return mIcmpPeerEvent;};

    /**
    *@method getIcmpUnknownEvent
    *
    *@brief getter for IcmpUnknownEvent object
    *
    *@return pointer to IcmpUnknownEvent object
    */
    static IcmpUnknownEvent& getIcmpUnknownEvent() {return mIcmpUnknownEvent;};

    /**
    *@method getSuspendTimerExpiredEvent
    *
    *@brief getter for SuspendTimerExpiredEvent object
    *
    *@return pointer to SuspendTimerExpiredEvent object
    */
    static SuspendTimerExpiredEvent& getSuspendTimerExpiredEvent() {return mSuspendTimerExpiredEvent;};

    /**
    *@method getSwitchActiveCommandCompleteEvent
    *
    *@brief getter for SwitchActiveCommandCompleteEvent object
    *
    *@return pointer to SwitchActiveCommandCompleteEvent object
    */
    static SwitchActiveCommandCompleteEvent& getSwitchActiveCommandCompleteEvent() {return mSwitchActiveCommandCompleteEvent;};

    /**
    *@method getSwitchActiveRequestEvent
    *
    *@brief getter for SwitchActiveRequestEvent object
    *
    *@return pointer to SwitchActiveRequestEvent object
    */
    static SwitchActiveRequestEvent& getSwitchActiveRequestEvent() {return mSwitchActiveRequestEvent;};

private:
    /**
    *@method postLinkManagerEvent
    *
    *@brief post LinkProberState change event to LinkManager state machine
    *
    *@param linkProberState (in)    pointer to current LinkProberState
    *
    *@return none
    */
    inline void postLinkManagerEvent(LinkProberState* linkProberState);

private:
    static IcmpSelfEvent mIcmpSelfEvent;
    static IcmpPeerEvent mIcmpPeerEvent;
    static IcmpUnknownEvent mIcmpUnknownEvent;
    static SuspendTimerExpiredEvent mSuspendTimerExpiredEvent;
    static SwitchActiveCommandCompleteEvent mSwitchActiveCommandCompleteEvent;
    static SwitchActiveRequestEvent mSwitchActiveRequestEvent;

private:
    link_manager::LinkManagerStateMachine &mLinkManagerStateMachine;
    ActiveState mActiveState;
    StandbyState mStandbyState;
    UnknownState mUnknownState;
    WaitState mWaitState;
};

} /* namespace link_prober */

#endif /* LINK_PROBER_LINKPROBERSTATEMACHINE_H_ */
