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
 * ActiveStandbyStateMachine.h
 *
 *  Created on: Oct 18, 2020
 *      Author: tamer
 */

#ifndef LINK_MANAGER_LINKMANAGERSTATEMACHINEACTIVESTANDBY_H_
#define LINK_MANAGER_LINKMANAGERSTATEMACHINEACTIVESTANDBY_H_

#include <bitset>
#include <functional>
#include <string>
#include <tuple>
#include <vector>
#include <boost/function.hpp>

#include "link_manager/LinkManagerStateMachineBase.h"
#include "link_prober/LinkProber.h"
#include "link_prober/LinkProberState.h"
#include "link_state/LinkState.h"
#include "link_state/LinkStateMachine.h"
#include "mux_state/MuxState.h"
#include "mux_state/MuxStateMachine.h"

namespace test {
class FakeMuxPort;
class MuxManagerTest;
}

namespace mux {
#define ps(compositeState)      std::get<0>(compositeState)
#define ms(compositeState)      std::get<1>(compositeState)
#define ls(compositeState)      std::get<2>(compositeState)

class MuxPort;
}

namespace link_manager
{

/**
 *@class ActiveStandbyStateMachine
 *
 *@brief Composite state machine of LinkProberState, MuxState, and LinkState
 */
class ActiveStandbyStateMachine: public LinkManagerStateMachineBase,
                               public std::enable_shared_from_this<ActiveStandbyStateMachine>
{
public:
    enum class SwitchCause {
        PeerHeartbeatMissing,
        PeerLinkDown,
        TlvSwitchActiveCommand,
        LinkDown,
        TransceiverDaemonTimeout,
        MatchingHardwareState,
        ConfigMuxMode,
        HarewareStateUnknown,

        Count
    };
public:
    /**
    *@method ActiveStandbyStateMachine
    *
    *@brief class default constructor
    */
    ActiveStandbyStateMachine() = delete;

    /**
    *@method ActiveStandbyStateMachine
    *
    *@brief class copy constructor
    *
    *@param ActiveStandbyStateMachine (in)  reference to ActiveStandbyStateMachine object to be copied
    */
    ActiveStandbyStateMachine(const ActiveStandbyStateMachine &) = delete;

    /**
    *@method ActiveStandbyStateMachine
    *
    *@brief class constructor
    *
    *@param muxPortPtr (in)     pointer to container MuxPort object
    *@param strand (in)         boost serialization object
    *@param muxPortConfig (in)  reference to MuxPortConfig object
    */
    ActiveStandbyStateMachine(
        mux::MuxPort *muxPortPtr,
        boost::asio::io_service::strand &strand,
        common::MuxPortConfig &muxPortConfig
    );

    /**
    *@method ~ActiveStandbyStateMachine
    *
    *@brief class destructor
    */
    virtual ~ActiveStandbyStateMachine() = default;

    /**
    *@method initializeTransitionFunctionTable
    *
    *@brief initialize static transition function table
    *
    *@return none
    */
    void initializeTransitionFunctionTable();

private:
    /**
    *@method setLabel
    *
    *@brief sets linkmgr State db state
    *
    *@return none
    */
    inline void setLabel(Label label);

    /**
    *@method enterLinkProberState
    *
    *@brief force LinkProberState to switch state
    *
    *@param nextState (in, out)     reference to composite state, the state linkProber
    *                               entry will be changed to align with state label provided
    *@param label (in)              state to switch to
    *
    *@param forceReset (in)         force reset the aligned link prober state
    *
    *@return none
    */
    inline void enterLinkProberState(CompositeState &nextState, link_prober::LinkProberState::Label label, bool forceReset=false);

    /**
    *@method enterMuxState
    *
    *@brief force MuxState to switch state
    *
    *@param nextState (in, out)     reference to composite state, the state MuxState
    *                               entry will be changed to align with state label provided
    *@param label (in)              state to switch to
    *
    *@return none
    */
    inline void enterMuxState(CompositeState &nextState, mux_state::MuxState::Label label);

    /**
    *@method enterLinkState
    *
    *@brief force LinkState to switch state
    *
    *@param nextState (in, out)     reference to composite state, the state LinkState
    *                               entry will be changed to align with state label provided
    *@param label (in)              state to switch to
    *
    *@return none
    */
    inline void enterLinkState(CompositeState &nextState, link_state::LinkState::Label label);

    /**
    *@method enterMuxWaitState
    *
    *@brief force MuxState to switch to WaitState
    *
    *@param nextState (in, out)     reference to composite state, the state MuxState
    *                               entry will be changed to WaitState
    *
    *@return none
    */
    inline void enterMuxWaitState(CompositeState &nextState);

    /**
    *@method switchMuxState
    *
    *@brief switch Mux to switch via xcvrd to state label provider
    *
    *@param nextState (in, out)     reference to composite state, the state MuxState
    *                               entry will be changed to align with state label provided
    *@param label (in)              state to switch to
    *@param forceSwitch (in)        Force switch mux state, used to match the driver state only
    *
    *@return none
    */
    inline void switchMuxState(link_manager::ActiveStandbyStateMachine::SwitchCause cause, CompositeState &nextState, mux_state::MuxState::Label label, bool forceSwitch = false);

    /**
     * @method shutdownOrRestartLinkProberOnDefaultRoute()
     * 
     * @brief  shutdown or restart link prober based on default route state
     * 
     * @return none
     */
    void shutdownOrRestartLinkProberOnDefaultRoute();

public:
    /**
    *@method handleSwssBladeIpv4AddressUpdate
    *
    *@brief initialize LinkProber component. Note if this is the last component to be initialized,
    *       state machine will be activated
    *
    *@return none
    */
    void handleSwssBladeIpv4AddressUpdate(boost::asio::ip::address address);

    /**
    *@method activateStateMachine
    *
    *@brief activate the state machine by starting the LinkProber. This should be done after all
    *       components have been initialized.
    *
    *@return none
    */
    void activateStateMachine();

    /**
    *@method handleStateChange
    *
    *@brief handles LinkProverEvent
    *
    *@param state (in)  new LinkProberState label
    *
    *@return none
    */
    void handleStateChange(LinkProberEvent &event, link_prober::LinkProberState::Label state);

    /**
    *@method handleStateChange
    *
    *@brief handles MuxStateEvent
    *
    *@param state (in)  new MuxState label
    *
    *@return none
    */
    void handleStateChange(MuxStateEvent &event, mux_state::MuxState::Label state);

    /**
    *@method handleStateChange
    *
    *@brief handles LinkStateEvent
    *
    *@param state (in)  new LinkState label
    *
    *@return none
    */
    void handleStateChange(LinkStateEvent &event, link_state::LinkState::Label state);

    /**
    *@method handleGetServerMacNotification
    *
    *@brief handle get Server MAC address
    *
    *@param address (in)    Server MAC address
    *
    *@return none
    */
    void handleGetServerMacAddressNotification(std::array<uint8_t, ETHER_ADDR_LEN> address);

    /**
    *@method handleGetMuxStateNotification
    *
    *@brief handle get MUX state notification
    *
    *@param label (in)              new MuxState label
    *
    *@return none
    */
    void handleGetMuxStateNotification(mux_state::MuxState::Label label);

    /**
    *@method handleProbeMuxStateNotification
    *
    *@brief handle probe MUX state notification
    *
    *@param label (in)              new MuxState label
    *
    *@return none
    */
    void handleProbeMuxStateNotification(mux_state::MuxState::Label label);

    /**
    *@method handleMuxStateNotification
    *
    *@brief handle MUX state notification
    *
    *@param label (in)              new MuxState label
    *
    *@return none
    */
    void handleMuxStateNotification(mux_state::MuxState::Label label);

    /**
    *@method handleSwssLinkStateNotification
    *
    *@brief handle link state change notification
    *
    *@param label (in)  new LinkState label
    *
    *@return none
    */
    void handleSwssLinkStateNotification(const link_state::LinkState::Label label);
    
    /**
     * @method handlePeerLinkStateNotification
     * 
     * @brief handle peer link state notification
     * 
     * @param label (in) new peer link state label
     * 
     * @return none
    */
    void handlePeerLinkStateNotification(const link_state::LinkState::Label label);

    /**
    *@method handleMuxConfigNotification
    *
    *@brief handle MUX configuration change notification
    *
    *@param mode (in)  new MUX config mode
    *
    *@return none
    */
    void handleMuxConfigNotification(const common::MuxPortConfig::Mode mode);

    /**
    *@method handleSuspendTimerExpiry
    *
    *@brief handle suspend timer expiry notification from LinkProber
    *
    *@return none
    */
    void handleSuspendTimerExpiry();

    /**
    *@method handleSwitchActiveCommandCompletion
    *
    *@brief handle completion of sending switch command to peer ToR
    *
    *@return none
    */
    void handleSwitchActiveCommandCompletion();

    /**
    *@method handleSwitchActiveRequestEvent
    *
    *@brief handle switch active request from peer ToR
    *
    *@return none
    */
    void handleSwitchActiveRequestEvent();

    /**
     * @method handleDefaultRouteStateNotification(const DefaultRoute routeState)
     * 
     * @brief handle default route state notification from routeorch
     * 
     * @param routeState
     * 
     * @return none
    */
    void handleDefaultRouteStateNotification(const DefaultRoute routeState);

    /**
     * @method handlePostPckLossRatioNotification
     * 
     * @brief handle get post pck loss ratio 
     * 
     * @param unknownEventCount (in) count of missing icmp packets
     * @param expectedPacketCount (in) count of expected icmp packets
     * 
     * @return none
    */
    void handlePostPckLossRatioNotification(const uint64_t unknownEventCount, const uint64_t expectedPacketCount);

    /**
     * @method handleResetLinkProberPckLossCount
     * 
     * @brief reset link prober heartbeat packet loss count 
     * 
     * @return none
    */
    void handleResetLinkProberPckLossCount();

private:
    /**
    *@method updateMuxLinkmgrState
    *
    *@brief Update State DB MUX LinkMgr state
    *
    *@return none
    */
    void updateMuxLinkmgrState();

    /**
    *@method startMuxProbeTimer
    *
    *@brief start a timer to monitor the MUX state
    *
    *@param factor      factor used to scale the timeout
    *
    *@return none
    */
    inline void startMuxProbeTimer(uint32_t factor = 1);

    /**
    *@method handleLinkWaitTimeout
    *
    *@brief handle when LinkProber heartbeats were lost due link down, bad cable, server down or state mismatching
    *
    *@param errorCode (in)          timer error code
    *
    *@return none
    */
    void handleMuxProbeTimeout(boost::system::error_code errorCode);

    /**
    *@method startMuxWaitTimer
    *
    *@brief start a timer to timeout xcvrd/orchagent
    *
    *@param factor      factor used to scale the timeout
    *
    *@return none
    */
    inline void startMuxWaitTimer(uint32_t factor = 1);

    /**
    *@method handleMuxWaitTimeout
    *
    *@brief handle when xcrvrd/orchagent has timed out responding to linkmgrd.
    *
    *@param errorCode (in)          timer error code
    *
    *@return none
    */
    void handleMuxWaitTimeout(boost::system::error_code errorCode);

    /**
    *@method initLinkProberState
    *
    *@brief initialize LinkProberState when configuring the composite state machine
    *
    *@param compositeState (in, out)    reference to composite state, the state linkProber
    *                                   entry will be changed to align with MuxState
    *
    *@param forceReset (in)             force reset the aligned link prober state
    *
    *@return none
    */
    void initLinkProberState(CompositeState &compositeState, bool forceReset=false);

    /**
    *@method LinkProberStandbyMuxActiveLinkUpTransitionFunction
    *
    *@brief transition function when entering {LinkProberStandby, MuxActive, LinkUp} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberStandbyMuxActiveLinkUpTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberUnknownMuxActiveLinkUpTransitionFunction
    *
    *@brief transition function when entering {LinkProberUnknown, MuxActive, LinkUp} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberUnknownMuxActiveLinkUpTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberActiveMuxStandbyLinkUpTransitionFunction
    *
    *@brief transition function when entering {LinkProberActive, MuxStandby, LinkUp} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberActiveMuxStandbyLinkUpTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberUnknownMuxStandbyLinkUpTransitionFunction
    *
    *@brief transition function when entering {LinkProberUnknown, MuxStandby, LinkUp} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberUnknownMuxStandbyLinkUpTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberActiveMuxUnknownLinkUpTransitionFunction
    *
    *@brief transition function when entering {LinkProberActive, MuxUnknown, LinkUp} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberActiveMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberStandbyMuxUnknownLinkUpTransitionFunction
    *
    *@brief transition function when entering {LinkProberStandby, MuxUnknown, LinkUp} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberStandbyMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberUnknownMuxUnknownLinkUpTransitionFunction
    *
    *@brief transition function when entering {LinkProberUnknown, MuxUnknown, LinkUp} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberUnknownMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberActiveMuxErrorLinkUpTransitionFunction
    *
    *@brief transition function when entering {LinkProberActive, MuxError, LinkUp} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberActiveMuxErrorLinkUpTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberStandbyMuxErrorLinkUpTransitionFunction
    *
    *@brief transition function when entering {LinkProberStandby, MuxError, LinkUp} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberStandbyMuxErrorLinkUpTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberWaitMuxActiveLinkUpTransitionFunction
    *
    *@brief transition function when entering {LinkProberWait, MuxActive, LinkUp} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberWaitMuxActiveLinkUpTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberWaitMuxStandbyLinkUpTransitionFunction
    *
    *@brief transition function when entering {LinkProberWait, MuxStandby, LinkUp} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberWaitMuxStandbyLinkUpTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberWaitMuxUnknownLinkUpTransitionFunction
    *
    *@brief transition function when entering {LinkProberWait, MuxUnknown, LinkUp} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberWaitMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberUnknownMuxActiveLinkDownTransitionFunction
    *
    *@brief transition function when entering {LinkProberUnknown, MuxActive, LinkDown} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberUnknownMuxActiveLinkDownTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberUnknownMuxStandbyLinkDownTransitionFunction
    *
    *@brief transition function when entering {LinkProberUnknown, MuxStandby, LinkDown} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberUnknownMuxStandbyLinkDownTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberUnknownMuxUnknownLinkDownTransitionFunction
    *
    *@brief transition function when entering {LinkProberUnknown, MuxUnknown, LinkDown} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberUnknownMuxUnknownLinkDownTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberWaitMuxActiveLinkDownTransitionFunction
    *
    *@brief transition function when entering {LinkProberWait, MuxActive, LinkDown} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberWaitMuxActiveLinkDownTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberWaitMuxStandbyLinkDownTransitionFunction
    *
    *@brief transition function when entering {LinkProberWait, MuxStandby, LinkDown} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberWaitMuxStandbyLinkDownTransitionFunction(CompositeState &nextState);

    /**
    *@method LinkProberWaitMuxUnknownLinkDownTransitionFunction
    *
    *@brief transition function when entering {LinkProberWait, MuxUnknown, LinkDown} state
    *
    *@param nextState (in, out)     reference to composite state, the state will be changed
    *                               to reflect next new state.
    *
    *@return none
    */
    void LinkProberWaitMuxUnknownLinkDownTransitionFunction(CompositeState &nextState);

private:
    // This is used for testing...
    friend class mux::MuxPort;
    friend class test::FakeMuxPort;
    friend class test::MuxManagerTest;

    /**
    *@method setInitializeProberFnPtr
    *
    *@brief set new InitializeProberFnPtr for the state machine. This method is used for testing
    *
    *@param initializeProberFnPtr (in)  pointer to new InitializeProberFnPtr
    *
    *@return none
    */
    void setInitializeProberFnPtr(boost::function<void ()> initializeProberFnPtr) {mInitializeProberFnPtr = initializeProberFnPtr;};

    /**
    *@method setStartProbingFnPtr
    *
    *@brief set new StartProbingFnPtr for the state machine. This method is used for testing
    *
    *@param startProbingFnPtr (in)  pointer to new StartProbingFnPtr
    *
    *@return none
    */
    void setStartProbingFnPtr(boost::function<void ()> startProbingFnPtr) {mStartProbingFnPtr = startProbingFnPtr;};

    /**
    *@method setUpdateEthernetFrameFnPtr
    *
    *@brief set new UpdateEthernetFrameFnPtr for the state machine. This method is used for testing
    *
    *@param updateEthernetFrameFnPtr (in)  pointer to new UpdateEthernetFrameFnPtr
    *
    *@return none
    */
    void setUpdateEthernetFrameFnPtr(boost::function<void ()> updateEthernetFrameFnPtr) {mUpdateEthernetFrameFnPtr = updateEthernetFrameFnPtr;};

    /**
    *@method setProbePeerTorFnPtr
    *
    *@brief set new ProbePeerTorFnPtr for the state machine. This method is used for testing
    *
    *@param probePeerTorFnPtr (in)  pointer to new ProbePeerTorFnPtr
    *
    *@return none
    */
    void setProbePeerTorFnPtr(boost::function<void ()> probePeerTorFnPtr) {mProbePeerTorFnPtr = probePeerTorFnPtr;};

    /**
    *@method setSuspendTxFnPtr
    *
    *@brief set new SuspendTXFnPtr for the state machine. This method is used for testing
    *
    *@param suspendTxFnPtr (in)  pointer to new  SuspendTxFnPtr
    *
    *@return none
    */
    void setSuspendTxFnPtr(boost::function<void (uint32_t suspendTime_msec)> suspendTxFnPtr) {mSuspendTxFnPtr = suspendTxFnPtr;};

    /**
    *@method setResumeTxFnPtr
    *
    *@brief set new ResumeTXFnPtr for the state machine. This method is used for testing
    *
    *@param resumeTxFnPtr (in)  pointer to new  ResumeTxFnPtr
    *
    *@return none
    */
    void setResumeTxFnPtr(boost::function<void ()> resumeTxFnPtr) {mResumeTxFnPtr = resumeTxFnPtr;};

    /**
    *@method setSendPeerSwitchCommandFnPtr
    *
    *@brief set new SendPeerSwitchCommandFnPtr for the state machine. This method is used for testing
    *
    *@param sendPeerSwitchCommandFnPtr (in)  pointer to new  ResumeTxFnPtr
    *
    *@return none
    */
    void setSendPeerSwitchCommandFnPtr(boost::function<void ()> sendPeerSwitchCommandFnPtr) {
        mSendPeerSwitchCommandFnPtr = sendPeerSwitchCommandFnPtr;
    };

    /**
     * @method setShutdownTxFnPtr
     * 
     * @brief set shutdownTxFnPtr. This method is used for testing
     * 
     * @param shutdownTxFnPtr (in) pointer to new ShutdownTxFnPtr
     * 
     * @return none
     */
    void setShutdownTxFnPtr(boost::function<void ()> shutdownTxFnPtr) {
        mShutdownTxFnPtr = shutdownTxFnPtr;
    }

    /**
     * @method setRestartTxFnPtr
     * 
     * @brief set restartTxFnPtr. This method is used for testing
     * 
     * @param restartTxFnPtr (in) pointer to new restartTxFnPtr
     * 
     * @return none
     */
    void setRestartTxFnPtr(boost::function<void ()> restartTxFnPtr) {
        mRestartTxFnPtr = restartTxFnPtr;
    }

    /**
     * @method setDecreaseIntervalFnPtr
     * 
     * @brief set new DecreaseIntervalFnPtr for the state machine. This method is used for testing
     * 
     * @param DecreaseIntervalFnPtr (in) pointer to new DecreaseIntervalFnPtr
     * 
     * @return none
     */
    void setDecreaseIntervalFnPtr(boost::function<void (uint32_t switchTime_msec)> DecreaseIntervalFnPtr) {
        mDecreaseIntervalFnPtr = DecreaseIntervalFnPtr;
    };

    /**
     * @method setRevertIntervalFnPtr
     * 
     * @brief set new RevertIntervalFnPtr for the state machine. This method is used for testing
     * 
     * @param  RevertIntervalFnPtr (in) pointer to new RevertIntervalFnPtr
     * 
     * @return none
     */
    void setRevertIntervalFnPtr(boost::function<void ()>  RevertIntervalFnPtr) {
        mRevertIntervalFnPtr = RevertIntervalFnPtr;
    };

private:
    link_state::LinkState::Label mPeerLinkState = link_state::LinkState::Label::Down;

    boost::asio::deadline_timer mDeadlineTimer;
    boost::asio::deadline_timer mWaitTimer;

    boost::function<void ()> mInitializeProberFnPtr;
    boost::function<void ()> mStartProbingFnPtr;
    boost::function<void ()> mProbePeerTorFnPtr;
    boost::function<void (uint32_t suspendTime_msec)> mSuspendTxFnPtr;
    boost::function<void ()> mResumeTxFnPtr;
    boost::function<void ()> mSendPeerSwitchCommandFnPtr;
    boost::function<void ()> mResetIcmpPacketCountsFnPtr;
    boost::function<void ()> mShutdownTxFnPtr;
    boost::function<void ()> mRestartTxFnPtr;
    boost::function<void (uint32_t switchTime_msec)> mDecreaseIntervalFnPtr;
    boost::function<void ()> mRevertIntervalFnPtr;

    uint32_t mWaitActiveUpCount = 0;
    uint32_t mActiveUnknownUpCount = 0;
    uint32_t mStandbyUnknownUpCount = 0;
    uint32_t mMuxUnknownBackoffFactor = 1;
    uint32_t mWaitStandbyUpBackoffFactor = 1;
    uint32_t mUnknownActiveUpBackoffFactor = 1;

    bool mPendingMuxModeChange = false;
    common::MuxPortConfig::Mode mTargetMuxMode = common::MuxPortConfig::Mode::Auto;

    bool mContinuousLinkProberUnknownEvent = false; // When posting unknown_end event, we want to make sure the previous state is unknown.

    link_manager::ActiveStandbyStateMachine::SwitchCause mSendSwitchActiveCommandCause;
};

} /* namespace link_manager */

#endif /* LINK_MANAGER_LINKMANAGERSTATEMACHINEACTIVESTANDBY_H_ */
