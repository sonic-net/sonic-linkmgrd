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
 * LinkManagerStateMachine.cpp
 *
 *  Created on: Oct 18, 2020
 *      Author: tamer
 */

#include <boost/bind/bind.hpp>

#include "link_manager/LinkManagerStateMachine.h"
#include "common/MuxLogger.h"
#include "common/MuxException.h"
#include "MuxPort.h"

#define LOG_MUX_STATE_TRANSITION(level, portName, currentState, nextState) \
    do { \
        MUXLOG##level(boost::format("%s: (P: %s, M: %s, L: %s) -> (P: %s, M: %s, L: %s)") % \
            portName % \
            mLinkProberStateName[ps(currentState)] % \
            mMuxStateName[ms(currentState)] % \
            mLinkStateName[ls(currentState)] % \
            mLinkProberStateName[ps(nextState)] % \
            mMuxStateName[ms(nextState)] % \
            mLinkStateName[ls(nextState)] \
        ); \
    } while (0)

#define LOGWARNING_MUX_STATE_TRANSITION(portName, currentState, nextState) \
    LOG_MUX_STATE_TRANSITION(WARNING, portName, currentState, nextState)

#define LOGINFO_MUX_STATE_TRANSITION(portName, currentState, nextState) \
    LOG_MUX_STATE_TRANSITION(INFO, portName, currentState, nextState)

#define MUXLOGTIMEOUT(portname, msg, currentState) \
    do { \
        MUXLOGWARNING(boost::format("%s: %s, current state: (P: %s, M: %s, L: %s)") % \
            portname % \
            msg % \
            mLinkProberStateName[ps(currentState)] % \
            mMuxStateName[ms(currentState)] % \
            mLinkStateName[ls(currentState)] \
        ); \
    } while (0)

