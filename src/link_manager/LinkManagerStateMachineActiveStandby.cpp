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
 * ActiveStandbyStateMachine.cpp
 *
 *  Created on: Oct 18, 2020
 *      Author: tamer
 */

#include <boost/bind/bind.hpp>

#include "link_manager/LinkManagerStateMachineActiveStandby.h"
#include "common/MuxLogger.h"
#include "common/MuxException.h"
#include "MuxPort.h"

namespace link_manager
{

constexpr auto MAX_BACKOFF_FACTOR = 128;

//
// ---> ActiveStandbyStateMachine(
//          mux::MuxPort *muxPortPtr,
//          boost::asio::io_service::strand &strand,
//          common::MuxPortConfig &muxPortConfig
//      );
//
// class constructor
//
ActiveStandbyStateMachine::ActiveStandbyStateMachine(
    mux::MuxPort *muxPortPtr,
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig
) :
    LinkManagerStateMachineBase(
        muxPortPtr,
        strand,
        muxPortConfig,
        {link_prober::LinkProberState::Label::Unknown, mux_state::MuxState::Label::Wait, link_state::LinkState::Label::Down}
    ),
    mDeadlineTimer(strand.context()),
    mWaitTimer(strand.context())
{
    assert(muxPortPtr != nullptr);
    mMuxStateMachine.setWaitStateCause(mux_state::WaitState::WaitStateCause::SwssUpdate);
    mMuxPortPtr->setMuxLinkmgrState(mLabel);
    initializeTransitionFunctionTable();
}

//
// ---> initializeTransitionFunctionTable()
//
// initialize static transition function table
//
void ActiveStandbyStateMachine::initializeTransitionFunctionTable()
{
    MUXLOGWARNING("Initializing State Transition Table...");
    LinkManagerStateMachineBase::initializeTransitionFunctionTable();

    mStateTransitionHandler[link_prober::LinkProberState::Label::Standby]
                           [mux_state::MuxState::Label::Active]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberStandbyMuxActiveLinkUpTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Active]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberUnknownMuxActiveLinkUpTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Active]
                           [mux_state::MuxState::Label::Standby]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberActiveMuxStandbyLinkUpTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Standby]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberUnknownMuxStandbyLinkUpTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Active]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberActiveMuxUnknownLinkUpTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Standby]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberStandbyMuxUnknownLinkUpTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberUnknownMuxUnknownLinkUpTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Active]
                           [mux_state::MuxState::Label::Error]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberActiveMuxErrorLinkUpTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Standby]
                           [mux_state::MuxState::Label::Error]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberStandbyMuxErrorLinkUpTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Wait]
                           [mux_state::MuxState::Label::Active]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberWaitMuxActiveLinkUpTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Wait]
                           [mux_state::MuxState::Label::Standby]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberWaitMuxStandbyLinkUpTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Wait]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberWaitMuxUnknownLinkUpTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Active]
                           [link_state::LinkState::Label::Down] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberUnknownMuxActiveLinkDownTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Standby]
                           [link_state::LinkState::Label::Down] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberUnknownMuxStandbyLinkDownTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Down] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberUnknownMuxUnknownLinkDownTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Wait]
                           [mux_state::MuxState::Label::Active]
                           [link_state::LinkState::Label::Down] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberWaitMuxActiveLinkDownTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Wait]
                           [mux_state::MuxState::Label::Standby]
                           [link_state::LinkState::Label::Down] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberWaitMuxStandbyLinkDownTransitionFunction,
            this,
            boost::placeholders::_1
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Wait]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Down] =
        boost::bind(
            &ActiveStandbyStateMachine::LinkProberWaitMuxUnknownLinkDownTransitionFunction,
            this,
            boost::placeholders::_1
        );
}

//
// ---> setLabel(Label label);
//
// sets linkmgr State db state
//
void ActiveStandbyStateMachine::setLabel(Label label) {
    if (mLabel != label) {
        mLabel = label;
        mMuxPortPtr->setMuxLinkmgrState(label);

        MUXLOGWARNING(boost::format("%s: Linkmgrd state is: %s %s") %
            mMuxPortConfig.getPortName() %
            mMuxStateName[ms(mCompositeState)] %
            mLinkHealthName[static_cast<int> (label)]
        );
    }
};

//
// ---> enterLinkProberState(CompositeState &nextState, link_prober::LinkProberState::Label label, bool forceReset);
//
// force LinkProberState to switch state
//
void ActiveStandbyStateMachine::enterLinkProberState(CompositeState &nextState, link_prober::LinkProberState::Label label, bool forceReset)
{
    mLinkProberStateMachinePtr->enterState(label);

    if (forceReset && ps(nextState) == label) {
        // only need to reset the link prober state if the state remains the same
        mLinkProberStateMachinePtr->resetCurrentState();
    }

    ps(nextState) = label;

    // link prober entering wait indicating switchover is initiated, but a switchover can be skipped if mode == manual.
    if(label == link_prober::LinkProberState::Label::Wait) {
        mMuxPortPtr->postLinkProberMetricsEvent(link_manager::ActiveStandbyStateMachine::LinkProberMetrics::LinkProberWaitStart);
    }
}

//
// ---> enterMuxState(CompositeState &nextState, mux_state::MuxState::Label label);
//
// force MuxState to switch state
//
void ActiveStandbyStateMachine::enterMuxState(CompositeState &nextState, mux_state::MuxState::Label label)
{
    mMuxStateMachine.enterState(label);
    ms(nextState) = label;
}

//
// ---> enterLinkState(CompositeState &nextState, link_state::LinkState::Label label);
//
// force LinkState to switch state
//
void ActiveStandbyStateMachine::enterLinkState(CompositeState &nextState, link_state::LinkState::Label label)
{
    mLinkStateMachine.enterState(label);
    ls(nextState) = label;
}

