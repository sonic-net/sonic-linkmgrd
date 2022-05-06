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

#ifndef LINK_MANAGER_LINKMANAGERSTATEMACHINEACTIVEACTIVE_H_
#define LINK_MANAGER_LINKMANAGERSTATEMACHINEACTIVEACTIVE_H_

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

namespace mux
{
#define ps(compositeState) std::get<0>(compositeState)
#define ms(compositeState) std::get<1>(compositeState)
#define ls(compositeState) std::get<2>(compositeState)

class MuxPort;
} // namespace mux

namespace link_manager
{

class ActiveActiveStateMachine : public LinkManagerStateMachineBase,
                                 public std::enable_shared_from_this<ActiveActiveStateMachine>
{
public:
    /**
     * @method ActiveActiveStateMachine
     *
     * @brief Construct a new Active Active State Machine object
     */
    ActiveActiveStateMachine() = delete;

    /**
     * @method ActiveActiveStateMachine
     *
     * @brief Construct a new Active Active State Machine object
     */
    ActiveActiveStateMachine(const ActiveActiveStateMachine &) = delete;

    /**
     * @method ActiveActiveStateMachine
     *
     * @brief Construct a new Active Active State Machine object
     *
     * @param muxPortPtr                    pointer to a MuxPort object
     * @param strand                        boost strand object
     * @param muxPortConfig                 reference to MuxPortConfig object
     */
    ActiveActiveStateMachine(
        mux::MuxPort *muxPortPtr,
        boost::asio::io_service::strand &strand,
        common::MuxPortConfig &muxPortConfig
    );

    /**
     * @method ~ActiveActiveStateMachine
     *
     * @brief Destroy the Active Active State Machine object
     */
    virtual ~ActiveActiveStateMachine() = default;

public:
    /**
     * @method activateStateMachine
     *
     * @brief activate the state machine by starting the link prober.
     */
    void activateStateMachine();

public: // db event handlers
    /**
     * @method handleSwssSoCIpv4AddressUpdate
     *
     * @brief initialize the link prober component. Note if this is the last component to be initialized,
     *        state machine will be activated
     *
     * @param address                       SoC IPv4 address
     */
    void handleSwssSoCIpv4AddressUpdate(boost::asio::ip::address address) override;

    /**
     * @method handleMuxStateNotification
     *
     * @brief handle MUX state notification
     *
     * @param label                         new MuxState label
     */
    void handleMuxStateNotification(mux_state::MuxState::Label label) override;

    /**
     * @method handleSwssLinkStateNotification
     *
     * @brief handle link state notification
     *
     * @param label                         new LinkState label
     */
    void handleSwssLinkStateNotification(const link_state::LinkState::Label label) override;

    /**
     * @method handleMuxConfigNotification
     *
     * @brief handle MUX configuration change notification
     *
     * @param mode                          new MUX config mode
     */
    void handleMuxConfigNotification(const common::MuxPortConfig::Mode mode) override;

    /**
     * @method handleProbeMuxStateNotification
     *
     * @brief handle probe MUX state notification
     *
     * @param label                         new MuxState label
     */
    void handleProbeMuxStateNotification(mux_state::MuxState::Label label) override;

    /**
     * @method handlePeerMuxStateNotification
     *
     * @brief handle peer MUX state notification
     *
     * @param label                         new peer MuxState label
     */
    void handlePeerMuxStateNotification(mux_state::MuxState::Label label) override;

public: // link prober event handlers
    /**
     * @method handleSuspendTimerExpiry
     *
     * @brief handle completion of sending switch command to peer ToR
     */
    void handleSuspendTimerExpiry() override;

public: // state handlers
    /**
     * @method handleStateChange
     *
     * @brief handles LinkProberEvent
     *
     * @param event                         LinkProberEvent object
     * @param state                         new LibProberState label
     */
    void handleStateChange(LinkProberEvent &event, link_prober::LinkProberState::Label state) override;

    /**
     * @method handleStateChange
     *
     * @brief handles MuxStateEvent
     *
     * @param event                         MuxStateEvent object
     * @param state                         new MuxState label
     */
    void handleStateChange(MuxStateEvent &event, mux_state::MuxState::Label state) override;

    /**
     * @method handleStateChange
     *
     * @brief handles LinkStateEvent
     *
     * @param event                         LinkStateEvent object
     * @param state                         new LinkState label
     */
    void handleStateChange(LinkStateEvent &event, link_state::LinkState::Label state) override;

    /**
     * @method handlePeerStateChange
     *
     * @brief handles peer LinkProberEvent
     *
     * @param event                         LinkProberEvent object
     * @param state                         new peer LinkState label
     */
    void handlePeerStateChange(LinkProberEvent &event, link_prober::LinkProberState::Label state) override;

public: // state transition functions
    /**
     * @method initializeTransitionFunctionTable
     *
     * @brief initialize static transition function table
     */
    void initializeTransitionFunctionTable() override;

    /**
     * @method LinkProberActiveMuxStandbyLinkUpTransitionFunction
     *
     * @brief transition function when entering {LinkProberActive, MuxStandby, LinkUp} state
     *
     * @param nextState                     reference to composite state
     */
    void LinkProberActiveMuxStandbyLinkUpTransitionFunction(CompositeState &nextState);

    /**
     * @method LinkProberActiveMuxUnknownLinkUpTransitionFunction
     *
     * @brief transition function when entering {LinkProberActive, MuxUnknown, LinkUp} state
     *
     * @param nextState                     reference to composite state
     */
    void LinkProberActiveMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);