namespace link_manager
{

constexpr auto MAX_BACKOFF_FACTOR = 8;

LinkManagerStateMachine::TransitionFunction
    LinkManagerStateMachine::mStateTransitionHandler[link_prober::LinkProberState::Label::Count]
                                                    [mux_state::MuxState::Label::Count]
                                                    [link_state::LinkState::Label::Count];

LinkProberEvent LinkManagerStateMachine::mLinkProberEvent;
MuxStateEvent LinkManagerStateMachine::mMuxStateEvent;
LinkStateEvent LinkManagerStateMachine::mLinkStateEvent;

std::vector<std::string> LinkManagerStateMachine::mLinkProberStateName = {"Active", "Standby", "Unknown", "Wait"};
std::vector<std::string> LinkManagerStateMachine::mMuxStateName = {"Active", "Standby", "Unknown", "Error", "Wait"};
std::vector<std::string> LinkManagerStateMachine::mLinkStateName = {"Up", "Down"};
std::vector<std::string> LinkManagerStateMachine::mLinkHealthName = {"Uninitialized", "Unhealthy", "Healthy"};

//
// ---> LinkManagerStateMachine(
//          mux::MuxPort *muxPortPtr,
//          boost::asio::io_service::strand &strand,
//          common::MuxPortConfig &muxPortConfig
//      );
//
// class constructor
//
LinkManagerStateMachine::LinkManagerStateMachine(
    mux::MuxPort *muxPortPtr,
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig
) :
    StateMachine(strand, muxPortConfig),
    mMuxPortPtr(muxPortPtr),
    mLinkProberStateMachine(*this, strand, muxPortConfig, ps(mCompositeState)),
    mMuxStateMachine(*this, strand, muxPortConfig, ms(mCompositeState)),
    mLinkStateMachine(*this, strand, muxPortConfig, ls(mCompositeState)),
    mDeadlineTimer(strand.context()),
    mWaitTimer(strand.context())
{
    assert(muxPortPtr != nullptr);
    mMuxStateMachine.setWaitStateCause(mux_state::WaitState::WaitStateCause::SwssUpdate);
    mMuxPortPtr->setMuxLinkmgrState(mLabel);
}

//
// ---> initializeTransitionFunctionTable()
//
// initialize static transition function table
//
void LinkManagerStateMachine::initializeTransitionFunctionTable()
{
    MUXLOGWARNING("Initializing State Transition Table...");
    for (uint8_t linkProberState = link_prober::LinkProberState::Label::Active;
            linkProberState < link_prober::LinkProberState::Label::Count; linkProberState++) {
        for (uint8_t muxState = mux_state::MuxState::Label::Active;
                muxState < mux_state::MuxState::Label::Count; muxState++) {
            for (uint8_t linkState = link_state::LinkState::Label::Up;
                    linkState < link_state::LinkState::Label::Count; linkState++) {
                mStateTransitionHandler[linkProberState][muxState][linkState] =
                    boost::bind(
                        &LinkManagerStateMachine::noopTransitionFunction,
                        boost::placeholders::_1,
                        boost::placeholders::_2
                    );
            }
        }
    }

    mStateTransitionHandler[link_prober::LinkProberState::Label::Standby]
                           [mux_state::MuxState::Label::Active]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberStandbyMuxActiveLinkUpTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Active]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberUnknownMuxActiveLinkUpTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Active]
                           [mux_state::MuxState::Label::Standby]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberActiveMuxStandbyLinkUpTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Standby]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberUnknownMuxStandbyLinkUpTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Active]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberActiveMuxUnknownLinkUpTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Standby]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberStandbyMuxUnknownLinkUpTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberUnknownMuxUnknownLinkUpTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Active]
                           [mux_state::MuxState::Label::Error]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberActiveMuxErrorLinkUpTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Standby]
                           [mux_state::MuxState::Label::Error]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberStandbyMuxErrorLinkUpTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Wait]
                           [mux_state::MuxState::Label::Active]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberWaitMuxActiveLinkUpTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Wait]
                           [mux_state::MuxState::Label::Standby]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberWaitMuxStandbyLinkUpTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Wait]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Up] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberWaitMuxUnknownLinkUpTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Active]
                           [link_state::LinkState::Label::Down] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberUnknownMuxActiveLinkDownTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Standby]
                           [link_state::LinkState::Label::Down] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberUnknownMuxStandbyLinkDownTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Down] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberUnknownMuxUnknownLinkDownTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Wait]
                           [mux_state::MuxState::Label::Active]
                           [link_state::LinkState::Label::Down] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberWaitMuxActiveLinkDownTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Wait]
                           [mux_state::MuxState::Label::Standby]
                           [link_state::LinkState::Label::Down] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberWaitMuxStandbyLinkDownTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
    mStateTransitionHandler[link_prober::LinkProberState::Label::Wait]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Down] =
        boost::bind(
            &LinkManagerStateMachine::LinkProberWaitMuxUnknownLinkDownTransitionFunction,
            boost::placeholders::_1,
            boost::placeholders::_2
        );
}

