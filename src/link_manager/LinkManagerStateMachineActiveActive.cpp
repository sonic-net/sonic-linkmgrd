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

#include <boost/bind/bind.hpp>

#include "link_manager/LinkManagerStateMachineActiveActive.h"
#include "common/MuxLogger.h"
#include "common/MuxException.h"
#include "MuxPort.h"

namespace link_manager
{

constexpr auto MAX_BACKOFF_FACTOR = 32;

//
// ---> ActiveActiveStateMachine(
//          mux::MuxPort *muxPortPtr,
//          boost::asio::io_service::strand &strand,
//          common::MuxPortConfig &muxPortConfig
//      )
//
// Construct a new Active Active State Machine object
//
ActiveActiveStateMachine::ActiveActiveStateMachine(
    mux::MuxPort *muxPortPtr,
    boost::asio::io_service::strand &strand,
    common::MuxPortConfig &muxPortConfig
)
    : LinkManagerStateMachineBase(
          muxPortPtr,
          strand,
          muxPortConfig,
          {link_prober::LinkProberState::Label::Wait,
           mux_state::MuxState::Label::Wait,
           link_state::LinkState::Label::Down}
      ),
      mDeadlineTimer(strand.context()),
      mWaitTimer(strand.context()),
      mPeerWaitTimer(strand.context())
{
    assert(muxPortPtr != nullptr);
    mMuxPortPtr->setMuxLinkmgrState(mLabel);
    initializeTransitionFunctionTable();
}

//
// ---> activateStateMachine();
//
// activate the state machine by starting the link prober.
//
void ActiveActiveStateMachine::activateStateMachine()
{
    if (mComponentInitState.all()) {
        std::array<uint8_t, ETHER_ADDR_LEN> macAddress = mMuxPortConfig.getBladeMacAddress();
        std::array<char, 3 *ETHER_ADDR_LEN> macAddressStr = {0};

        snprintf(
            macAddressStr.data(), macAddressStr.size(), "%02x:%02x:%02x:%02x:%02x:%02x",
            macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]
        );

        MUXLOGWARNING(
            boost::format("%s: MUX port link prober initialized with server IP: %s, server MAC: %s") %
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

/*--------------------------------------------------------------------------------------------------------------
|  db event handlers
---------------------------------------------------------------------------------------------------------------*/

//
// ---> handleSwssSoCIpv4AddressUpdate(boost::asio::ip::address address);
//
// initialize the link prober component. Note if this is the last component to be initialized,
// state machine will be activated
//
void ActiveActiveStateMachine::handleSwssSoCIpv4AddressUpdate(boost::asio::ip::address address)
{
    if (!mComponentInitState.test(LinkProberComponent)) {
        mMuxPortConfig.setBladeIpv4Address(address);

        try {
            mLinkProberPtr = std::make_shared<link_prober::LinkProber>(
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
            mShutdownTxFnPtr = boost::bind(
                &link_prober::LinkProber::shutdownTxProbes, mLinkProberPtr.get()
            );
            mRestartTxFnPtr = boost::bind(
                &link_prober::LinkProber::restartTxProbes, mLinkProberPtr.get()
            );

            setComponentInitState(LinkProberComponent);

            activateStateMachine();
        } catch (const std::bad_alloc &ex) {
            std::ostringstream errMsg;
            errMsg << "Failed allocate memory. Exception details: " << ex.what();

            throw MUX_ERROR(BadAlloc, errMsg.str());
        }
    } else if (address != mMuxPortConfig.getBladeIpv4Address()) {
        mMuxPortConfig.setBladeIpv4Address(address);
        mUpdateEthernetFrameFnPtr();
    }
}

//
// ---> handleMuxStateNotification(mux_state::MuxState::Label label);
//
// handle MUX state notification
//
void ActiveActiveStateMachine::handleMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGWARNING(boost::format("%s: state db mux state: %s") % mMuxPortConfig.getPortName() % mMuxStateName[label]);

    mWaitTimer.cancel();
    mLastMuxStateNotification = label;

    if (mComponentInitState.all()) {
        if (mMuxStateMachine.getWaitStateCause() != mux_state::WaitState::WaitStateCause::SwssUpdate) {
            MUXLOGWARNING(boost::format("%s: Received unsolicited MUX state change notification!") % mMuxPortConfig.getPortName());
        }
        mProbePeerTorFnPtr();
        mResumeTxFnPtr();
        postMuxStateEvent(label);
    } else {
        if (label == mux_state::MuxState::Unknown) {
            // probe xcvrd to read the current mux state
            probeMuxState();
        }
        enterMuxState(mCompositeState, label);
        setComponentInitState(MuxStateComponent);
        activateStateMachine();
    }
}

//
// ---> handleSwssLinkStateNotification(const link_state::LinkState::Label label);
//
// handle link state notification
//
void ActiveActiveStateMachine::handleSwssLinkStateNotification(const link_state::LinkState::Label label)
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
        setComponentInitState(LinkStateComponent);
        activateStateMachine();
    }
}

//
// ---> handleMuxConfigNotification(const common::MuxPortConfig::Mode mode);
//
// handle MUX configuration change notification
//
void ActiveActiveStateMachine::handleMuxConfigNotification(const common::MuxPortConfig::Mode mode)
{
    if (mComponentInitState.all()) {
        CompositeState nextState = mCompositeState;
        if (mode == common::MuxPortConfig::Mode::Active && ms(mCompositeState) != mux_state::MuxState::Label::Active) {
            switchMuxState(nextState, mux_state::MuxState::Label::Active);
        } else if (mode == common::MuxPortConfig::Mode::Standby && ms(mCompositeState) != mux_state::MuxState::Label::Standby) {
            switchMuxState(nextState, mux_state::MuxState::Label::Standby);
        } else {
            // enforce link prober state to match mux state to trigger possible transitions
            initLinkProberState(nextState);
            probeMuxState();
        }
        LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
        updateMuxLinkmgrState();
    }

    mMuxPortConfig.setMode(mode);
    shutdownOrRestartLinkProberOnDefaultRoute();
}

//
// ---> handleProbeMuxStateNotification(mux_state::MuxState::Label label);
//
// handle probe MUX state notification
//
void ActiveActiveStateMachine::handleProbeMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGWARNING(boost::format("%s: app db mux state: %s") % mMuxPortConfig.getPortName() % mMuxStateName[label]);

