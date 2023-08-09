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
#ifndef LINK_PROBER_LINKPROBERSTATEMACHINEBASE_H_
#define LINK_PROBER_LINKPROBERSTATEMACHINEBASE_H_

#include "common/StateMachine.h"
#include "link_prober/ActiveState.h"
#include "link_prober/PeerActiveState.h"
#include "link_prober/PeerUnknownState.h"
#include "link_prober/PeerWaitState.h"
#include "link_prober/StandbyState.h"
#include "link_prober/UnknownState.h"
#include "link_prober/WaitState.h"

namespace link_manager
{
class LinkManagerStateMachineBase;
} /* namespace link_manager */

namespace link_prober
{
/**
 *@class IcmpSelfEvent
 *
 *@brief signals a IcmpSelfEvent event to LinkProber state machine
 */
class IcmpSelfEvent
{
public:
    IcmpSelfEvent() = default;
    ~IcmpSelfEvent() = default;
};

/**
 *@class IcmpPeerEvent
 *
 *@brief signals a IcmpPeerEvent event to LinkProber state machine
 */
class IcmpPeerEvent
{
public:
    IcmpPeerEvent() = default;
    ~IcmpPeerEvent() = default;
};

/**
 *@class IcmpUnknownEvent
 *
 *@brief signals a IcmpUnknownEvent event to LinkProber state machine
 */
class IcmpUnknownEvent
{
public:
    IcmpUnknownEvent() = default;
    ~IcmpUnknownEvent() = default;
};

/**
 *@class IcmpPeerActiveEvent
 *
 *@brief signals a IcmpPeerActiveEvent event to LinkProber state machine
 */
class IcmpPeerActiveEvent
{
public:
    IcmpPeerActiveEvent() = default;
    ~IcmpPeerActiveEvent() = default;
};

/**
 *@class IcmpPeerUnknownEvent
 *
 *@brief signals a IcmpPeerUnknownEvent event to LinkProber state machine
 */
class IcmpPeerUnknownEvent
{
public:
    IcmpPeerUnknownEvent() = default;
    ~IcmpPeerUnknownEvent() = default;
};

/**
 *@class SuspendTimerExpiredEvent
 *
 *@brief signals a SuspendTimerExpiredEvent event to LinkProber state machine
 */
class SuspendTimerExpiredEvent
{
public:
    SuspendTimerExpiredEvent() = default;
    ~SuspendTimerExpiredEvent() = default;
};

/**
 *@class SwitchActiveCommandCompleteEvent
 *
 *@brief signals a SwitchActiveCommandCompleteEvent event to LinkProber state machine
 */
class SwitchActiveCommandCompleteEvent
{
public:
    SwitchActiveCommandCompleteEvent() = default;
    ~SwitchActiveCommandCompleteEvent() = default;
};

/**
 *@class SwitchActiveRequestEvent
 *
 *@brief signals a SwitchActiveRequestEvent event to LinkProber state machine
 */
class SwitchActiveRequestEvent
{
public:
    SwitchActiveRequestEvent() = default;
    ~SwitchActiveRequestEvent() = default;
};

/**
 *@class MuxProbeRequestEvent
 *
 *@brief signals a MuxProbeRequestEvent event to LinkProber state machine
 */
class MuxProbeRequestEvent
{
public:
    MuxProbeRequestEvent() = default;
    ~MuxProbeRequestEvent() = default;
};

class LinkProberStateMachineActiveStandby;
class LinkProberStateMachineActiveActive;

/**
 *@class LinkProberStateMachineBase
 *
 *@brief base link prober state machine class to maintains common interfaces
 */
class LinkProberStateMachineBase : public common::StateMachine
{
public:
    /**
     *@method LinkProberStateMachineBase
     *
     *@brief class default constructor
     */
    LinkProberStateMachineBase() = delete;

    /**
     *@method LinkProberStateMachineBase
     *
     *@brief class copy constructor
     *
     *@param LinkProberStateMachineBase (in)  reference to LinkProberStateMachineBase object to be copied
     */
    LinkProberStateMachineBase(const LinkProberStateMachineBase &) = delete;