//
// ---> setLabel(Label label);
//
// sets linkmgr State db state
//
void LinkManagerStateMachine::setLabel(Label label) {
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
// ---> enterLinkProberState(CompositeState &nextState, link_prober::LinkProberState::Label label);
//
// force LinkProberState to switch state
//
void LinkManagerStateMachine::enterLinkProberState(CompositeState &nextState, link_prober::LinkProberState::Label label)
{
    mLinkProberStateMachine.enterState(label);
    ps(nextState) = label;
}

//
// ---> enterMuxState(CompositeState &nextState, mux_state::MuxState::Label label);
//
// force MuxState to switch state
//
void LinkManagerStateMachine::enterMuxState(CompositeState &nextState, mux_state::MuxState::Label label)
{
    mMuxStateMachine.enterState(label);
    ms(nextState) = label;
}

//
// ---> enterLinkState(CompositeState &nextState, link_state::LinkState::Label label);
//
// force LinkState to switch state
//
void LinkManagerStateMachine::enterLinkState(CompositeState &nextState, link_state::LinkState::Label label)
{
    mLinkStateMachine.enterState(label);
    ls(nextState) = label;
}

//
// ---> enterMuxWaitState(CompositeState &nextState);
//
// force MuxState to switch to WaitState
//
void LinkManagerStateMachine::enterMuxWaitState(CompositeState &nextState)
{
    enterMuxState(nextState, mux_state::MuxState::Label::Wait);
    mMuxStateMachine.setWaitStateCause(mux_state::WaitState::WaitStateCause::DriverUpdate);
    mMuxPortPtr->probeMuxState();
    startMuxWaitTimer();
}

//
// ---> switchMuxState(CompositeState &nextState, mux_state::MuxState::Label label, bool forceSwitch);
//
// switch Mux to switch via xcvrd to state label provider
//
void LinkManagerStateMachine::switchMuxState(
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
        enterMuxState(nextState, mux_state::MuxState::Label::Wait);
        mMuxStateMachine.setWaitStateCause(mux_state::WaitState::WaitStateCause::SwssUpdate);
        mMuxPortPtr->postMetricsEvent(Metrics::SwitchingStart, label);
        mMuxPortPtr->setMuxState(label);
        mDeadlineTimer.cancel();
        startMuxWaitTimer();
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
void LinkManagerStateMachine::handleSwssBladeIpv4AddressUpdate(boost::asio::ip::address address)
{
    if (!mComponentInitState.test(LinkProberComponent)) {
        mMuxPortConfig.setBladeIpv4Address(address);

        try {
            mLinkProberPtr = std::make_shared<link_prober::LinkProber> (
                mMuxPortConfig,
                getStrand().context(),
                mLinkProberStateMachine
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
void LinkManagerStateMachine::activateStateMachine()
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
}

//
// ---> handleStateChange(LinkProberEvent &event, link_prober::LinkProberState::Label state);
//
// handles LinkProverEvent
//
void LinkManagerStateMachine::handleStateChange(LinkProberEvent &event, link_prober::LinkProberState::Label state)
{
    if ((dynamic_cast<link_prober::LinkProberState *> (mLinkProberStateMachine.getCurrentState()))->getStateLabel() == state) {
        MUXLOGWARNING(boost::format("%s: Received link prober event, new state: %s") %
            mMuxPortConfig.getPortName() %
            mLinkProberStateName[state]
        );

        CompositeState nextState = mCompositeState;
        ps(nextState) = state;
        mStateTransitionHandler[ps(nextState)][ms(nextState)][ls(nextState)](this, nextState);
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
void LinkManagerStateMachine::handleStateChange(MuxStateEvent &event, mux_state::MuxState::Label state)
{
    if ((dynamic_cast<mux_state::MuxState *> (mMuxStateMachine.getCurrentState()))->getStateLabel() == state) {
        MUXLOGINFO(boost::format("%s: Received mux state event, new state: %s") %
            mMuxPortConfig.getPortName() %
            mMuxStateName[state]
        );

        CompositeState nextState = mCompositeState;
        ms(nextState) = state;
        mStateTransitionHandler[ps(nextState)][ms(nextState)][ls(nextState)](this, nextState);
        LOGINFO_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
    }

    if (ms(mCompositeState) != mux_state::MuxState::Wait) {
        // Verify if state db MUX state matches the driver/current MUX state
        mMuxPortPtr->getMuxState();
    }

    if (ms(mCompositeState) != mux_state::MuxState::Unknown) {
        mMuxUnknownBackoffFactor = 1;
    }

    updateMuxLinkmgrState();
}

//
// ---> handleStateChange(LinkStateEvent &event, link_state::LinkState::Label state);
//
// handles LinkStateEvent
//
void LinkManagerStateMachine::handleStateChange(LinkStateEvent &event, link_state::LinkState::Label state)
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
            initLinkProberState(nextState);
//            enterMuxWaitState(nextState);
        } else if (ls(mCompositeState) == link_state::LinkState::Up &&
                   ls(nextState) == link_state::LinkState::Down &&
                   ms(mCompositeState) == mux_state::MuxState::Label::Active) {
            // switch MUX to standby since we are entering LinkDown state
            switchMuxState(nextState, mux_state::MuxState::Label::Standby);
        } else {
            mStateTransitionHandler[ps(nextState)][ms(nextState)][ls(nextState)](this, nextState);
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
void LinkManagerStateMachine::handleGetServerMacAddressNotification(std::array<uint8_t, ETHER_ADDR_LEN> address)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    if (address != mMuxPortConfig.getBladeMacAddress()) {
        mMuxPortConfig.setBladeMacAddress(address);
        if (mUpdateEthernetFrameFnPtr) {
            mUpdateEthernetFrameFnPtr();
        } else {
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
void LinkManagerStateMachine::handleGetMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGINFO(boost::format("%s: state db mux state: %s") % mMuxPortConfig.getPortName() % mMuxStateName[label]);

    if (mComponentInitState.all() && ms(mCompositeState) != label &&
        ms(mCompositeState) != mux_state::MuxState::Wait &&
        ms(mCompositeState) != mux_state::MuxState::Error) {
        // notify swss of mux state change
        MUXLOGWARNING(boost::format("%s: Switching MUX state from '%s' to '%s' to match linkmgrd/xcvrd state") %
            mMuxPortConfig.getPortName() %
            mMuxStateName[label] %
            mMuxStateName[ms(mCompositeState)]
        );
        switchMuxState(mCompositeState, ms(mCompositeState), true);
    }
}

//
// ---> handleProbeMuxStateNotification(mux_state::MuxState::Label label);
//
// handle MUX state notification. Source of notification could be app_db via xcvrd
//
void LinkManagerStateMachine::handleProbeMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGINFO(boost::format("%s: app db mux state: %s") % mMuxPortConfig.getPortName() % mMuxStateName[label]);

    mWaitTimer.cancel();

    if (mComponentInitState.all()) {
        if (mMuxStateMachine.getWaitStateCause() != mux_state::WaitState::WaitStateCause::DriverUpdate) {
            MUXLOGERROR(boost::format("%s: Received unsolicited MUX state probe notification!") %
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
void LinkManagerStateMachine::handleMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGWARNING(boost::format("%s: state db mux state: %s") % mMuxPortConfig.getPortName() % mMuxStateName[label]);

    mWaitTimer.cancel();

    if (mComponentInitState.all()) {
        if (mMuxStateMachine.getWaitStateCause() != mux_state::WaitState::WaitStateCause::SwssUpdate ||
            ms(mCompositeState) != mux_state::MuxState::Wait) {
            MUXLOGERROR(boost::format("%s: Received unsolicited MUX state change notification!") %
                mMuxPortConfig.getPortName()
            );
        }
        mProbePeerTorFnPtr();
        postMuxStateEvent(label);
        mMuxPortPtr->postMetricsEvent(Metrics::SwitchingEnd, label);
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
void LinkManagerStateMachine::handleSwssLinkStateNotification(const link_state::LinkState::Label label)
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

//
// ---> handleMuxConfigNotification(const common::MuxPortConfig::Mode mode);
//
// handle MUX configuration change notification
//
void LinkManagerStateMachine::handleMuxConfigNotification(const common::MuxPortConfig::Mode mode)
{
    if (mComponentInitState.all()) {
        if (mode == common::MuxPortConfig::Mode::Active &&
            ms(mCompositeState) != mux_state::MuxState::Label::Active &&
            ms(mCompositeState) != mux_state::MuxState::Label::Wait) {
            CompositeState nextState = mCompositeState;
            enterLinkProberState(nextState, link_prober::LinkProberState::Wait);
            switchMuxState(nextState, mux_state::MuxState::Label::Active);
            LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
            mCompositeState = nextState;
        } else if(mode == common::MuxPortConfig::Mode::Standby &&
                  ms(mCompositeState) != mux_state::MuxState::Label::Standby &&
                  ms(mCompositeState) != mux_state::MuxState::Label::Wait) {
            mSendPeerSwitchCommandFnPtr();
        } else {
            mMuxStateMachine.setWaitStateCause(mux_state::WaitState::WaitStateCause::DriverUpdate);
            mMuxPortPtr->probeMuxState();
        }

        updateMuxLinkmgrState();
    }

    mMuxPortConfig.setMode(mode);
}

//
// ---> handleSuspendTimerExpiry();
//
// handle suspend timer expiry notification from LinkProber
//
void LinkManagerStateMachine::handleSuspendTimerExpiry()
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
    }
}

//
// ---> handleSwitchActiveCommandCompletion();
//
// handle completion of sending switch command to peer ToR
//
void LinkManagerStateMachine::handleSwitchActiveCommandCompletion()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    if (ms(mCompositeState) != mux_state::MuxState::Label::Standby &&
        ms(mCompositeState) != mux_state::MuxState::Label::Wait) {
        CompositeState nextState = mCompositeState;
        enterLinkProberState(nextState, link_prober::LinkProberState::Wait);
        switchMuxState(nextState, mux_state::MuxState::Label::Standby);
        LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
    }
}

//
// ---> handleSwitchActiveRequestEvent();
//
// handle switch active request from peer ToR
//
void LinkManagerStateMachine::handleSwitchActiveRequestEvent()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    if (ms(mCompositeState) != mux_state::MuxState::Label::Active &&
        ms(mCompositeState) != mux_state::MuxState::Label::Wait) {
        CompositeState nextState = mCompositeState;
        enterLinkProberState(nextState, link_prober::LinkProberState::Wait);
        switchMuxState(nextState, mux_state::MuxState::Label::Active);
        LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
    }
}

//
// ---> updateMuxLinkmgrState();
//
// Update State DB MUX LinkMgr state
//
void LinkManagerStateMachine::updateMuxLinkmgrState()
{
    Label label = Label::Unhealthy;
    if (ls(mCompositeState) == link_state::LinkState::Label::Up &&
        ((ps(mCompositeState) == link_prober::LinkProberState::Label::Active &&
         ms(mCompositeState) == mux_state::MuxState::Label::Active) ||
        (ps(mCompositeState) == link_prober::LinkProberState::Label::Standby &&
         ms(mCompositeState) == mux_state::MuxState::Label::Standby))) {
        label = Label::Healthy;
    }

    setLabel(label);
}

//
// ---> startMuxProbeTimer(uint32_t factor);
//
// start a timer to monitor the MUX state
//
void LinkManagerStateMachine::startMuxProbeTimer(uint32_t factor)
{
    mDeadlineTimer.expires_from_now(boost::posix_time::milliseconds(
        factor * mMuxPortConfig.getNegativeStateChangeRetryCount() * mMuxPortConfig.getTimeoutIpv4_msec()
    ));
    mDeadlineTimer.async_wait(getStrand().wrap(boost::bind(
        &LinkManagerStateMachine::handleMuxProbeTimeout,
        this,
        boost::asio::placeholders::error
    )));
}

//
// ---> handleMuxProbeTimeout(boost::system::error_code errorCode);
//
// handle when LinkProber heartbeats were lost due link down, bad cable or server down
//
void LinkManagerStateMachine::handleMuxProbeTimeout(boost::system::error_code errorCode)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    if (errorCode == boost::system::errc::success &&
        (ps(mCompositeState) == link_prober::LinkProberState::Label::Wait ||
         ms(mCompositeState) == mux_state::MuxState::Label::Unknown ||
         ls(mCompositeState) == link_state::LinkState::Label::Down)) {
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
void LinkManagerStateMachine::startMuxWaitTimer(uint32_t factor)
{
    mWaitTimer.expires_from_now(boost::posix_time::milliseconds(
        factor * mMuxPortConfig.getNegativeStateChangeRetryCount() * mMuxPortConfig.getTimeoutIpv4_msec()
    ));
    mWaitTimer.async_wait(getStrand().wrap(boost::bind(
        &LinkManagerStateMachine::handleMuxWaitTimeout,
        this,
        boost::asio::placeholders::error
    )));
}

//
// ---> handleMuxWaitTimeout(boost::system::error_code errorCode);
//
// handle when xcrvrd/orchagent has timed out responding to linkmgrd.
//
void LinkManagerStateMachine::handleMuxWaitTimeout(boost::system::error_code errorCode)
{
    if (errorCode == boost::system::errc::success) {
        if (mMuxStateMachine.getWaitStateCause() == mux_state::WaitState::WaitStateCause::SwssUpdate) {
            MUXLOGTIMEOUT(mMuxPortConfig.getPortName(), "orchagent timed out responding to linkmgrd", mCompositeState);
        } else if (mMuxStateMachine.getWaitStateCause() == mux_state::WaitState::WaitStateCause::DriverUpdate) {
            MUXLOGTIMEOUT(mMuxPortConfig.getPortName(), "xcvrd timed out responding to linkmgrd", mCompositeState);
        } else {
            MUXLOGTIMEOUT(mMuxPortConfig.getPortName(), "Unknown timeout reason!!!", mCompositeState);
        }
        startMuxWaitTimer(MAX_BACKOFF_FACTOR);
    }
}

//
// ---> initLinkProberState(CompositeState &compositeState);
//
// initialize LinkProberState when configuring the composite state machine
//
void LinkManagerStateMachine::initLinkProberState(CompositeState &compositeState)
{
    switch (ms(compositeState)) {
    case mux_state::MuxState::Label::Active:
        enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Active);
        break;
    case mux_state::MuxState::Label::Standby:
        enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Standby);
        break;
    case mux_state::MuxState::Label::Unknown:
        enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Unknown);
        break;
    case mux_state::MuxState::Label::Error:
        enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Wait);
        break;
    case mux_state::MuxState::Label::Wait:
        enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Unknown);
        break;
    default:
        break;
    }
}

//
// ---> postMuxStateEvent(mux_state::MuxState::Label label)
//
// post event to MUX state machine to change state
//
void LinkManagerStateMachine::postMuxStateEvent(mux_state::MuxState::Label label)
{
    switch (label) {
    case mux_state::MuxState::Label::Active:
        mMuxStateMachine.postMuxStateEvent(mux_state::MuxStateMachine::getActiveEvent());
        break;
    case mux_state::MuxState::Label::Standby:
        mMuxStateMachine.postMuxStateEvent(mux_state::MuxStateMachine::getStandbyEvent());
        break;
    case mux_state::MuxState::Label::Unknown:
        mMuxStateMachine.postMuxStateEvent(mux_state::MuxStateMachine::getUnknownEvent());
        break;
    case mux_state::MuxState::Label::Error:
        mMuxStateMachine.postMuxStateEvent(mux_state::MuxStateMachine::getErrorEvent());
        break;
    default:
        break;
    }
}

//
// ---> noopTransitionFunction(CompositeState &nextState);
//
// No-op transition function
//
void LinkManagerStateMachine::noopTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
}

//
// ---> LinkProberStandbyMuxActiveLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberStandby, MuxActive, LinkUp} state
//
void LinkManagerStateMachine::LinkProberStandbyMuxActiveLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    // Probe the MUX state as internal MUX state is active while LP reports standby link
    enterMuxWaitState(nextState);
}

//
// ---> LinkProberUnknownMuxActiveLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxActive, LinkUp} state
//
void LinkManagerStateMachine::LinkProberUnknownMuxActiveLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    // Suspend TX probes to help peer ToR takes over in case active link is bad
    mSuspendTxFnPtr(mMuxPortConfig.getLinkWaitTimeout_msec());
    mWaitActiveUpCount = 0;
}

//
// ---> LinkProberActiveMuxStandbyLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberActive, MuxStandby, LinkUp} state
//
void LinkManagerStateMachine::LinkProberActiveMuxStandbyLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    enterMuxWaitState(nextState);
}