    mWaitTimer.cancel();
    if (label == mux_state::MuxState::Label::Active || label == mux_state::MuxState::Label::Standby) {
        mMuxProbeBackoffFactor = 1;
        mDeadlineTimer.cancel();
    }

    if (mComponentInitState.all()) {
        if (mMuxStateMachine.getWaitStateCause() != mux_state::WaitState::WaitStateCause::DriverUpdate) {
            MUXLOGWARNING(
                boost::format("%s: Received unsolicited MUX state probe notification!") %
                mMuxPortConfig.getPortName()
            );
        }

        postMuxStateEvent(label);
    } else {
        if (label == mux_state::MuxState::Unknown) {
            MUXLOGWARNING(
                boost::format("%s: xcvrd reports MUX state as '%s' during init. phase! Is there a functioning MUX?") %
                mMuxPortConfig.getPortName() %
                mMuxStateName[label]
            );
            probeMuxState();
        }

        enterMuxState(mCompositeState, label);
        setComponentInitState(MuxStateComponent);
        activateStateMachine();
    }
}

//
// ---> handlePeerMuxStateNotification(mux_state::MuxState::Label label);
//
// handle peer MUX state notification
//
void ActiveActiveStateMachine::handlePeerMuxStateNotification(mux_state::MuxState::Label label)
{
    MUXLOGWARNING(boost::format("%s: state db mux state: %s") % mMuxPortConfig.getPortName() % mMuxStateName[label]);

    mPeerWaitTimer.cancel();
    enterPeerMuxState(label);
}

//
// ---> handleGetServerMacAddressNotification(std::array<uint8_t, ETHER_ADDR_LEN> address);
//
// handle get Server MAC address
//
void ActiveActiveStateMachine::handleGetServerMacAddressNotification(std::array<uint8_t, ETHER_ADDR_LEN> address)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    mMuxPortConfig.setLastUpdatedMacAddress(address);
    if (!mMuxPortConfig.getIfUseWellKnownMacActiveActive() && address != mMuxPortConfig.getBladeMacAddress()) {
        mMuxPortConfig.setBladeMacAddress(address);
        if (mUpdateEthernetFrameFnPtr) {
            mUpdateEthernetFrameFnPtr();
        } else {
            std::array<char, 3 *ETHER_ADDR_LEN> addressStr = {0};
            snprintf(
                addressStr.data(), addressStr.size(), "%02x:%02x:%02x:%02x:%02x:%02x",
                address[0], address[1], address[2], address[3], address[4], address[5]
            );

            MUXLOGERROR(
                boost::format("%s: failed to update Ethernet frame with mac '%s', link prober init state: %d") %
                mMuxPortConfig.getPortName() %
                addressStr.data() %
                mComponentInitState.test(LinkProberComponent)
            );
        }
    }
}