//
// ---> enterMuxWaitState(CompositeState &nextState);
//
// force MuxState to switch to WaitState
//
void ActiveStandbyStateMachine::enterMuxWaitState(CompositeState &nextState)
{
    enterMuxState(nextState, mux_state::MuxState::Label::Wait);
    mMuxStateMachine.setWaitStateCause(mux_state::WaitState::WaitStateCause::DriverUpdate);
    mMuxPortPtr->probeMuxState();
    startMuxWaitTimer(MAX_BACKOFF_FACTOR);
}

//
// ---> switchMuxState(CompositeState &nextState, mux_state::MuxState::Label label, bool forceSwitch);
//
// switch Mux to switch via xcvrd to state label provider
//
void ActiveStandbyStateMachine::switchMuxState(
    link_manager::ActiveStandbyStateMachine::SwitchCause cause,
    CompositeState &nextState,
    mux_state::MuxState::Label label,
    bool forceSwitch
)
{
    if (forceSwitch || mMuxPortConfig.getMode() == common::MuxPortConfig::Mode::Auto) {
        MUXLOGWARNING(boost::format("%s: Switching MUX state to '%s'") %
            mMuxPortConfig.getPortName() %
            mMuxStateName[label]
        );

        // mWaitActiveUpCount is introduced to address asymmetric link drop issue. 
        // Resetting the count to avoid unnecessary HB suspends. 
        if(label == mux_state::MuxState::Label::Active) {
            mWaitActiveUpCount = 0;
        }

        enterMuxState(nextState, mux_state::MuxState::Label::Wait);
        mMuxStateMachine.setWaitStateCause(mux_state::WaitState::WaitStateCause::SwssUpdate);
        mMuxPortPtr->postMetricsEvent(Metrics::SwitchingStart, label);
        mMuxPortPtr->postSwitchCause(cause);
        mMuxPortPtr->setMuxState(label);
        if(mMuxPortConfig.ifEnableSwitchoverMeasurement()) {
            mDecreaseIntervalFnPtr(mMuxPortConfig.getLinkWaitTimeout_msec()); 
        }
        mDeadlineTimer.cancel();
        startMuxWaitTimer(MAX_BACKOFF_FACTOR);
    } else {
        enterMuxWaitState(nextState);
    }
}

//
// ---> handleSwssBladeIpv4AddressUpdate(boost::asio::ip::address address);
//
// initialize LinkProber component. Note if this is the last component to be initialized,
// state machine will be activated
//
void ActiveStandbyStateMachine::handleSwssBladeIpv4AddressUpdate(boost::asio::ip::address address)
{
    if (!mComponentInitState.test(LinkProberComponent)) {
        mMuxPortConfig.setBladeIpv4Address(address);

        try {
            mLinkProberPtr = std::make_shared<link_prober::LinkProber> (
                mMuxPortConfig,
                getStrand().context(),
                mLinkProberStateMachinePtr.get()
            );
            mInitializeProberFnPtr = boost::bind(
                &link_prober::LinkProber::initialize, mLinkProberPtr.get()
            );
            mStartProbingFnPtr = boost::bind(
                &link_prober::LinkProber::startProbing, mLinkProberPtr.get()
            );
            mUpdateEthernetFrameFnPtr = boost::bind(
                &link_prober::LinkProber::updateEthernetFrame, mLinkProberPtr.get()
            );
            mProbePeerTorFnPtr = boost::bind(
                &link_prober::LinkProber::probePeerTor, mLinkProberPtr.get()
            );
            mSuspendTxFnPtr = boost::bind(
                &link_prober::LinkProber::suspendTxProbes, mLinkProberPtr.get(), boost::placeholders::_1
            );
            mResumeTxFnPtr = boost::bind(
                &link_prober::LinkProber::resumeTxProbes, mLinkProberPtr.get()
            );
            mSendPeerSwitchCommandFnPtr = boost::bind(
                &link_prober::LinkProber::sendPeerSwitchCommand, mLinkProberPtr.get()
            );
            mResetIcmpPacketCountsFnPtr = boost::bind(
                &link_prober::LinkProber::resetIcmpPacketCounts, mLinkProberPtr.get()
            );
            mShutdownTxFnPtr = boost::bind(
                &link_prober::LinkProber::shutdownTxProbes, mLinkProberPtr.get()
            );
            mRestartTxFnPtr = boost::bind(
                &link_prober::LinkProber::restartTxProbes, mLinkProberPtr.get()
            );
            mDecreaseIntervalFnPtr = boost::bind(
                &link_prober::LinkProber::decreaseProbeIntervalAfterSwitch, mLinkProberPtr.get(), boost::placeholders::_1
            );
            mRevertIntervalFnPtr = boost::bind(
                &link_prober::LinkProber::revertProbeIntervalAfterSwitchComplete, mLinkProberPtr.get()
            );
            mComponentInitState.set(LinkProberComponent);

            activateStateMachine();
        }
        catch (const std::bad_alloc &ex) {
            std::ostringstream errMsg;
            errMsg << "Failed allocate memory. Exception details: " << ex.what();

            throw MUX_ERROR(BadAlloc, errMsg.str());
        }
    } else if (address != mMuxPortConfig.getBladeIpv4Address()){
        mMuxPortConfig.setBladeIpv4Address(address);

        mUpdateEthernetFrameFnPtr();
    }
}

