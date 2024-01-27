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
 * LinkProberStateMachineActiveStandby.h
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
 *@class LinkProberStateMachineActiveStandby
 *
 *@brief maintains LinkProber state machine
 */
class LinkProberStateMachineActiveStandby:  public LinkProberStateMachineBase
{
public:
    using LinkProberStateMachineBase::processEvent;
    using LinkProberStateMachineBase::postLinkProberStateEvent;

public:
    /**
    *@method LinkProberStateMachineActiveStandby
    *
    *@brief class default constructor
    */
    LinkProberStateMachineActiveStandby() = delete;

    /**
    *@method LinkProberStateMachineActiveStandby
    *
    *@brief class copy constructor
    *
    *@param LinkProberStateMachineActiveStandby (in)  reference to LinkProberStateMachineActiveStandby object to be copied
    */
    LinkProberStateMachineActiveStandby(const LinkProberStateMachineActiveStandby &) = delete;

    /**
    *@method LinkProberStateMachineActiveStandby
    *
    *@brief class constructor
    *
    *@param linkManagerStateMachinePtr (in) pointer to LinkManagerStateMachineBase
    *@param strand (in)                     reference to boost serialization object
    *@param muxPortConfig (in)              reference to MuxPortConfig object
    *@param label (in)                      state machine initial state
    */
    LinkProberStateMachineActiveStandby(
        link_manager::LinkManagerStateMachineBase *linkManagerStateMachinePtr,
        boost::asio::io_service::strand &strand,
        common::MuxPortConfig &muxPortConfig,
        LinkProberState::Label label
    );

    /**
    *@method ~LinkProberStateMachineActiveStandby
    *
    *@brief class destructor
    */
    virtual ~LinkProberStateMachineActiveStandby() = default;

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
     *@brief process LinkProberPeerUpEvent
     *
     *@param linkProberPeerUpEvent (in)  reference to the LinkProberPeerUpEvent event
     *
     *@return none
     */
    void processEvent(LinkProberPeerUpEvent &linkProberPeerUpEvent) override;

    /**
     *@method processEvent
     *
     *@brief process LinkProberPeerDownEvent
     *
     *@param linkProberPeerDownEvent (in)  reference to the LinkProberPeerDownEvent event
     *
     *@return none
     */
    void processEvent(LinkProberPeerDownEvent &linkProberPeerDownEvent) override;

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
     * @method handlePckLossRatioUpdate
     * 
     * @brief post pck loss ratio update to link manager
     * 
     * @param unknownEventCount (in) count of missing icmp packets
     * @param expectedPacketCount (in) count of expected icmp packets
     * 
     * @return none
    */
    void handlePckLossRatioUpdate(const uint64_t unknownEventCount, const uint64_t expectedPacketCount) override;
};

} /* namespace link_prober */

#endif /* LINK_PROBER_LINKPROBERSTATEMACHINE_H_ */