//
// ---> handleUseWellKnownMacAddressNotification();
//
// handle use well known mac address
//
void ActiveActiveStateMachine::handleUseWellKnownMacAddressNotification()
{
    MUXLOGINFO(mMuxPortConfig.getPortName());

    std::array<uint8_t, ETHER_ADDR_LEN> address;
    if (mMuxPortConfig.getIfUseWellKnownMacActiveActive()) {
        address = mMuxPortConfig.getWellKnownMacAddress();
    } else {
        address = mMuxPortConfig.getLastUpdatedMacAddress();
    }
    mMuxPortConfig.setBladeMacAddress(address);

    if (mUpdateEthernetFrameFnPtr) {
        mUpdateEthernetFrameFnPtr();
    } else {
        std::array<char, 3 *ETHER_ADDR_LEN> addressStr = {0};
        snprintf(
            addressStr.data(), addressStr.size(), "%02x:%02x:%02x:%02x:%02x:%02x",
            address[0], address[1], address[2], address[3], address[4], address[5]
        );

        MUXLOGERROR(
            boost::format("%s: failed to update Ethernet frame with mac '%s', link prober init state: %d") %
            mMuxPortConfig.getPortName() %
            addressStr.data() %
            mComponentInitState.test(LinkProberComponent)
        );
    }
}

/*--------------------------------------------------------------------------------------------------------------
|  link prober event handlers
---------------------------------------------------------------------------------------------------------------*/

//
// ---> handleSuspendTimerExpiry();
//
// handle completion of sending switch command to peer ToR
//
void ActiveActiveStateMachine::handleSuspendTimerExpiry()
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());
    mResumeTxFnPtr();
}

/*--------------------------------------------------------------------------------------------------------------
|  state handlers
---------------------------------------------------------------------------------------------------------------*/

//
// ---> handleStateChange(LinkProberEvent &event, link_prober::LinkProberState::Label state);
//
// handles LinkProberEvent
//
void ActiveActiveStateMachine::handleStateChange(
    LinkProberEvent &event,
    link_prober::LinkProberState::Label state
)
{
    if ((dynamic_cast<link_prober::LinkProberState *>(mLinkProberStateMachinePtr->getCurrentState()))->getStateLabel() == state) {
        MUXLOGWARNING(
            boost::format("%s: Received link prober event, new state: %s") %
            mMuxPortConfig.getPortName() %
            mLinkProberStateName[state]
        );

        CompositeState nextState = mCompositeState;
        ps(nextState) = state;
        mStateTransitionHandler[ps(nextState)][ms(nextState)][ls(nextState)](nextState);
        LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
    }

    updateMuxLinkmgrState();
}

//
// ---> handleStateChange(MuxStateEvent &event, mux_state::MuxState::Label state);
//
// handles MuxStateEvent
//
void ActiveActiveStateMachine::handleStateChange(
    MuxStateEvent &event,
    mux_state::MuxState::Label state
)
{
    if ((dynamic_cast<mux_state::MuxState *>(mMuxStateMachine.getCurrentState()))->getStateLabel() == state) {
        MUXLOGWARNING(
            boost::format("%s: Received mux state event, new state: %s") %
            mMuxPortConfig.getPortName() %
            mMuxStateName[state]
        );

        CompositeState nextState = mCompositeState;
        ms(nextState) = state;
        mStateTransitionHandler[ps(nextState)][ms(nextState)][ls(nextState)](nextState);
        LOGINFO_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
    }

    updateMuxLinkmgrState();
}