//
// ---> activateStateMachine();
//
// activate the state machine by starting the LinkProber. This should be done after all
// components have been initialized.
//
void ActiveStandbyStateMachine::activateStateMachine()
{
    if (mComponentInitState.all()) {
        std::array<uint8_t, ETHER_ADDR_LEN> macAddress = mMuxPortConfig.getBladeMacAddress();
        std::array<char, 3 * ETHER_ADDR_LEN> macAddressStr = {0};

        snprintf(
            macAddressStr.data(), macAddressStr.size(), "%02x:%02x:%02x:%02x:%02x:%02x",
            macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]
        );

        MUXLOGWARNING(boost::format("%s: MUX port link prober initialized with server IP: %s, server MAC: %s") %
            mMuxPortConfig.getPortName() %
            mMuxPortConfig.getBladeIpv4Address().to_string() %
            macAddressStr.data()
        );
        // make link prober state match the MUX state since the state machine is activated for the first time
        CompositeState nextState = mCompositeState;
        initLinkProberState(nextState);
        LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;

        mInitializeProberFnPtr();
        mStartProbingFnPtr();

        updateMuxLinkmgrState();
    }

    if (mComponentInitState.test(LinkProberComponent) && mComponentInitState.test(MuxStateComponent)) {
        mMuxPortPtr->warmRestartReconciliation();
    }
}

//
// ---> handleStateChange(LinkProberEvent &event, link_prober::LinkProberState::Label state);
//
// handles LinkProberEvent
//
void ActiveStandbyStateMachine::handleStateChange(LinkProberEvent &event, link_prober::LinkProberState::Label state)
{
    if ((dynamic_cast<link_prober::LinkProberState *> (mLinkProberStateMachinePtr->getCurrentState()))->getStateLabel() == state) {
        MUXLOGWARNING(boost::format("%s: Received link prober event, new state: %s") %
            mMuxPortConfig.getPortName() %
            mLinkProberStateName[state]
        );

        // update state db link prober metrics to collect link prober state change data
        if (mContinuousLinkProberUnknownEvent == true && state != link_prober::LinkProberState::Unknown) {
            mContinuousLinkProberUnknownEvent = false;
        } 
        
        if (mContinuousLinkProberUnknownEvent == false && state == link_prober::LinkProberState::Label::Unknown) {
            mContinuousLinkProberUnknownEvent = true;
            mMuxPortPtr->postLinkProberMetricsEvent(link_manager::ActiveStandbyStateMachine::LinkProberMetrics::LinkProberUnknownStart);
        } 
        
        if (state == link_prober::LinkProberState::Label::Active) {
            mMuxPortPtr->postLinkProberMetricsEvent(link_manager::ActiveStandbyStateMachine::LinkProberMetrics::LinkProberActiveStart);

            mStandbyUnknownUpCount = 0;
        }
         
        if (state == link_prober::LinkProberState::Label::Standby) {
            mMuxPortPtr->postLinkProberMetricsEvent(link_manager::ActiveStandbyStateMachine::LinkProberMetrics::LinkProberStandbyStart);

            mActiveUnknownUpCount = 0;
        }

        CompositeState nextState = mCompositeState;
        ps(nextState) = state;
        mStateTransitionHandler[ps(nextState)][ms(nextState)][ls(nextState)](nextState);
        LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
    }

    if (ps(mCompositeState) != link_prober::LinkProberState::Unknown) {
        mResumeTxFnPtr();
    }

    updateMuxLinkmgrState();
}

//
// ---> handleStateChange(MuxStateEvent &event, mux_state::MuxState::Label state);
//
// handles MuxStateEvent
//
void ActiveStandbyStateMachine::handleStateChange(MuxStateEvent &event, mux_state::MuxState::Label state)
{
    if ((dynamic_cast<mux_state::MuxState *> (mMuxStateMachine.getCurrentState()))->getStateLabel() == state) {
        MUXLOGINFO(boost::format("%s: Received mux state event, new state: %s") %
            mMuxPortConfig.getPortName() %
            mMuxStateName[state]
        );

        CompositeState nextState = mCompositeState;
        ms(nextState) = state;
        mStateTransitionHandler[ps(nextState)][ms(nextState)][ls(nextState)](nextState);
        LOGINFO_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
    }

    if (ms(mCompositeState) != mux_state::MuxState::Wait) {
        // Verify if state db MUX state matches the driver/current MUX state
        mMuxPortPtr->getMuxState();

        // Handle pending mux mode config change
        if (mPendingMuxModeChange) {
            MUXLOGINFO(boost::format("%s: Mux state: %s . Execute pending MUX mode config change.") % 
                mMuxPortConfig.getPortName() %
                mMuxStateName[ms(mCompositeState)]
            );

            handleMuxConfigNotification(mTargetMuxMode);
            mPendingMuxModeChange = false;
        }
    }

    if (state != mux_state::MuxState::Label::Unknown) {
        mMuxUnknownBackoffFactor = 1;

        mActiveUnknownUpCount = 0;
        mStandbyUnknownUpCount = 0;
    }

    updateMuxLinkmgrState();
}

//
// ---> handleStateChange(LinkStateEvent &event, link_state::LinkState::Label state);
//
// handles LinkStateEvent
//
void ActiveStandbyStateMachine::handleStateChange(LinkStateEvent &event, link_state::LinkState::Label state)
{
    if ((dynamic_cast<link_state::LinkState *> (mLinkStateMachine.getCurrentState()))->getStateLabel() == state) {
        MUXLOGWARNING(boost::format("%s: Received link state event, new state: %s") %
            mMuxPortConfig.getPortName() %
            mLinkStateName[state]
        );

        CompositeState nextState = mCompositeState;
        ls(nextState) = state;
        if (ls(mCompositeState) == link_state::LinkState::Down &&
            ls(nextState) == link_state::LinkState::Up) {
            // start fresh when the link transition from Down to UP state
            // and so the link prober will initially match the MUX state
            // There is a problem with this approach as it will hide link flaps that result in lost heart-beats.
            initLinkProberState(nextState, true);
//            enterMuxWaitState(nextState);
        } else if (ls(mCompositeState) == link_state::LinkState::Up &&
                   ls(nextState) == link_state::LinkState::Down &&
                   ms(mCompositeState) != mux_state::MuxState::Label::Standby) {
            // switch MUX to standby since we are entering LinkDown state
            switchMuxState(link_manager::ActiveStandbyStateMachine::SwitchCause::LinkDown, nextState, mux_state::MuxState::Label::Standby);

            mActiveUnknownUpCount = 0;
            mStandbyUnknownUpCount = 0;
        } else {
            mStateTransitionHandler[ps(nextState)][ms(nextState)][ls(nextState)](nextState);
        }
        LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
    }

    updateMuxLinkmgrState();
}