    /**
     * @method LinkProberUnknownMuxActiveLinkUpTransitionFunction
     *
     * @brief transition function when entering {LinkProberUnknown, MuxActive, LinkUp} state
     *
     * @param nextState                     reference to composite state
     */
    void LinkProberUnknownMuxActiveLinkUpTransitionFunction(CompositeState &nextState);

    /**
     * @method LinkProberUnknownMuxUnknownLinkUpTransitionFunction
     *
     * @brief transition function when entering {LinkProberUnknown, MuxUnknown, LinkUp} state
     *
     * @param nextState                     reference to composite state
     */
    void LinkProberUnknownMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);

    /**
     * @method LinkProberActiveMuxErrorLinkUpTransitionFunction
     *
     * @brief transition function when entering {LinkProberActive, MuxError, LinkUp} state
     *
     * @param nextState                     reference to composite state
     */
    void LinkProberActiveMuxErrorLinkUpTransitionFunction(CompositeState &nextState);

    /**
     * @method LinkProberActiveMuxWaitLinkUpTransitionFunction
     *
     * @brief transition function when entering {LinkProberActive, MuxWait, LinkUp} state
     *
     * @param nextState                     reference to composite state
     */
    void LinkProberActiveMuxWaitLinkUpTransitionFunction(CompositeState &nextState);

    /**
     * @method LinkProberUnknownMuxWaitLinkUpTransitionFunction
     *
     * @brief transition function when entering {LinkProberUnknown, MuxWait, LinkUp} state
     *
     * @param nextState                     reference to composite state
     */
    void LinkProberUnknownMuxWaitLinkUpTransitionFunction(CompositeState &nextState);