//
// ---> LinkProberUnknownMuxStandbyLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxStandby, LinkUp} state
//
void LinkManagerStateMachine::LinkProberUnknownMuxStandbyLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    enterLinkProberState(nextState, link_prober::LinkProberState::Wait);

    // Start switching MUX to active state as we lost HB from active ToR
    switchMuxState(nextState, mux_state::MuxState::Label::Active);
    mDeadlineTimer.cancel();
    mWaitActiveUpCount = 0;
}

//
// ---> LinkProberActiveMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberActive, MuxUnknown, LinkUp} state
//
void LinkManagerStateMachine::LinkProberActiveMuxUnknownLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    enterMuxWaitState(nextState);
}

//
// ---> LinkProberStandbyMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberStandby, MuxUnknown, LinkUp} state
//
void LinkManagerStateMachine::LinkProberStandbyMuxUnknownLinkUpTransitionFunction(
    CompositeState &nextState
)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    enterMuxWaitState(nextState);
}

//
// ---> LinkProberUnknownMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxUnknown, LinkUp} state
//
void LinkManagerStateMachine::LinkProberUnknownMuxUnknownLinkUpTransitionFunction(
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
void LinkManagerStateMachine::LinkProberActiveMuxErrorLinkUpTransitionFunction(
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
void LinkManagerStateMachine::LinkProberStandbyMuxErrorLinkUpTransitionFunction(
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
void LinkManagerStateMachine::LinkProberWaitMuxActiveLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer();

    if (mWaitActiveUpCount++ & 0x1) {
        mSuspendTxFnPtr(mMuxPortConfig.getLinkWaitTimeout_msec());
    }
}

//
// ---> LinkProberWaitMuxStandbyLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberWait, MuxStandby, LinkUp} state
//
void LinkManagerStateMachine::LinkProberWaitMuxStandbyLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer();
}

//
// ---> LinkProberWaitMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberWait, MuxUnknown, LinkUp} state
//
void LinkManagerStateMachine::LinkProberWaitMuxUnknownLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGERROR(mMuxPortConfig.getPortName());

    startMuxProbeTimer(mMuxUnknownBackoffFactor);
    mMuxUnknownBackoffFactor <<= 1;
    mMuxUnknownBackoffFactor = mMuxUnknownBackoffFactor > MAX_BACKOFF_FACTOR ? MAX_BACKOFF_FACTOR : mMuxUnknownBackoffFactor;
}