//
// ---> handleStateChange(LinkStateEvent &event, link_state::LinkState::Label state);
//
// handles LinkStateEvent
//
void ActiveActiveStateMachine::handleStateChange(
    LinkStateEvent &event,
    link_state::LinkState::Label state
)
{
    if ((dynamic_cast<link_state::LinkState *>(mLinkStateMachine.getCurrentState()))->getStateLabel() == state) {
        MUXLOGWARNING(
            boost::format("%s: Received link state event, new state: %s") %
            mMuxPortConfig.getPortName() %
            mLinkStateName[state]
        );

        CompositeState nextState = mCompositeState;
        ls(nextState) = state;
        if (ls(mCompositeState) == link_state::LinkState::Down && ls(nextState) == link_state::LinkState::Up) {
            initLinkProberState(nextState);
            initPeerLinkProberState();
        } else if (ls(mCompositeState) == link_state::LinkState::Up && ls(nextState) == link_state::LinkState::Down && ms(mCompositeState) == mux_state::MuxState::Label::Active) {
            switchMuxState(nextState, mux_state::MuxState::Label::Standby);
        } else {
            mStateTransitionHandler[ps(nextState)][ms(nextState)][ls(nextState)](nextState);
        }
        LOGWARNING_MUX_STATE_TRANSITION(mMuxPortConfig.getPortName(), mCompositeState, nextState);
        mCompositeState = nextState;
    }

    updateMuxLinkmgrState();
}

//
// ---> handlePeerStateChange(LinkProberEvent &event, link_prober::LinkProberState::Label state);
//
// handles peer LinkProberEvent
//
void ActiveActiveStateMachine::handlePeerStateChange(
    LinkProberEvent &event,
    link_prober::LinkProberState::Label state
)
{
    if ((dynamic_cast<link_prober::LinkProberState *>(mLinkProberStateMachinePtr->getCurrentPeerState()))->getStateLabel() == state) {
        MUXLOGWARNING(
            boost::format("%s: Received peer link prober event, new state: %s") %
            mMuxPortConfig.getPortName() %
            mLinkProberStateName[state]
        );

        enterPeerLinkProberState(state);
        switch (state) {
            case link_prober::LinkProberState::Label::PeerActive: {
                enterPeerMuxState(mux_state::MuxState::Active);
                break;
            }
            case link_prober::LinkProberState::Label::PeerUnknown: {
                if (mLabel == Label::Healthy) {
                    switchPeerMuxState(mux_state::MuxState::Label::Standby);
                }
                break;
            }
            default: {
                break;
            }
        }
    }
}

/*--------------------------------------------------------------------------------------------------------------
 |  state transition functions
 ---------------------------------------------------------------------------------------------------------------*/

//
// ---> initializeTransitionFunctionTable();
//
// initialize transition function table
//
void ActiveActiveStateMachine::initializeTransitionFunctionTable()
{
    MUXLOGWARNING("Initializing State Transition Table...");
    LinkManagerStateMachineBase::initializeTransitionFunctionTable();

    mStateTransitionHandler[link_prober::LinkProberState::Label::Active]
                           [mux_state::MuxState::Label::Active]
                           [link_state::LinkState::Label::Up] =
                               boost::bind(
                                   &ActiveActiveStateMachine::LinkProberActiveMuxActiveLinkUpTransitionFunction,
                                   this,
                                   boost::placeholders::_1
                               );

    mStateTransitionHandler[link_prober::LinkProberState::Label::Active]
                           [mux_state::MuxState::Label::Standby]
                           [link_state::LinkState::Label::Up] =
                               boost::bind(
                                   &ActiveActiveStateMachine::LinkProberActiveMuxStandbyLinkUpTransitionFunction,
                                   this,
                                   boost::placeholders::_1
                               );

    mStateTransitionHandler[link_prober::LinkProberState::Label::Active]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Up] =
                               boost::bind(
                                   &ActiveActiveStateMachine::LinkProberActiveMuxUnknownLinkUpTransitionFunction,
                                   this,
                                   boost::placeholders::_1
                               );

    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Active]
                           [link_state::LinkState::Label::Up] =
                               boost::bind(
                                   &ActiveActiveStateMachine::LinkProberUnknownMuxActiveLinkUpTransitionFunction,
                                   this,
                                   boost::placeholders::_1
                               );

    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Standby]
                           [link_state::LinkState::Label::Up] =
                               boost::bind(
                                   &ActiveActiveStateMachine::LinkProberUnknownMuxStandbyLinkUpTransitionFunction,
                                   this,
                                   boost::placeholders::_1
                               );

    mStateTransitionHandler[link_prober::LinkProberState::Label::Active]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Up] =
                               boost::bind(
                                   &ActiveActiveStateMachine::LinkProberActiveMuxUnknownLinkUpTransitionFunction,
                                   this,
                                   boost::placeholders::_1
                               );

    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Unknown]
                           [link_state::LinkState::Label::Up] =
                               boost::bind(
                                   &ActiveActiveStateMachine::LinkProberUnknownMuxUnknownLinkUpTransitionFunction,
                                   this,
                                   boost::placeholders::_1
                               );

    mStateTransitionHandler[link_prober::LinkProberState::Label::Active]
                           [mux_state::MuxState::Label::Error]
                           [link_state::LinkState::Label::Up] =
                               boost::bind(
                                   &ActiveActiveStateMachine::LinkProberActiveMuxErrorLinkUpTransitionFunction,
                                   this,
                                   boost::placeholders::_1
                               );

    mStateTransitionHandler[link_prober::LinkProberState::Label::Active]
                           [mux_state::MuxState::Label::Wait]
                           [link_state::LinkState::Label::Up] =
                               boost::bind(
                                   &ActiveActiveStateMachine::LinkProberActiveMuxWaitLinkUpTransitionFunction,
                                   this,
                                   boost::placeholders::_1
                               );

    mStateTransitionHandler[link_prober::LinkProberState::Label::Unknown]
                           [mux_state::MuxState::Label::Wait]
                           [link_state::LinkState::Label::Up] =
                               boost::bind(
                                   &ActiveActiveStateMachine::LinkProberUnknownMuxWaitLinkUpTransitionFunction,
                                   this,
                                   boost::placeholders::_1
                               );
}