//
// ---> handleGetServerMacAddressNotification(std::array<uint8_t, ETHER_ADDR_LEN> address);
//
// handle get Server MAC address
//
void ActiveStandbyStateMachine::handleGetServerMacAddressNotification(std::array<uint8_t, ETHER_ADDR_LEN> address)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    if (address != mMuxPortConfig.getBladeMacAddress()) {
        mMuxPortConfig.setBladeMacAddress(address);
        if (mUpdateEthernetFrameFnPtr) {
            mUpdateEthernetFrameFnPtr();
        } else if (mComponentInitState.test(LinkProberComponent)) {
            std::array<char, 3 * ETHER_ADDR_LEN> addressStr = {0};
            snprintf(
                addressStr.data(), addressStr.size(), "%02x:%02x:%02x:%02x:%02x:%02x",
                address[0], address[1], address[2], address[3], address[4], address[5]
            );

            MUXLOGERROR(boost::format("%s: failed to update Ethernet frame with mac '%s', link prober init state: %d") %
                mMuxPortConfig.getPortName() %
                addressStr.data() %
                mComponentInitState.test(LinkProberComponent)
            );
        }
    }
}

//
// ---> handleGetMuxStateNotification(mux_state::MuxState::Label label);
//
// handle get MUX state notification
//
void ActiveStandbyStateMachine::handleGetMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGINFO(boost::format("%s: state db mux state: %s") % mMuxPortConfig.getPortName() % mMuxStateName[label]);

    if (mComponentInitState.all() && ms(mCompositeState) != label &&
        ms(mCompositeState) != mux_state::MuxState::Wait &&
        ms(mCompositeState) != mux_state::MuxState::Error &&
        ms(mCompositeState) != mux_state::MuxState::Unknown) {
        // notify swss of mux state change
        MUXLOGWARNING(boost::format("%s: Switching MUX state from '%s' to '%s' to match linkmgrd/xcvrd state") %
            mMuxPortConfig.getPortName() %
            mMuxStateName[label] %
            mMuxStateName[ms(mCompositeState)]
        );
        switchMuxState(link_manager::ActiveStandbyStateMachine::SwitchCause::MatchingHardwareState, mCompositeState, ms(mCompositeState), true);
    }
}

//
// ---> handleProbeMuxStateNotification(mux_state::MuxState::Label label);
//
// handle MUX state notification. Source of notification could be app_db via xcvrd
//
void ActiveStandbyStateMachine::handleProbeMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGINFO(boost::format("%s: app db mux state: %s") % mMuxPortConfig.getPortName() % mMuxStateName[label]);

    mWaitTimer.cancel();

    if (mComponentInitState.all()) {
        if (mMuxStateMachine.getWaitStateCause() != mux_state::WaitState::WaitStateCause::DriverUpdate) {
            MUXLOGWARNING(boost::format("%s: Received unsolicited MUX state probe notification!") %
                mMuxPortConfig.getPortName()
            );
        }

        postMuxStateEvent(label);
    } else if (label != mux_state::MuxState::Unknown) {
        MUXLOGWARNING(boost::format("%s: Initializing MUX state '%s' to match xcvrd state") %
            mMuxPortConfig.getPortName() %
            mMuxStateName[label]
        );

        // mux state was probed during initialization, update the mux state
        enterMuxState(mCompositeState, mux_state::MuxState::Label::Wait);
        mMuxStateMachine.setWaitStateCause(mux_state::WaitState::WaitStateCause::SwssUpdate);
        mMuxPortPtr->setMuxState(label);
    } else {
        MUXLOGWARNING(boost::format("%s: xcvrd reports MUX state as '%s' during init. phase! Is there a functioning MUX?") %
            mMuxPortConfig.getPortName() %
            mMuxStateName[label]
        );

        enterMuxState(mCompositeState, label);
        mComponentInitState.set(MuxStateComponent);
        activateStateMachine();
    }
}

//
// ---> handleMuxStateNotification(mux_state::MuxState::Label label);
//
// handle MUX state notification
//
void ActiveStandbyStateMachine::handleMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGWARNING(boost::format("%s: state db mux state: %s") % mMuxPortConfig.getPortName() % mMuxStateName[label]);

    mWaitTimer.cancel();

    if (mComponentInitState.all()) {
        if (mMuxStateMachine.getWaitStateCause() != mux_state::WaitState::WaitStateCause::SwssUpdate ||
            ms(mCompositeState) != mux_state::MuxState::Wait) {
            MUXLOGWARNING(boost::format("%s: Received unsolicited MUX state change notification!") %
                mMuxPortConfig.getPortName()
            );
        }
        mProbePeerTorFnPtr();
        postMuxStateEvent(label);
        if (mMuxStateMachine.testWaitStateCause(mux_state::WaitState::WaitStateCause::SwssUpdate)) {
            mMuxPortPtr->postMetricsEvent(Metrics::SwitchingEnd, label);
            mMuxStateMachine.resetWaitStateCause(mux_state::WaitState::WaitStateCause::SwssUpdate);
        }
    } else if (label == mux_state::MuxState::Unknown) {
        // state db may not have been initialized with up-to-date mux state
        // probe xcvrd to read the current mux state
        enterMuxWaitState(mCompositeState);
    } else {
        enterMuxState(mCompositeState, label);

        mComponentInitState.set(MuxStateComponent);
        activateStateMachine();
    }
}