//
// ---> LinkProberUnknownMuxActiveLinkDownTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxActive, LinkDown} state
//
void LinkManagerStateMachine::LinkProberUnknownMuxActiveLinkDownTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer();
}

//
// ---> LinkProberUnknownMuxStandbyLinkDownTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxStandby, LinkDown} state
//
void LinkManagerStateMachine::LinkProberUnknownMuxStandbyLinkDownTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer();
}

//
// ---> LinkProberUnknownMuxUnknownLinkDownTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxUnknown, LinkDown} state
//
void LinkManagerStateMachine::LinkProberUnknownMuxUnknownLinkDownTransitionFunction(CompositeState &nextState)
{
    MUXLOGERROR(mMuxPortConfig.getPortName());

    startMuxProbeTimer(mMuxUnknownBackoffFactor);
    mMuxUnknownBackoffFactor <<= 1;
    mMuxUnknownBackoffFactor = mMuxUnknownBackoffFactor > MAX_BACKOFF_FACTOR ? MAX_BACKOFF_FACTOR : mMuxUnknownBackoffFactor;
}

//
// ---> LinkProberWaitMuxActiveLinkDownTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberWait, MuxActive, LinkDown} state
//
void LinkManagerStateMachine::LinkProberWaitMuxActiveLinkDownTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer();
}

//
// ---> LinkProberWaitMuxStandbyLinkDownTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberWait, MuxStandby, LinkDown} state
//
void LinkManagerStateMachine::LinkProberWaitMuxStandbyLinkDownTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    startMuxProbeTimer();
}

//
// ---> LinkProberWaitMuxUnknownLinkDownTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberWait, MuxUnknown, LinkDown} state
//
void LinkManagerStateMachine::LinkProberWaitMuxUnknownLinkDownTransitionFunction(CompositeState &nextState)
{
    MUXLOGERROR(mMuxPortConfig.getPortName());

    startMuxProbeTimer(mMuxUnknownBackoffFactor);
    mMuxUnknownBackoffFactor <<= 1;
    mMuxUnknownBackoffFactor = mMuxUnknownBackoffFactor > MAX_BACKOFF_FACTOR ? MAX_BACKOFF_FACTOR : mMuxUnknownBackoffFactor;
}

} /* namespace link_manager */