//
// ---> LinkProberActiveMuxActiveLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberActive, MuxActive, LinkUp} state
//
void ActiveActiveStateMachine::LinkProberActiveMuxActiveLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    if (ms(mCompositeState) == mux_state::MuxState::Unknown) {
        switchMuxState(nextState, mux_state::MuxState::Label::Active);
    }
}

//
// ---> LinkProberActiveMuxStandbyLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberActive, MuxStandby, LinkUp} state
//
void ActiveActiveStateMachine::LinkProberActiveMuxStandbyLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    switchMuxState(nextState, mux_state::MuxState::Label::Active);
}

//
// ---> LinkProberUnknownMuxActiveLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxActive, LinkUp} state
//
void ActiveActiveStateMachine::LinkProberUnknownMuxActiveLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    if (mMuxPortConfig.getMode() == common::MuxPortConfig::Mode::Active && mLastMuxStateNotification != mux_state::MuxState::Label::Active) {
        // last switch mux state to active failed, try again
        switchMuxState(nextState, mux_state::MuxState::Label::Active, true);
    } else {
        switchMuxState(nextState, mux_state::MuxState::Label::Standby);
    }
}

//
// ---> LinkProberUnknownMuxStandbyLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxStandby, LinkUp} state
//
void ActiveActiveStateMachine::LinkProberUnknownMuxStandbyLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    if (mMuxPortConfig.getMode() == common::MuxPortConfig::Mode::Active && mLastMuxStateNotification != mux_state::MuxState::Label::Active) {
        // last switch mux state to active failed, try again
        switchMuxState(nextState, mux_state::MuxState::Label::Active, true);
    }
}


//
// ---> LinkProberActiveMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberActive, MuxUnknown, LinkUp} state
//
void ActiveActiveStateMachine::LinkProberActiveMuxUnknownLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    if (ps(mCompositeState) != link_prober::LinkProberState::Active) {
        switchMuxState(nextState, mux_state::MuxState::Label::Active);
    } else {
        startMuxProbeTimer();
    }
}

//
// ---> LinkProberUnknownMuxUnknownLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxUnknown, LinkUp} state
//
void ActiveActiveStateMachine::LinkProberUnknownMuxUnknownLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    if (ps(mCompositeState) != link_prober::LinkProberState::Unknown) {
        switchMuxState(nextState, mux_state::MuxState::Label::Standby);
    } else {
        startMuxProbeTimer();
    }
}

//
// ---> LinkProberActiveMuxErrorLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberActive, MuxError, LinkUp} state
//
void ActiveActiveStateMachine::LinkProberActiveMuxErrorLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    startMuxProbeTimer();
}

//
// ---> LinkProberActiveMuxWaitLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberActive, MuxWait, LinkUp} state
//
void ActiveActiveStateMachine::LinkProberActiveMuxWaitLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    switchMuxState(nextState, mux_state::MuxState::Label::Active);
}