//
// ---> handleSwssLinkStateNotification(const link_state::LinkState::Label label);
//
// handle link state change notification
//
void ActiveStandbyStateMachine::handleSwssLinkStateNotification(const link_state::LinkState::Label label)
{
    MUXLOGINFO(boost::format("%s: state db link state: %s") % mMuxPortConfig.getPortName() % mLinkStateName[label]);

    if (mComponentInitState.all()) {
        if (label == link_state::LinkState::Label::Up) {
            mLinkStateMachine.postLinkStateEvent(link_state::LinkStateMachine::getUpEvent());
        } else if (label == link_state::LinkState::Label::Down) {
            mLinkStateMachine.postLinkStateEvent(link_state::LinkStateMachine::getDownEvent());
        }
    } else {
        enterLinkState(mCompositeState, label);

        mComponentInitState.set(LinkStateComponent);
        activateStateMachine();
    }
}

// ---> handlePeerLinkStateNotification(const link_state::LinkState::Label label);
// 
// handle peer link state change notification 
//
void ActiveStandbyStateMachine::handlePeerLinkStateNotification(const link_state::LinkState::Label label)
{
    MUXLOGINFO(boost::format("%s: state db peer link state: %s") % mMuxPortConfig.getPortName() % mLinkStateName[label]);

    mPeerLinkState = label;
    if(label == link_state::LinkState::Label::Down && 
        ls(mCompositeState) == link_state::LinkState::Label::Up &&
        ms(mCompositeState) == mux_state::MuxState::Standby) {
        CompositeState nextState = mCompositeState;
        enterLinkProberState(nextState, link_prober::LinkProberState::Wait);
        if (mDefaultRouteState == DefaultRoute::OK) {
            switchMuxState(link_manager::ActiveStandbyStateMachine::SwitchCause::PeerLinkDown, nextState, mux_state::MuxState::Label::Active);
        }
        LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
    }
}

//
// ---> handleMuxConfigNotification(const common::MuxPortConfig::Mode mode);
//
// handle MUX configuration change notification
//
void ActiveStandbyStateMachine::handleMuxConfigNotification(const common::MuxPortConfig::Mode mode)
{
    if (mComponentInitState.test(MuxStateComponent) &&
        mode != common::MuxPortConfig::Mode::Auto && 
        mode != common::MuxPortConfig::Mode::Manual &&
        ms(mCompositeState) == mux_state::MuxState::Wait) {
        
        MUXLOGINFO(boost::format("%s: Mux state: %s , mux mode config change is pending. ") % 
                mMuxPortConfig.getPortName() %
                mMuxStateName[ms(mCompositeState)]
            );

        mPendingMuxModeChange = true;
        mTargetMuxMode = mode;

        return;
    }

    if (mComponentInitState.all()) {
        if (mode == common::MuxPortConfig::Mode::Active &&
                ms(mCompositeState) != mux_state::MuxState::Label::Active) {
            CompositeState nextState = mCompositeState;
            enterLinkProberState(nextState, link_prober::LinkProberState::Wait);
            switchMuxState(link_manager::ActiveStandbyStateMachine::SwitchCause::ConfigMuxMode, nextState, mux_state::MuxState::Label::Active);
            LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
            mCompositeState = nextState;
        } else if(mode == common::MuxPortConfig::Mode::Standby &&
                    ms(mCompositeState) != mux_state::MuxState::Label::Standby) {
            mSendSwitchActiveCommandCause = link_manager::ActiveStandbyStateMachine::SwitchCause::ConfigMuxMode;
            mSendPeerSwitchCommandFnPtr();
        } else {
            LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, mCompositeState);
            if (ls(mCompositeState) == link_state::LinkState::Down &&
                ms(mCompositeState) != mux_state::MuxState::Label::Standby) {
                CompositeState nextState = mCompositeState;
                switchMuxState(link_manager::ActiveStandbyStateMachine::SwitchCause::LinkDown, nextState, mux_state::MuxState::Label::Standby, true);
                LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
                mCompositeState = nextState;
            } else {
                startMuxProbeTimer();
            }
        }

        updateMuxLinkmgrState();
    }

    mMuxPortConfig.setMode(mode);
    shutdownOrRestartLinkProberOnDefaultRoute();
}

//
// ---> handleSuspendTimerExpiry();
//
// handle suspend timer expiry notification from LinkProber
//
void ActiveStandbyStateMachine::handleSuspendTimerExpiry()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());
    // Note: suspend timer is started when Mux state is active and link is in unknown state.
    //       If standby (peer) ToR fails to pull the link, this ToR will loop between those states
    //       Prober: Unknown, Mux: Active, Link: Up --Suspend Timer Expires--> Wait, Wait, UP -> (probe)
    //               Unknown, Active, Up or Unknown, Standby, UP or Unknown, Unknown, UP
    if (ps(mCompositeState) == link_prober::LinkProberState::Label::Unknown &&
        ms(mCompositeState) == mux_state::MuxState::Label::Active &&
        ls(mCompositeState) == link_state::LinkState::Label::Up) {
        CompositeState currState = mCompositeState;
        enterMuxWaitState(mCompositeState);
        LOGINFO_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), currState, mCompositeState);
    } else {
        mUnknownActiveUpBackoffFactor = 1;
    }
}

//
// ---> handleSwitchActiveCommandCompletion();
//
// handle completion of sending switch command to peer ToR
//
void ActiveStandbyStateMachine::handleSwitchActiveCommandCompletion()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    if (ms(mCompositeState) != mux_state::MuxState::Label::Standby) {
        CompositeState nextState = mCompositeState;
        enterLinkProberState(nextState, link_prober::LinkProberState::Wait);
        switchMuxState(mSendSwitchActiveCommandCause, nextState, mux_state::MuxState::Label::Standby, true);
        LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
    }
}

//
// ---> handleSwitchActiveRequestEvent();
//
// handle switch active request from peer ToR
//
void ActiveStandbyStateMachine::handleSwitchActiveRequestEvent()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    if (ms(mCompositeState) != mux_state::MuxState::Label::Active &&
        ms(mCompositeState) != mux_state::MuxState::Label::Wait) {
        CompositeState nextState = mCompositeState;
        enterLinkProberState(nextState, link_prober::LinkProberState::Wait);
        switchMuxState(link_manager::ActiveStandbyStateMachine::SwitchCause::TlvSwitchActiveCommand, nextState, mux_state::MuxState::Label::Active);
        LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
    }
}