    /**
     *@method LinkProberStateMachineBase
     *
     *@brief class constructor
     *
     *@param linkManagerStateMachinePtr (in) pointer to LinkManagerStateMachineBase
     *@param strand (in)                     reference to boost serialization object
     *@param muxPortConfig (in)              reference to MuxPortConfig object
     *@param label (in)                      state machine initial state
     */
    LinkProberStateMachineBase(
        link_manager::LinkManagerStateMachineBase *linkManagerStateMachinePtr,
        boost::asio::io_service::strand &strand,
        common::MuxPortConfig &muxPortConfig
    );

    /**
     *@method ~LinkProberStateMachineBase
     *
     *@brief class destructor
     */
    virtual ~LinkProberStateMachineBase() = default;

    /**
     *@method enterState
     *
     *@brief force the state machine to enter a given state
     *
     *@param label (in)  label of target state
     *
     *@return none
     */
    virtual void enterState(LinkProberState::Label label) = 0;

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
     *@brief process IcmpPeerActiveEvent
     *
     *@param icmpPeerActiveEvent (in)  reference to the IcmpPeerActiveEvent event
     *
     *@return none
     */
    virtual void processEvent(IcmpPeerActiveEvent &icmpPeerActiveEvent);

    /**
     *@method processEvent
     *
     *@brief process IcmpPeerUnknownEvent
     *
     *@param icmpPeerUnknownEvent (in)  reference to the IcmpPeerUnknownEvent event
     *
     *@return none
     */
    virtual void processEvent(IcmpPeerUnknownEvent &icmpPeerUnknownEvent);

    /**
     *@method processEvent
     *
     *@brief process LinkProberState suspend timer expiry event
     *
     *@param suspendTimerExpiredEvent (in)  reference to the SuspendTimerExpiredEvent event
     *
     *@return none
     */
    virtual void processEvent(SuspendTimerExpiredEvent &suspendTimerExpiredEvent);

    /**
     *@method processEvent
     *
     *@brief process LinkProberState send switch command completion
     *
     *@param switchActiveCommandCompleteEvent (in)  reference to the SwitchActiveCommandCompleteEvent event
     *
     *@return none
     */
    virtual void processEvent(SwitchActiveCommandCompleteEvent &switchActiveCommandCompleteEvent);

    /**
     *@method processEvent
     *
     *@brief process LinkProberState switch active request
     *
     *@param switchActiveRequestEvent (in)  reference to the SwitchActiveRequestEvent event
     *
     *@return none
     */
    virtual void processEvent(SwitchActiveRequestEvent &switchActiveRequestEvent);

    /**
     *@method processEvent
     *
     *@brief process LinkProberState mux probe request
     *
     *@param muxProbeRequestEvent (in)  reference to the MuxProbeRequestEvent event
     *
     *@return none
     */
    virtual void processEvent(MuxProbeRequestEvent &muxProbeRequestEvent);

    /**
     *@method handleMackAddressUpdate
     *
     *@brief process LinkProberState MAC address update event
     *
     *@param address (in)    Server MAC address
     *
     *@return none
     */
    virtual void handleMackAddressUpdate(const std::array<uint8_t, ETHER_ADDR_LEN> address);

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
    virtual void handlePckLossRatioUpdate(const uint64_t unknownEventCount, const uint64_t expectedPacketCount);

public:
    /**
     *@method getCurrentPeerState
     *
     *@brief getter for current peer state
     *
     *@return current peer state of the state machine
     */
    virtual LinkProberState *getCurrentPeerState();

    /**
     *@method enterPeerState
     *
     *@brief force the state machine to enter a given peer state
     *
     *@param label (in)  label of target peer state
     *
     *@return none
     */
    virtual void enterPeerState(LinkProberState::Label label);

public:
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
    *@method getPeerActiveState
    *
    *@brief getter for PeerActiveState object
    *
    *@return pointer to PeerActiveState object
    */
    PeerActiveState* getPeerActiveState() {return &mPeerActiveState;};

    /**
    *@method getPeerUnknownState
    *
    *@brief getter for PeerUnknownState object
    *
    *@return pointer to PeerUnknownState object
    */
    PeerUnknownState* getPeerUnknownState() {return &mPeerUnknownState;};

