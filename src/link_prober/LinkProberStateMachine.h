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

#include "link_prober/LinkProberStateMachineBase.h"

namespace link_manager {
class LinkManagerStateMachineBase;
} /* namespace link_manager */

namespace link_prober
{
/**
 *@class LinkProberStateMachine
 *
 *@brief maintains LinkProber state machine
 */
class LinkProberStateMachine:  public LinkProberStateMachineBase
{
public:
    using LinkProberStateMachineBase::processEvent;
    using LinkProberStateMachineBase::postLinkProberStateEvent;

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
    *@param linkManagerStateMachinePtr (in) pointer to LinkManagerStateMachineBase
    *@param strand (in)                     reference to boost serialization object
    *@param muxPortConfig (in)              reference to MuxPortConfig object
    *@param label (in)                      state machine initial state
    */
    LinkProberStateMachine(
        link_manager::LinkManagerStateMachineBase *linkManagerStateMachinePtr,
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
    void enterState(LinkProberState::Label label) override;

    /**
    *@method processEvent
    *
    *@brief process LinkProberState suspend timer expiry event
    *
    *@param suspendTimerExpiredEvent (in)  reference to the SuspendTimerExpiredEvent event
    *
    *@return none
    */
    void processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent) override;

    /**
    *@method processEvent
    *
    *@brief process LinkProberState send switch command completion
    *
    *@param switchActiveCommandCompleteEvent (in)  reference to the SwitchActiveCommandCompleteEvent event
    *
    *@return none
    */
    void processEvent(SwitchActiveCommandCompleteEvent &switchActiveCommandCompleteEvent) override;

    /**
    *@method processEvent
    *
    *@brief process LinkProberState switch active request
    *
    *@param switchActiveRequestEvent (in)  reference to the SwitchActiveRequestEvent event
    *
    *@return none
    */
    void processEvent(SwitchActiveRequestEvent &switchActiveRequestEvent) override;

    /**
    *@method handleMackAddressUpdate
    *
    *@brief process LinkProberState MAC address update event
    *
    *@param address (in)    Server MAC address
    *
    *@return none
    */
    void handleMackAddressUpdate(const std::array<uint8_t, ETHER_ADDR_LEN> address) override;

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
     * @method handlePckLossRatioUpdate
     * 
     * @brief post pck loss ratio update to link manager
     * 
     * @param unknownEventCount (in) count of missing icmp packets
     * @param expectedPacketCount (in) count of expected icmp packets
     * 
     * @return none
    */
    void handlePckLossRatioUpdate(const uint64_t unknownEventCount, const uint64_t expectedPacketCount);

    /**
     * @method handleAppDbStateRetrieved
     * 
     */

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
    ActiveState mActiveState;
    StandbyState mStandbyState;
    UnknownState mUnknownState;
    WaitState mWaitState;
};

} /* namespace link_prober */

#endif /* LINK_PROBER_LINKPROBERSTATEMACHINE_H_ */