// 
// ---> handleDefaultRouteStateNotification(const DefaultRoute routeState);
// 
// handle default route state notification from routeorch
//
void ActiveStandbyStateMachine::handleDefaultRouteStateNotification(const DefaultRoute routeState)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    if (mDefaultRouteState == DefaultRoute::NA && routeState == DefaultRoute::OK) {
        initLinkProberState(mCompositeState);
    }
    mDefaultRouteState = routeState;
    shutdownOrRestartLinkProberOnDefaultRoute();

    updateMuxLinkmgrState();
}

//
//
// ---> shutdownOrRestartLinkProberOnDefaultRoute();
//
// shutdown or restart link prober based on default route state
//
void ActiveStandbyStateMachine::shutdownOrRestartLinkProberOnDefaultRoute()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    if (mComponentInitState.all()) {
        if (mMuxPortConfig.getMode() == common::MuxPortConfig::Mode::Auto && mDefaultRouteState == DefaultRoute::NA) {
            mShutdownTxFnPtr();
        } else {
            // If mux mode is in manual/standby/active mode, we should restart link prober. 
            // If default route state is "ok", we should retart link prober.
            mRestartTxFnPtr();
        }
    }
}

//
// ---> handlePostPckLossRatioNotification(const uint64_t unknownEventCount, const uint64_t expectedPacketCount);
// 
// handle post pck loss ratio 
//
void ActiveStandbyStateMachine::handlePostPckLossRatioNotification(const uint64_t unknownEventCount, const uint64_t expectedPacketCount)
{
    MUXLOGDEBUG(boost::format("%s: posting pck loss ratio, pck_loss_count / pck_expected_count : %d / %d") %
        mMuxPortConfig.getPortName() %
        unknownEventCount % 
        expectedPacketCount
    );
    
    mMuxPortPtr->postPckLossRatio(unknownEventCount, expectedPacketCount);
}

// ---> handleResetLinkProberPckLossCount();
// 
// reset link prober heartbeat packet loss count 
// 
void ActiveStandbyStateMachine::handleResetLinkProberPckLossCount()
{
    MUXLOGDEBUG(boost::format("%s: reset link prober packet loss counts ") % mMuxPortConfig.getPortName());

    mResetIcmpPacketCountsFnPtr();
}

//
// ---> updateMuxLinkmgrState();
//
// Update State DB MUX LinkMgr state
//
void ActiveStandbyStateMachine::updateMuxLinkmgrState()
{
    Label label = Label::Unhealthy;
    if (ls(mCompositeState) == link_state::LinkState::Label::Up &&
        ((ps(mCompositeState) == link_prober::LinkProberState::Label::Active &&
         ms(mCompositeState) == mux_state::MuxState::Label::Active) ||
        (ps(mCompositeState) == link_prober::LinkProberState::Label::Standby &&
         ms(mCompositeState) == mux_state::MuxState::Label::Standby)) &&
        (mMuxPortConfig.ifEnableDefaultRouteFeature() == false || mDefaultRouteState == DefaultRoute::OK)) {
        label = Label::Healthy;
        mRevertIntervalFnPtr();
    }

    setLabel(label);
}

//
// ---> startMuxProbeTimer(uint32_t factor);
//
// start a timer to monitor the MUX state
//
void ActiveStandbyStateMachine::startMuxProbeTimer(uint32_t factor)
{
    mDeadlineTimer.expires_from_now(boost::posix_time::milliseconds(
        factor * mMuxPortConfig.getNegativeStateChangeRetryCount() * mMuxPortConfig.getTimeoutIpv4_msec()
    ));
    mDeadlineTimer.async_wait(getStrand().wrap(boost::bind(
        &ActiveStandbyStateMachine::handleMuxProbeTimeout,
        this,
        boost::asio::placeholders::error
    )));
}

//
// ---> handleMuxProbeTimeout(boost::system::error_code errorCode);
//
// handle when LinkProber heartbeats were lost due link down, bad cable, server down or state mismatching
//
void ActiveStandbyStateMachine::handleMuxProbeTimeout(boost::system::error_code errorCode)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    if (!(ps(mCompositeState) == link_prober::LinkProberState::Label::Wait &&
         ms(mCompositeState) == mux_state::MuxState::Label::Standby &&
         ls(mCompositeState) == link_state::LinkState::Label::Up)) {
             mWaitStandbyUpBackoffFactor = 1;
    }

    if (errorCode == boost::system::errc::success &&
        (ps(mCompositeState) == link_prober::LinkProberState::Label::Wait ||
         ms(mCompositeState) == mux_state::MuxState::Label::Unknown ||
         ls(mCompositeState) == link_state::LinkState::Label::Down ||
         (ps(mCompositeState) == link_prober::LinkProberState::Label::Standby &&
         ms(mCompositeState) == mux_state::MuxState::Label::Active) ||
        (ps(mCompositeState) == link_prober::LinkProberState::Label::Active &&
         ms(mCompositeState) == mux_state::MuxState::Label::Standby))
    ) {
        CompositeState currState = mCompositeState;
        enterMuxWaitState(mCompositeState);
        LOGINFO_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), currState, mCompositeState);
    }
}

//
// ---> startMuxWaitTimer(uint32_t factor);
//
// start a timer to monitor the MUX state
//
void ActiveStandbyStateMachine::startMuxWaitTimer(uint32_t factor)
{
    mWaitTimer.expires_from_now(boost::posix_time::milliseconds(
        factor * mMuxPortConfig.getNegativeStateChangeRetryCount() * mMuxPortConfig.getTimeoutIpv4_msec()
    ));
    mWaitTimer.async_wait(getStrand().wrap(boost::bind(
        &ActiveStandbyStateMachine::handleMuxWaitTimeout,
        this,
        boost::asio::placeholders::error
    )));
}