//
// ---> LinkProberUnknownMuxWaitLinkUpTransitionFunction(CompositeState &nextState);
//
// transition function when entering {LinkProberUnknown, MuxWait, LinkUp} state
//
void ActiveActiveStateMachine::LinkProberUnknownMuxWaitLinkUpTransitionFunction(CompositeState &nextState)
{
    MUXLOGINFO(mMuxPortConfig.getPortName());
    switchMuxState(nextState, mux_state::MuxState::Label::Standby);
}

/*--------------------------------------------------------------------------------------------------------------
 |  utility methods to check/modify state
 ---------------------------------------------------------------------------------------------------------------*/

//
// ---> setLabel(Label label);
//
// sets linkmgr State db state
//
void ActiveActiveStateMachine::setLabel(Label label)
{
    if (mLabel != label) {
        mLabel = label;
        mMuxPortPtr->setMuxLinkmgrState(label);

        MUXLOGWARNING(
            boost::format("%s: Linkmgrd state is: %s %s") %
            mMuxPortConfig.getPortName() %
            mMuxStateName[ms(mCompositeState)] %
            mLinkHealthName[static_cast<int>(label)]
        );
    }
}

//
// ---> enterLinkProberState(CompositeState &nextState, link_prober::LinkProberState::Label label);
//
// force LinkProberState to switch state
//
void ActiveActiveStateMachine::enterLinkProberState(
    CompositeState &nextState,
    link_prober::LinkProberState::Label label
)
{
    mLinkProberStateMachinePtr->enterState(label);
    ps(nextState) = label;
}

//
// ---> enterMuxState(CompositeState &nextState, mux_state::MuxState::Label label);
//
// force MuxState to switch state
//
void ActiveActiveStateMachine::enterMuxState(
    CompositeState &nextState,
    mux_state::MuxState::Label label
)
{
    mMuxStateMachine.enterState(label);
    ms(nextState) = label;
}

//
// ---> enterLinkState(CompositeState &nextState, link_state::LinkState::Label label);
//
// force LinkState to switch state
//
void ActiveActiveStateMachine::enterLinkState(
    CompositeState &nextState,
    link_state::LinkState::Label label
)
{
    mLinkStateMachine.enterState(label);
    ls(nextState) = label;
}

//
// ---> enterPeerMuxState(mux_state::MuxState::Label label);
//
// force peer MuxState to switch state
//
void ActiveActiveStateMachine::enterPeerMuxState(mux_state::MuxState::Label label)
{
    mPeerMuxState = label;
}

//
// ---> enterPeerLinkProberState(link_prober::LinkProberState::Label label);
//
// force peer LinkProberState to switch state
//
void ActiveActiveStateMachine::enterPeerLinkProberState(link_prober::LinkProberState::Label label)
{
    mLinkProberStateMachinePtr->enterPeerState(label);
    mPeerLinkProberState = label;
}

//
// ---> switchMuxState(CompositeState &nextState, mux_state::MuxState::Label label, bool forceSwitch);
//
// switch MUX to the target state
//
void ActiveActiveStateMachine::switchMuxState(
    CompositeState &nextState,
    mux_state::MuxState::Label label,
    bool forceSwitch
)
{
    if (forceSwitch ||
        mMuxPortConfig.getMode() == common::MuxPortConfig::Mode::Auto ||
        mMuxPortConfig.getMode() == common::MuxPortConfig::Mode::Detached) {
        MUXLOGWARNING(
            boost::format("%s: Switching MUX state to '%s'") %
            mMuxPortConfig.getPortName() %
            mMuxStateName[label]
        );
        if (label == mux_state::MuxState::Label::Standby) {
            // suspend heartbeat to help peer ToR to toggle
            mSuspendTxFnPtr(mMuxPortConfig.getLinkWaitTimeout_msec());
        }
        enterMuxState(nextState, label);
        mMuxStateMachine.setWaitStateCause(mux_state::WaitState::WaitStateCause::SwssUpdate);
        mMuxPortPtr->setMuxState(label);
        mDeadlineTimer.cancel();
        startMuxWaitTimer();
    } else {
        probeMuxState();
    }
}

//
// ---> switchPeerMuxState(CompositeState &nextState, mux_state::MuxState::Label label, bool forceSwitch);
//
// switch peer MUX to the target state
//
void ActiveActiveStateMachine::switchPeerMuxState(mux_state::MuxState::Label label)
{
    if (mMuxPortConfig.getMode() == common::MuxPortConfig::Mode::Auto) {
        MUXLOGWARNING(
            boost::format("%s: Switching peer MUX state to '%s'") %
            mMuxPortConfig.getPortName() %
            mMuxStateName[label]
        );
        enterPeerMuxState(label);
        mMuxPortPtr->setPeerMuxState(label);
        startPeerMuxWaitTimer();
    }
}