private: // utility methods to check/modify state
    /**
     * @method setLabel
     *
     * @brief sets linkmgr State db state
     *
     * @param label                         new LinkManagerStateMachine label
     */
    inline void setLabel(Label label) override;

    /**
     * @method enterLinkProberState
     *
     * @brief force LinkProberState to switch state
     *
     * @param nextState                     reference to composite state
     * @param label                         new LinkProberState label to switch to
     */
    inline void enterLinkProberState(CompositeState &nextState, link_prober::LinkProberState::Label label);

    /**
     * @method enterMuxState
     *
     * @brief force MuxState to switch state
     *
     * @param nextState                     reference to composite state
     * @param label                         new MuxState label to switch to
     */
    inline void enterMuxState(CompositeState &nextState, mux_state::MuxState::Label label);

    /**
     * @method enterLinkState
     *
     * @brief force LinkState to switch state
     *
     * @param nextState                     reference to composite state
     * @param label                         new LinkState label to switch to
     */
    inline void enterLinkState(CompositeState &nextState, link_state::LinkState::Label label);

    /**
     * @method enterPeerMuxState
     *
     * @brief force peer MuxState to switch state
     *
     * @param nextState                     reference to composite state
     * @param label                         new peer MuxState label to switch to
     */
    inline void enterPeerMuxState(mux_state::MuxState::Label label);

    /**
     * @method enterPeerLinkProberState
     *
     * @brief force peer LinkProberState to switch state
     *
     * @param label                         new peer LinkProberState to switch to
     */
    inline void enterPeerLinkProberState(link_prober::LinkProberState::Label label);

    /**
     * @method switchMuxState
     *
     * @brief switch MUX to the target state
     *
     * @param nextState                     reference to composite state
     * @param label                         new MuxState label to switch to
     */
    inline void switchMuxState(CompositeState &nextState, mux_state::MuxState::Label label);

    /**
     * @method switchPeerMuxState
     *
     * @brief switch peer MUX to the target state
     *
     * @param label                         new MuxState label to switch to
     */
    inline void switchPeerMuxState(mux_state::MuxState::Label label);

    /**
     * @method probeMuxState
     *
     * @brief probe mux state
     */
    inline void probeMuxState();

    /**
     * @method updateMuxLinkmgrState
     *
     * @brief update State DB MUX LinkMgr state
     */
    void updateMuxLinkmgrState();

    /**
     * @method initLinkProberState
     *
     * @brief initialize LinkProberState when configuring the composite state machine
     *
     * @param compositeState                reference to composite state
     */
    void initLinkProberState(CompositeState &compositeState);

    /**
     * @method initPeerLinkProberState
     *
     * @brief initialize peer LinkProberState when configuring the composite state machine
     */
    void initPeerLinkProberState();

private:
    /**
     * @brief start a mux probe and wait for mux probe notification(active or standby) from xcvrd
     */
    inline void startMuxProbeTimer();

    /**
     * @brief handles when xcvrd has timeout responding mux probe
     *
     * @param errorCode                     error code object
     */
    void handleMuxProbeTimeout(boost::system::error_code errorCode);

    /**
     * @method startMuxWaitTimer
     *
     * @brief start a timer to wait for mux state notification from xcvrd/orchagent
     *
     * @param factor                        factor used to scale the timeout
     */
    inline void startMuxWaitTimer(uint32_t factor = 1);

    /**
     * @method handleMuxWaitTimeout
     *
     * @brief handle when xcrvrd/orchagent has timed out responding mux state
     *
     * @param errorCode                     error code object
     */
    void handleMuxWaitTimeout(boost::system::error_code errorCode);

    /**
     * @method startPeerMuxWaitTimer
     *
     * @brief start a timer to wait for peer mux state notification from xcvrd/orchagent
     *
     * @param factor                        factor used to scale the timeout
     */
    inline void startPeerMuxWaitTimer(uint32_t factor = 1);

    /**
     * @method handlePeerMuxWaitTimeout
     *
     * @brief handle when xcrvrd/orchagent has timed out responding peer mux state
     *
     * @param errorCode                     error code object
     */
    void handlePeerMuxWaitTimeout(boost::system::error_code errorCode);