    /**
    *@method getPeerWaitState
    *
    *@brief getter for PeerWaitState object
    *
    *@return pointer to PeerWaitState object
    */
    PeerWaitState* getPeerWaitState() {return &mPeerWaitState;};

public:
    /**
     *@method resetCurrentState
     *
     *@brief reset current link prober state
     */
    void resetCurrentState();

public:
    /**
     *@method getIcmpSelfEvent
     *
     *@brief getter for IcmpSelfEvent object
     *
     *@return pointer to IcmpSelfEvent object
     */
    static IcmpSelfEvent &getIcmpSelfEvent() { return mIcmpSelfEvent; };

    /**
     *@method getIcmpPeerEvent
     *
     *@brief getter for IcmpPeerEvent object
     *
     *@return pointer to IcmpPeerEvent object
     */
    static IcmpPeerEvent &getIcmpPeerEvent() { return mIcmpPeerEvent; };

    /**
     *@method getIcmpUnknownEvent
     *
     *@brief getter for IcmpUnknownEvent object
     *
     *@return pointer to IcmpUnknownEvent object
     */
    static IcmpUnknownEvent &getIcmpUnknownEvent() { return mIcmpUnknownEvent; };

    /**
     *@method getSuspendTimerExpiredEvent
     *
     *@brief getter for SuspendTimerExpiredEvent object
     *
     *@return pointer to SuspendTimerExpiredEvent object
     */
    static SuspendTimerExpiredEvent &getSuspendTimerExpiredEvent() { return mSuspendTimerExpiredEvent; };

    /**
     *@method getSwitchActiveCommandCompleteEvent
     *
     *@brief getter for SwitchActiveCommandCompleteEvent object
     *
     *@return pointer to SwitchActiveCommandCompleteEvent object
     */
    static SwitchActiveCommandCompleteEvent &getSwitchActiveCommandCompleteEvent() { return mSwitchActiveCommandCompleteEvent; };

    /**
     *@method getSwitchActiveRequestEvent
     *
     *@brief getter for SwitchActiveRequestEvent object
     *
     *@return pointer to SwitchActiveRequestEvent object
     */
    static SwitchActiveRequestEvent &getSwitchActiveRequestEvent() { return mSwitchActiveRequestEvent; };

    /**
     *@method getMuxProbeRequestEvent
     *
     *@brief getter for MuxProbeRequestEvent object
     *
     *@return pointer to MuxProbeRequestEvent object
     */
    static MuxProbeRequestEvent &getMuxProbeRequestEvent() { return mMuxProbeRequestEvent; };

    /**
     *@method getIcmpPeerActiveEvent
     *
     *@brief getter for IcmpPeerActiveEvent object
     *
     *@return pointer to IcmpPeerActiveEvent object
     */
    static IcmpPeerActiveEvent &getIcmpPeerActiveEvent() { return mIcmpPeerActiveEvent; }

    /**
     *@method getIcmpPeerUnknownEvent
     *
     *@brief getter for IcmpPeerUnknownEvent object
     *
     *@return pointer to IcmpPeerUnknownEvent object
     */
    static IcmpPeerUnknownEvent &getIcmpPeerUnknownEvent() { return mIcmpPeerUnknownEvent; }

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
    inline void postLinkManagerEvent(LinkProberState *linkProberState);

private:
    static IcmpSelfEvent mIcmpSelfEvent;
    static IcmpPeerEvent mIcmpPeerEvent;
    static IcmpUnknownEvent mIcmpUnknownEvent;
    static SuspendTimerExpiredEvent mSuspendTimerExpiredEvent;
    static SwitchActiveCommandCompleteEvent mSwitchActiveCommandCompleteEvent;
    static SwitchActiveRequestEvent mSwitchActiveRequestEvent;
    static MuxProbeRequestEvent mMuxProbeRequestEvent;
    static IcmpPeerActiveEvent mIcmpPeerActiveEvent;
    static IcmpPeerUnknownEvent mIcmpPeerUnknownEvent;

private:
    friend class LinkProberStateMachineActiveStandby;
    friend class LinkProberStateMachineActiveActive;

private:
    link_manager::LinkManagerStateMachineBase *mLinkManagerStateMachinePtr;
    ActiveState mActiveState;
    StandbyState mStandbyState;
    UnknownState mUnknownState;
    WaitState mWaitState;
    PeerActiveState mPeerActiveState;
    PeerUnknownState mPeerUnknownState;
    PeerWaitState mPeerWaitState;
};
} // namespace link_prober

#endif
