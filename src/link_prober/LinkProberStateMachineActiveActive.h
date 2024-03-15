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

#ifndef LINK_PROBER_LINKPROBERSTATEMACHINEACTIVEACTIVE_H_
#define LINK_PROBER_LINKPROBERSTATEMACHINEACTIVEACTIVE_H_

#include "link_prober/LinkProberStateMachineBase.h"

namespace link_manager
{
class LinkManagerStateMachineBase;
}

namespace link_prober
{

class LinkProberStateMachineActiveActive : public LinkProberStateMachineBase
{
public:
    using LinkProberStateMachineBase::postLinkProberStateEvent;
    using LinkProberStateMachineBase::processEvent;

public:
    /**
     *@method LinkProberStateMachineActiveActive
     *
     *@brief class default constructor
     */
    LinkProberStateMachineActiveActive() = delete;

    /**
     *@method LinkProberStateMachineActiveActive
     *
     *@brief class copy constructor
     *
     *@param LinkProberStateMachineActiveActive (in)  reference to LinkProberStateMachineActiveActive object to be copied
     */
    LinkProberStateMachineActiveActive(const LinkProberStateMachineActiveActive &) = delete;

    /**
     *@method LinkProberStateMachineActiveActive
     *
     *@brief class constructor
     *
     *@param linkManagerStateMachinePtr (in) pointer to LinkManagerStateMachineBase
     *@param strand (in)                     reference to boost serialization object
     *@param muxPortConfig (in)              reference to MuxPortConfig object
     *@param label (in)                      state machine initial state
     */
    LinkProberStateMachineActiveActive(
        link_manager::LinkManagerStateMachineBase *linkManagerStateMachinePtr,
        boost::asio::io_service::strand &strand,
        common::MuxPortConfig &muxPortConfig,
        LinkProberState::Label label
    );

    /**
     *@method ~LinkProberStateMachineActiveActive
     *
     *@brief class destructor
     */
    virtual ~LinkProberStateMachineActiveActive() = default;

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
     *@brief process IcmpPeerActiveEvent
     *
     *@param icmpPeerActiveEvent (in)  reference to the IcmpPeerActiveEvent event
     *
     *@return none
     */
    void processEvent(IcmpPeerActiveEvent &icmpPeerActiveEvent) override;

    /**
     *@method processEvent
     *
     *@brief process IcmpPeerUnknownEvent
     *
     *@param icmpPeerUnknownEvent (in)  reference to the IcmpPeerUnknownEvent event
     *
     *@return none
     */
    void processEvent(IcmpPeerUnknownEvent &icmpPeerUnknownEvent) override;

    /**
     *@method processEvent
     *
     *@brief process SuspendTimerExpiredEvent
     *
     *@param suspendTimerExpiredEvent (in)  reference to the SuspendTimerExpiredEvent event
     *
     *@return none
     */
    void processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent) override;

    /**
     *@method processEvent
     *
     *@brief process MuxProbeRequestEvent
     *
     *@param muxProbeRequestEvent (in)  reference to the MuxProbeRequestEvent event
     *
     *@return none
     */
    void processEvent(MuxProbeRequestEvent &muxProbeRequestEvent) override;

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
     *@method getCurrentPeerState
     *
     *@brief getter for current peer state
     *
     *@return current peer state of the state machine
     */
    LinkProberState *getCurrentPeerState() override;

    /**
     *@method enterPeerState
     *
     *@brief force the state machine to enter a given peer state
     *
     *@param label (in)  label of target peer state
     *
     *@return none
     */
    void enterPeerState(LinkProberState::Label label) override;

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

private:
    /**
     *@method setCurrentPeerState
     *
     *@brief setter for current peer state
     *
     *@param state (in)  current peer state of the state machine
     *
     *@return none
     */
    void setCurrentPeerState(LinkProberState *state);

private:
    /**
     *@method postLinkManagerPeerEvent
     *
     *@brief post peer LinkProberState change event to LinkManager state machine
     *
     *@param linkProberState (in)    pointer to current peer LinkProberState
     *
     *@return none
     */
    inline void postLinkManagerPeerEvent(LinkProberState *linkProberState);

private:
    LinkProberState *mCurrentPeerState = nullptr;
};
} /* namespace link_prober */

#endif /* LINK_PROBER_LINKPROBERSTATEMACHINEACTIVEACTIVE_H_ */