private: // testing only
    friend class mux::MuxPort;
    friend class test::FakeMuxPort;
    friend class test::MuxManagerTest;

    /**
     * @method setInitializeProberFnPtr
     *
     * @brief set new InitializeProberFnPtr for the state machine. This method is used for testing
     *
     * @param initializeProberFnPtr (in)    pointer to new InitializeProberFnPtr
     */
    void setInitializeProberFnPtr(boost::function<void()> initializeProberFnPtr) { mInitializeProberFnPtr = initializeProberFnPtr; };

    /**
     * @method setStartProbingFnPtr
     *
     * @brief set new StartProbingFnPtr for the state machine. This method is used for testing
     *
     * @param startProbingFnPtr (in)        pointer to new StartProbingFnPtr
     */
    void setStartProbingFnPtr(boost::function<void()> startProbingFnPtr) { mStartProbingFnPtr = startProbingFnPtr; };

    /**
     * @method setUpdateEthernetFrameFnPtr
     *
     * @brief set new UpdateEthernetFrameFnPtr for the state machine. This method is used for testing
     *
     * @param updateEthernetFrameFnPtr (in) pointer to new UpdateEthernetFrameFnPtr
     */
    void setUpdateEthernetFrameFnPtr(boost::function<void()> updateEthernetFrameFnPtr) { mUpdateEthernetFrameFnPtr = updateEthernetFrameFnPtr; };

    /**
     * @method setProbePeerTorFnPtr
     *
     * @brief set new ProbePeerTorFnPtr for the state machine. This method is used for testing
     *
     * @param probePeerTorFnPtr (in)        pointer to new ProbePeerTorFnPtr
     */
    void setProbePeerTorFnPtr(boost::function<void()> probePeerTorFnPtr) { mProbePeerTorFnPtr = probePeerTorFnPtr; };

    /**
     * @method setSuspendTxFnPtr
     *
     * @brief set new SuspendTXFnPtr for the state machine. This method is used for testing
     *
     * @param suspendTxFnPtr (in)           pointer to new  SuspendTxFnPtr
     */
    void setSuspendTxFnPtr(boost::function<void(uint32_t suspendTime_msec)> suspendTxFnPtr) { mSuspendTxFnPtr = suspendTxFnPtr; };

    /**
     * @method setResumeTxFnPtr
     *
     * @brief set new ResumeTXFnPtr for the state machine. This method is used for testing
     *
     * @param resumeTxFnPtr (in)            pointer to new  ResumeTxFnPtr
     */
    void setResumeTxFnPtr(boost::function<void()> resumeTxFnPtr) { mResumeTxFnPtr = resumeTxFnPtr; };

    /**
     * @method setShutdownTxFnPtr
     *
     * @brief set shutdownTxFnPtr. This method is used for testing
     *
     * @param shutdownTxFnPtr (in)          pointer to new ShutdownTxFnPtr
     *
     * @return none
     */
    void setShutdownTxFnPtr(boost::function<void()> shutdownTxFnPtr) { mShutdownTxFnPtr = shutdownTxFnPtr; }

    /**
     * @method setRestartTxFnPtr
     *
     * @brief set restartTxFnPtr. This method is used for testing
     *
     * @param restartTxFnPtr (in)           pointer to new restartTxFnPtr
     *
     * @return none
     */
    void setRestartTxFnPtr(boost::function<void()> restartTxFnPtr) { mRestartTxFnPtr = restartTxFnPtr; }

private: // peer link prober state and mux state
    link_prober::LinkProberState::Label mPeerLinkProberState = link_prober::LinkProberState::Label::PeerWait;
    mux_state::MuxState::Label mPeerMuxState = mux_state::MuxState::Label::Wait;

private:
    uint32_t mMuxProbeBackoffFactor = 1;

    boost::asio::deadline_timer mDeadlineTimer;
    boost::asio::deadline_timer mWaitTimer;
    boost::asio::deadline_timer mPeerWaitTimer;

    boost::function<void()> mInitializeProberFnPtr;
    boost::function<void()> mStartProbingFnPtr;
    boost::function<void()> mUpdateEthernetFrameFnPtr;
    boost::function<void()> mProbePeerTorFnPtr;
    boost::function<void(uint32_t suspendTime_msec)> mSuspendTxFnPtr;
    boost::function<void()> mResumeTxFnPtr;
    boost::function<void()> mShutdownTxFnPtr;
    boost::function<void()> mRestartTxFnPtr;
};

} /* namespace link_manager */

#endif /* LINK_MANAGER_LINKMANAGERSTATEMACHINEACTIVEACTIVE_H_ */