//
// ---> handleMuxWaitTimeout(boost::system::error_code errorCode);
//
// handle when xcrvrd/orchagent has timed out responding to linkmgrd.
//
void ActiveStandbyStateMachine::handleMuxWaitTimeout(boost::system::error_code errorCode)
{
    if (errorCode == boost::system::errc::success) {
        if (mMuxStateMachine.getWaitStateCause() == mux_state::WaitState::WaitStateCause::SwssUpdate) {
            MUXLOGTIMEOUT(mMuxPortConfig.getPortName(), "orchagent timed out responding to linkmgrd", mCompositeState);
        } else if (mMuxStateMachine.getWaitStateCause() == mux_state::WaitState::WaitStateCause::DriverUpdate) {
            MUXLOGTIMEOUT(mMuxPortConfig.getPortName(), "xcvrd timed out responding to linkmgrd", mCompositeState);
            // send switch active command to peer
            mSendSwitchActiveCommandCause = link_manager::ActiveStandbyStateMachine::SwitchCause::TransceiverDaemonTimeout;
            mSendPeerSwitchCommandFnPtr();
        } else {
            MUXLOGTIMEOUT(mMuxPortConfig.getPortName(), "Unknown timeout reason!!!", mCompositeState);
        }
    }
}

//
// ---> initLinkProberState(CompositeState &compositeState, bool forceReset);
//
// initialize LinkProberState when configuring the composite state machine
//
void ActiveStandbyStateMachine::initLinkProberState(CompositeState &compositeState, bool forceReset)
{
    switch (ms(compositeState)) {
    case mux_state::MuxState::Label::Active:
        enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Active, forceReset);
        break;
    case mux_state::MuxState::Label::Standby:
        enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Standby, forceReset);
        break;
    case mux_state::MuxState::Label::Unknown:
        enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Unknown, forceReset);
        break;
    case mux_state::MuxState::Label::Error:
        enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Wait, forceReset);
        break;
    case mux_state::MuxState::Label::Wait:
        enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Wait, forceReset);
        break;
    default:
        break;
    }
}

//
// ---> LinkProberStandbyMuxActiveLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberStandby, MuxActive, LinkUp} state
//
void ActiveStandbyStateMachine::LinkProberStandbyMuxActiveLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    if ((ps(mCompositeState) != ps(nextState)) &&
        (ps(nextState) == link_prober::LinkProberState::Label::Active ||
         ps(nextState) == link_prober::LinkProberState::Label::Standby)) {
        // If entering by link prober state change, probe mux state immediately. 
        enterMuxWaitState(nextState);
    } else {
        // There can be a delay for hardware state change in switchovers.
        // So if not entering by link prober state change, start a timer for mux state probing. 
        // When timer expires and if link prober & mux state remain inconsistent, do mux state probing. 
        startMuxProbeTimer();
    }
}

//
// ---> LinkProberUnknownMuxActiveLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxActive, LinkUp} state
//
void ActiveStandbyStateMachine::LinkProberUnknownMuxActiveLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    // Suspend TX probes to help peer ToR takes over in case active link is bad
    mSuspendTxFnPtr(mMuxPortConfig.getLinkWaitTimeout_msec()*mUnknownActiveUpBackoffFactor);
    mUnknownActiveUpBackoffFactor <<= 1;
    mUnknownActiveUpBackoffFactor = mUnknownActiveUpBackoffFactor > MAX_BACKOFF_FACTOR ? MAX_BACKOFF_FACTOR : mUnknownActiveUpBackoffFactor;
    mWaitActiveUpCount = 0;
}

//
// ---> LinkProberActiveMuxStandbyLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberActive, MuxStandby, LinkUp} state
//
void ActiveStandbyStateMachine::LinkProberActiveMuxStandbyLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    if ((ps(mCompositeState) != ps(nextState)) &&
        (ps(nextState) == link_prober::LinkProberState::Label::Active ||
         ps(nextState) == link_prober::LinkProberState::Label::Standby)) {
        // If entering by link prober state change, probe mux state immediately.     
        enterMuxWaitState(nextState);
    } else {
        // There can be a delay for hardware state change in switchovers.
        // So if not entering by link prober state change, start a timer for mux state probing. 
        // When timer expires and if link prober & mux state remain inconsistent, do mux state probing. 
        startMuxProbeTimer();
    }
}

//
// ---> LinkProberUnknownMuxStandbyLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxStandby, LinkUp} state
//
void ActiveStandbyStateMachine::LinkProberUnknownMuxStandbyLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    enterLinkProberState(nextState, link_prober::LinkProberState::Wait);

    // Start switching MUX to active state as we lost HB from active ToR
    if (mDefaultRouteState == DefaultRoute::OK) {
        switchMuxState(link_manager::ActiveStandbyStateMachine::SwitchCause::PeerHeartbeatMissing, nextState, mux_state::MuxState::Label::Active);
        mDeadlineTimer.cancel();
    } else {
        enterMuxWaitState(nextState);
    }
    mWaitActiveUpCount = 0;
}

//
// ---> LinkProberActiveMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberActive, MuxUnknown, LinkUp} state
//
void ActiveStandbyStateMachine::LinkProberActiveMuxUnknownLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    mActiveUnknownUpCount++;

    if (mActiveUnknownUpCount == mMuxPortConfig.getNegativeStateChangeRetryCount()) {
        switchMuxState(link_manager::ActiveStandbyStateMachine::SwitchCause::HarewareStateUnknown, nextState, mux_state::MuxState::Label::Active);
    } else {
        enterMuxWaitState(nextState);
    }
}