//
// ---> probeMuxState();
//
// probe mux state
//
void ActiveActiveStateMachine::probeMuxState()
{
    mMuxStateMachine.setWaitStateCause(mux_state::WaitState::WaitStateCause::DriverUpdate);
    mMuxPortPtr->probeMuxState();
    startMuxWaitTimer();
}

//
// ---> updateMuxLinkmgrState();
//
// update State DB MUX LinkMgr state
//
void ActiveActiveStateMachine::updateMuxLinkmgrState()
{
    Label label = Label::Unhealthy;
    if (ls(mCompositeState) == link_state::LinkState::Label::Up && ps(mCompositeState) == link_prober::LinkProberState::Label::Active && ms(mCompositeState) == mux_state::MuxState::Label::Active) {
        label = Label::Healthy;
    }
    setLabel(label);
}

//
// ---> initLinkProberState(CompositeState &compositeState);
//
// initialize LinkProberState when configuring the composite state machine
//
void ActiveActiveStateMachine::initLinkProberState(CompositeState &compositeState)
{
    switch (ms(compositeState)) {
        case mux_state::MuxState::Label::Active:
            enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Active);
            break;
        case mux_state::MuxState::Label::Standby:
            enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Unknown);
            break;
        case mux_state::MuxState::Label::Unknown:
            enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Wait);
            break;
        case mux_state::MuxState::Label::Error:
            enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Wait);
            break;
        case mux_state::MuxState::Label::Wait:
            enterLinkProberState(compositeState, link_prober::LinkProberState::Label::Wait);
            break;
        default:
            break;
    }
}

//
// ---> initPeerLinkProberState();
//
// initialize peer LinkProberState when configuring the composite state machine
//
void ActiveActiveStateMachine::initPeerLinkProberState()
{
    switch (mPeerMuxState) {
        case mux_state::MuxState::Label::Active:
            enterPeerLinkProberState(link_prober::LinkProberState::Label::PeerActive);
            break;
        case mux_state::MuxState::Label::Standby:
            enterPeerLinkProberState(link_prober::LinkProberState::Label::PeerUnknown);
            break;
        case mux_state::MuxState::Label::Unknown:
            enterPeerLinkProberState(link_prober::LinkProberState::Label::PeerWait);
            break;
        case mux_state::MuxState::Label::Error:
            enterPeerLinkProberState(link_prober::LinkProberState::Label::PeerWait);
            break;
        case mux_state::MuxState::Label::Wait:
            enterPeerLinkProberState(link_prober::LinkProberState::Label::PeerWait);
            break;
        default:
            break;
    }
}

//
// ---> startMuxProbeTimer();
//
// start a mux probe and wait for mux probe notification(active or standby) from xcvrd
//
void ActiveActiveStateMachine::startMuxProbeTimer()
{
    probeMuxState();
    mDeadlineTimer.expires_from_now(boost::posix_time::milliseconds(
        mMuxProbeBackoffFactor * mMuxPortConfig.getNegativeStateChangeRetryCount() * mMuxPortConfig.getTimeoutIpv4_msec()
    ));
    mDeadlineTimer.async_wait(getStrand().wrap(boost::bind(
        &ActiveActiveStateMachine::handleMuxProbeTimeout,
        this,
        boost::asio::placeholders::error
    )));
}

//
// ---> handleMuxProbeTimeout(boost::system::error_code errorCode);
//
// handles when xcvrd has timeout responding mux probe
//
void ActiveActiveStateMachine::handleMuxProbeTimeout(boost::system::error_code errorCode)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());
    if (errorCode == boost::system::errc::success) {
        if (ms(mCompositeState) == mux_state::MuxState::Label::Unknown ||
            ms(mCompositeState) == mux_state::MuxState::Label::Error ||
            ms(mCompositeState) == mux_state::MuxState::Label::Wait) {
            mMuxProbeBackoffFactor <<= 1;
            mMuxProbeBackoffFactor = mMuxProbeBackoffFactor > MAX_BACKOFF_FACTOR ? MAX_BACKOFF_FACTOR : mMuxProbeBackoffFactor;
            startMuxProbeTimer();
        } else {
            mMuxProbeBackoffFactor = 1;
        }
    }
}

//
// ---> startMuxWaitTimer(uint32_t factor);
//
// start a timer to wait for mux state notification from xcvrd/orchagent
//
void ActiveActiveStateMachine::startMuxWaitTimer(uint32_t factor)
{
    mWaitTimer.expires_from_now(boost::posix_time::milliseconds(
        factor * mMuxPortConfig.getNegativeStateChangeRetryCount() * mMuxPortConfig.getTimeoutIpv4_msec()
    ));
    mWaitTimer.async_wait(getStrand().wrap(boost::bind(
        &ActiveActiveStateMachine::handleMuxWaitTimeout,
        this,
        boost::asio::placeholders::error
    )));
}

//
// ---> handleMuxWaitTimeout(boost::system::error_code errorCode);
//
// handle when xcrvrd/orchagent has timed out responding mux state
//
void ActiveActiveStateMachine::handleMuxWaitTimeout(boost::system::error_code errorCode)
{
    if (errorCode == boost::system::errc::success) {
        if (mMuxStateMachine.getWaitStateCause() == mux_state::WaitState::WaitStateCause::SwssUpdate) {
            MUXLOGTIMEOUT(mMuxPortConfig.getPortName(), "orchagent timed out responding to linkmgrd", mCompositeState);
        } else if (mMuxStateMachine.getWaitStateCause() == mux_state::WaitState::WaitStateCause::DriverUpdate) {
            MUXLOGTIMEOUT(mMuxPortConfig.getPortName(), "xcvrd timed out responding to linkmgrd", mCompositeState);
        } else {
            MUXLOGTIMEOUT(mMuxPortConfig.getPortName(), "Unknown timeout reason!!!", mCompositeState);
        }
        mMuxWaitBackoffFactor <<= 1;
        mMuxWaitBackoffFactor = mMuxWaitBackoffFactor > MAX_BACKOFF_FACTOR ? MAX_BACKOFF_FACTOR : mMuxWaitBackoffFactor;
        startMuxWaitTimer(mMuxWaitBackoffFactor);
    } else {
        mMuxWaitBackoffFactor = 1;
    }
}

//
// ---> startPeerMuxWaitTimer(uint32_t factor);
//
// start a timer to wait for peer mux state notification from xcvrd/orchagent
//
void ActiveActiveStateMachine::startPeerMuxWaitTimer(uint32_t factor)
{
    mPeerWaitTimer.expires_from_now(boost::posix_time::milliseconds(
        factor * mMuxPortConfig.getNegativeStateChangeRetryCount() * mMuxPortConfig.getTimeoutIpv4_msec()
    ));
    mPeerWaitTimer.async_wait(getStrand().wrap(boost::bind(
        &ActiveActiveStateMachine::handlePeerMuxWaitTimeout,
        this,
        boost::asio::placeholders::error
    )));
}

//
// ---> handlePeerMuxWaitTimeout(boost::system::error_code errorCode);
//
// handle when xcrvrd/orchagent has timed out responding peer mux state
//
void ActiveActiveStateMachine::handlePeerMuxWaitTimeout(boost::system::error_code errorCode)
{
    if (errorCode == boost::system::errc::success) {
        MUXLOGWARNING(
            boost::format("%s: %s, current peer mux state: %s") %
            mMuxPortConfig.getPortName() %
            "xcvrd timed out responding to linkmgrd peer mux state" %
            mMuxStateName[mPeerMuxState]
        );
        mPeerMuxWaitBackoffFactor <<= 1;
        mPeerMuxWaitBackoffFactor = mPeerMuxWaitBackoffFactor > MAX_BACKOFF_FACTOR ? MAX_BACKOFF_FACTOR : mPeerMuxWaitBackoffFactor;
        startPeerMuxWaitTimer(mPeerMuxWaitBackoffFactor);
    } else {
        mPeerMuxWaitBackoffFactor = 1;
    }
}

//
// ---> handleDefaultRouteStateNotification(const DefaultRoute routeState);
//
// handle default route state notification from routeorch
//
void ActiveActiveStateMachine::handleDefaultRouteStateNotification(const DefaultRoute routeState)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    mDefaultRouteState = routeState;
    shutdownOrRestartLinkProberOnDefaultRoute();
}

//
//
// ---> shutdownOrRestartLinkProberOnDefaultRoute();
//
// shutdown or restart link prober based on default route state
//
void ActiveActiveStateMachine::shutdownOrRestartLinkProberOnDefaultRoute()
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

} /* namespace link_manager */