//
// ---> LinkProberStandbyMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberStandby, MuxUnknown, LinkUp} state
//
void ActiveStandbyStateMachine::LinkProberStandbyMuxUnknownLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    mStandbyUnknownUpCount++;

    if ((ps(mCompositeState) != ps(nextState)) &&
        (ps(nextState) == link_prober::LinkProberState::Label::Active ||
         ps(nextState) == link_prober::LinkProberState::Label::Standby)) {
        enterMuxWaitState(nextState);
    } else if (mStandbyUnknownUpCount == mMuxPortConfig.getNegativeStateChangeRetryCount()) {
        switchMuxState(link_manager::ActiveStandbyStateMachine::SwitchCause::HarewareStateUnknown, nextState, mux_state::MuxState::Label::Standby);
    } else {
        startMuxProbeTimer();
    }
}

//
// ---> LinkProberUnknownMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxUnknown, LinkUp} state
//
void ActiveStandbyStateMachine::LinkProberUnknownMuxUnknownLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer(mMuxUnknownBackoffFactor);
    mMuxUnknownBackoffFactor <<= 1;
    mMuxUnknownBackoffFactor = mMuxUnknownBackoffFactor > MAX_BACKOFF_FACTOR ? MAX_BACKOFF_FACTOR : mMuxUnknownBackoffFactor;
}

//
// ---> LinkProberActiveMuxErrorLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberActive, MuxError, LinkUp} state
//
void ActiveStandbyStateMachine::LinkProberActiveMuxErrorLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    if ((ps(mCompositeState) != ps(nextState)) &&
        (ps(nextState) == link_prober::LinkProberState::Label::Active ||
         ps(nextState) == link_prober::LinkProberState::Label::Standby)) {
        enterMuxWaitState(nextState);
    }
}

//
// ---> LinkProberStandbyMuxErrorLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberStandby, MuxError, LinkUp} state
//
void ActiveStandbyStateMachine::LinkProberStandbyMuxErrorLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    if ((ps(mCompositeState) != ps(nextState)) &&
        (ps(nextState) == link_prober::LinkProberState::Label::Active ||
         ps(nextState) == link_prober::LinkProberState::Label::Standby)) {
        enterMuxWaitState(nextState);
    }
}

//
// ---> LinkProberWaitMuxActiveLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberWait, MuxActive, LinkUp} state
//
void ActiveStandbyStateMachine::LinkProberWaitMuxActiveLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer(mWaitActiveUpCount > 7? MAX_BACKOFF_FACTOR : (1<<mWaitActiveUpCount));

    if (mWaitActiveUpCount++ & 0x1) {
        mSuspendTxFnPtr(mMuxPortConfig.getLinkWaitTimeout_msec());
    }
}

//
// ---> LinkProberWaitMuxStandbyLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberWait, MuxStandby, LinkUp} state
//
void ActiveStandbyStateMachine::LinkProberWaitMuxStandbyLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer(mWaitStandbyUpBackoffFactor);
    mWaitStandbyUpBackoffFactor <<= 1;
    mWaitStandbyUpBackoffFactor = mWaitStandbyUpBackoffFactor > MAX_BACKOFF_FACTOR ? MAX_BACKOFF_FACTOR : mWaitStandbyUpBackoffFactor;
}

//
// ---> LinkProberWaitMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberWait, MuxUnknown, LinkUp} state
//
void ActiveStandbyStateMachine::LinkProberWaitMuxUnknownLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGWARNING(mMuxPortConfig.getPortName());

    startMuxProbeTimer(mMuxUnknownBackoffFactor);
    mMuxUnknownBackoffFactor <<= 1;
    mMuxUnknownBackoffFactor = mMuxUnknownBackoffFactor > MAX_BACKOFF_FACTOR ? MAX_BACKOFF_FACTOR : mMuxUnknownBackoffFactor;
}

//
// ---> LinkProberUnknownMuxActiveLinkDownTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxActive, LinkDown} state
//
void ActiveStandbyStateMachine::LinkProberUnknownMuxActiveLinkDownTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer();
}

//
// ---> LinkProberUnknownMuxStandbyLinkDownTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxStandby, LinkDown} state
//
void ActiveStandbyStateMachine::LinkProberUnknownMuxStandbyLinkDownTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer();
}

//
// ---> LinkProberUnknownMuxUnknownLinkDownTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxUnknown, LinkDown} state
//
void ActiveStandbyStateMachine::LinkProberUnknownMuxUnknownLinkDownTransitionFunction(CompositeState &nextState)
{
    MUXLOGWARNING(mMuxPortConfig.getPortName());

    startMuxProbeTimer(mMuxUnknownBackoffFactor);
    mMuxUnknownBackoffFactor <<= 1;
    mMuxUnknownBackoffFactor = mMuxUnknownBackoffFactor > MAX_BACKOFF_FACTOR ? MAX_BACKOFF_FACTOR : mMuxUnknownBackoffFactor;
}

//
// ---> LinkProberWaitMuxActiveLinkDownTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberWait, MuxActive, LinkDown} state
//
void ActiveStandbyStateMachine::LinkProberWaitMuxActiveLinkDownTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer();
}

//
// ---> LinkProberWaitMuxStandbyLinkDownTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberWait, MuxStandby, LinkDown} state
//
void ActiveStandbyStateMachine::LinkProberWaitMuxStandbyLinkDownTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer();
}

//
// ---> LinkProberWaitMuxUnknownLinkDownTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberWait, MuxUnknown, LinkDown} state
//
void ActiveStandbyStateMachine::LinkProberWaitMuxUnknownLinkDownTransitionFunction(CompositeState &nextState)
{
    MUXLOGWARNING(mMuxPortConfig.getPortName());

    startMuxProbeTimer(mMuxUnknownBackoffFactor);
    mMuxUnknownBackoffFactor <<= 1;
    mMuxUnknownBackoffFactor = mMuxUnknownBackoffFactor > MAX_BACKOFF_FACTOR ? MAX_BACKOFF_FACTOR : mMuxUnknownBackoffFactor;
}

} /* namespace link_manager */
