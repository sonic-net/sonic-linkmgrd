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
 * LinkManagerStateMachineTest.cpp
 *
 *  Created on: Oct 25, 2020
 *      Author: Tamer Ahmed
 */

#include "LinkManagerStateMachineTest.h"
#include "link_prober/LinkProberStateMachineBase.h"

#define VALIDATE_STATE(p, m, l) \
    do { \
        mTestCompositeState = mFakeMuxPort.getCompositeState(); \
        EXPECT_EQ(ps(mTestCompositeState), link_prober::LinkProberState::Label::p); \
        EXPECT_EQ(ms(mTestCompositeState), mux_state::MuxState::Label::m); \
        EXPECT_EQ(ls(mTestCompositeState), link_state::LinkState::Label::l); \
    } while (0)

namespace test
{

LinkManagerStateMachineTest::LinkManagerStateMachineTest() :
    mDbInterfacePtr(std::make_shared<FakeDbInterface> (&mIoService)),
    mFakeMuxPort(
        mDbInterfacePtr,
        mMuxConfig,
        mPortName,
        mServerId,
        mIoService
    )
{
    mMuxConfig.setTimeoutIpv4_msec(10);
    mMuxConfig.setPositiveStateChangeRetryCount(mPositiveUpdateCount);
    mMuxConfig.setMuxStateChangeRetryCount(mPositiveUpdateCount);
    mMuxConfig.setLinkStateChangeRetryCount(mPositiveUpdateCount);
}

void LinkManagerStateMachineTest::runIoService(uint32_t count)
{
    if (count == 0) {
        if(mIoService.stopped()) {
            mIoService.restart();
        }
        mIoService.run();
    }

    for (uint8_t i = 0; i < count; i++) {
        if(mIoService.stopped()) {
            mIoService.restart();
        }
        mIoService.run_one();
    }
}

void LinkManagerStateMachineTest::postLinkProberEvent(link_prober::LinkProberState::Label label, uint32_t count, uint32_t detect_multiplier)
{
    switch (label) {
        case link_prober::LinkProberState::Active: {
            detect_multiplier = (detect_multiplier == 0 ? mMuxConfig.getPositiveStateChangeRetryCount() : detect_multiplier);
            for (uint8_t i = 0; i < detect_multiplier; i++) {
                mFakeMuxPort.mFakeLinkProber->postLinkProberEvent<link_prober::IcmpSelfEvent&>(
                    link_prober::LinkProberStateMachineBase::getIcmpSelfEvent()
                );
                runIoService(count);
            }
            break;
        }
        case link_prober::LinkProberState::Standby: {
            detect_multiplier = (detect_multiplier == 0 ? mMuxConfig.getPositiveStateChangeRetryCount() : detect_multiplier);
            for (uint8_t i = 0; i < detect_multiplier; i++) {
                mFakeMuxPort.mFakeLinkProber->postLinkProberEvent<link_prober::IcmpPeerEvent&>(
                    link_prober::LinkProberStateMachineBase::getIcmpPeerEvent()
                );
                runIoService(count);
            }
            break;
        }
        case link_prober::LinkProberState::Unknown: {
            detect_multiplier = (detect_multiplier == 0 ? mMuxConfig.getNegativeStateChangeRetryCount() : detect_multiplier);
            for (uint8_t i = 0; i < detect_multiplier; i++) {
                mFakeMuxPort.mFakeLinkProber->postLinkProberEvent<link_prober::IcmpUnknownEvent&>(
                    link_prober::LinkProberStateMachineBase::getIcmpUnknownEvent()
                );
                runIoService(count);
            }
            break;
        }
        default: {
            break;
        }
    }
}

void LinkManagerStateMachineTest::postMuxEvent(mux_state::MuxState::Label label, uint32_t count)
{
    mux_state::MuxStateMachine& muxStateMachine = mFakeMuxPort.getMuxStateMachine();
    for (uint8_t i = 0; i < mMuxConfig.getMuxStateChangeRetryCount(); i++) {
        switch (label) {
        case mux_state::MuxState::Active:
            muxStateMachine.postMuxStateEvent(mux_state::MuxStateMachine::getActiveEvent());
            break;
        case mux_state::MuxState::Standby:
            muxStateMachine.postMuxStateEvent(mux_state::MuxStateMachine::getStandbyEvent());
            break;
        case mux_state::MuxState::Unknown:
            muxStateMachine.postMuxStateEvent(mux_state::MuxStateMachine::getUnknownEvent());
            break;
        case mux_state::MuxState::Error:
            muxStateMachine.postMuxStateEvent(mux_state::MuxStateMachine::getErrorEvent());
            break;
        default:
            break;
        }
        runIoService(count);
    }
}

void LinkManagerStateMachineTest::postLinkEvent(link_state::LinkState::Label label, uint32_t count)
{
    link_state::LinkStateMachine& linkStateMachine = mFakeMuxPort.getLinkStateMachine();
    for (uint8_t i = 0; i < mMuxConfig.getLinkStateChangeRetryCount(); i++) {
        switch (label) {
        case link_state::LinkState::Up:
            linkStateMachine.postLinkStateEvent(link_state::LinkStateMachine::getUpEvent());
            break;
        case link_state::LinkState::Down:
            linkStateMachine.postLinkStateEvent(link_state::LinkStateMachine::getDownEvent());
            break;
        default:
            break;
        }
        runIoService(count);
    }
}

void LinkManagerStateMachineTest::postSuspendTimerExpiredEvent(uint32_t count)
{
    mFakeMuxPort.mFakeLinkProber->postSuspendTimerExpiredEvent();
    runIoService(count);
}

void LinkManagerStateMachineTest::handleMuxState(std::string state, uint32_t count)
{
    for (uint8_t i = 0; i < mPositiveUpdateCount; i++) {
        mFakeMuxPort.handleMuxState(state);
        runIoService(count);
    }
}

void LinkManagerStateMachineTest::handleGetMuxState(std::string state, uint32_t count)
{
    mFakeMuxPort.handleGetMuxState(state);
    runIoService(count);
}

void LinkManagerStateMachineTest::handleProbeMuxState(std::string state, uint32_t count)
{
    for (uint8_t i = 0; i < mPositiveUpdateCount; i++) {
        mFakeMuxPort.handleProbeMuxState(state);
        runIoService(count);
    }
}

void LinkManagerStateMachineTest::handleLinkState(std::string linkState, uint32_t count)
{
    for (uint8_t i = 0; i < mMuxConfig.getLinkStateChangeRetryCount(); i++) {
        mFakeMuxPort.handleLinkState(linkState);
        runIoService(count);
    }
}

void LinkManagerStateMachineTest::handleMuxConfig(std::string config, uint32_t count)
{
    mFakeMuxPort.handleMuxConfig(config);
    runIoService(count);
}

void LinkManagerStateMachineTest::activateStateMachine()
{
    mFakeMuxPort.activateStateMachine();
    mFakeMuxPort.handleDefaultRouteState("ok");
}

void LinkManagerStateMachineTest::setMuxActive()
{
    activateStateMachine();
    VALIDATE_STATE(Unknown, Wait, Down);

    postLinkEvent(link_state::LinkState::Up);
    VALIDATE_STATE(Wait, Wait, Up);

    // change state to active
    postLinkProberEvent(link_prober::LinkProberState::Active);
    VALIDATE_STATE(Active, Wait, Up);

    // change state to active
    postLinkProberEvent(link_prober::LinkProberState::Active);
    VALIDATE_STATE(Active, Wait, Up);

    // switch mux to active state
    postMuxEvent(mux_state::MuxState::Active);
    VALIDATE_STATE(Active, Active, Up);
}

void LinkManagerStateMachineTest::setMuxStandby()
{
    activateStateMachine();
    VALIDATE_STATE(Unknown, Wait, Down);

    postLinkEvent(link_state::LinkState::Down);
    VALIDATE_STATE(Unknown, Wait, Down);

    postLinkEvent(link_state::LinkState::Up);
    VALIDATE_STATE(Wait, Wait, Up);

    // change state to active
    postLinkProberEvent(link_prober::LinkProberState::Standby);
    VALIDATE_STATE(Standby, Wait, Up);

    // switch mux to active state
    postMuxEvent(mux_state::MuxState::Standby);
    VALIDATE_STATE(Standby, Standby, Up);
}

void LinkManagerStateMachineTest::postDefaultRouteEvent(std::string routeState, uint32_t count)
{
    mFakeMuxPort.handleDefaultRouteState(routeState);
    runIoService(count);
}

void LinkManagerStateMachineTest::postPeerLinkStateEvent(std::string linkState, uint32_t count)
{
    mFakeMuxPort.handlePeerLinkState(linkState);
    runIoService(count);
}

void LinkManagerStateMachineTest::postPckLossRatioUpdateEvent(uint64_t unknownCount, uint64_t totalCount)
{
    mFakeMuxPort.postPckLossRatio(unknownCount, totalCount);
    mFakeMuxPort.mFakeLinkProber->mIcmpUnknownEventCount = unknownCount;
    mFakeMuxPort.mFakeLinkProber->mIcmpPacketCount = totalCount;

    runIoService();
}

void LinkManagerStateMachineTest::postPckLossCountsResetEvent()
{
    mFakeMuxPort.mFakeLinkProber->resetIcmpPacketCounts();

    runIoService(2);
}

TEST_F(LinkManagerStateMachineTest, MuxActiveSwitchOver)
{
    setMuxActive();

    // verify MUX enters wait state and that the diver is being probed
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    postLinkProberEvent(link_prober::LinkProberState::Standby, 2);
    VALIDATE_STATE(Standby, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 1);

    // fake mux statedb state to be active
    mDbInterfacePtr->setNextMuxState(mux_state::MuxState::Active);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    // driver notification
    handleProbeMuxState("standby", 4);
    VALIDATE_STATE(Standby, Standby, Up);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 2);

    // get state db mux state
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    handleGetMuxState("active", 2);
    VALIDATE_STATE(Standby, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);

    // swss notification
    handleMuxState("standby", 3);
    VALIDATE_STATE(Standby, Standby, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxActiveRemoteSwitchOver)
{
    setMuxActive();

    // swss notification
    handleMuxState("active", 3);
    VALIDATE_STATE(Active, Active, Up);

    // verify MUX enters wait state and that the diver is being probed
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    VALIDATE_STATE(Unknown, Active, Up);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);

    // fake mux statedb state to be active
    mDbInterfacePtr->setNextMuxState(mux_state::MuxState::Active);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    // driver notification
    handleProbeMuxState("standby", 3);
    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);

    // get state db mux state
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);
    handleGetMuxState("active", 2);
    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);

    postLinkProberEvent(link_prober::LinkProberState::Standby, 2);
    VALIDATE_STATE(Standby, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);

    // swss notification
    handleMuxState("standby", 3);
    VALIDATE_STATE(Standby, Standby, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyRemoteSwitchOver)
{
    setMuxStandby();

    // swss notification
    handleMuxState("standby", 3);
    VALIDATE_STATE(Standby, Standby, Up);

    handleLinkState("down");
    VALIDATE_STATE(Standby, Standby, Down);

    handleMuxState("active", 3);
    VALIDATE_STATE(Standby, Active, Down);

    handleMuxState("standby", 3);
    VALIDATE_STATE(Standby, Standby, Down);

    // verify MUX enters wait state and that the diver is being probed
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 3);
    postLinkProberEvent(link_prober::LinkProberState::Unknown, 3);
    VALIDATE_STATE(Unknown, Wait, Down);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 1);

    // fake mux statedb state to be active
    mDbInterfacePtr->setNextMuxState(mux_state::MuxState::Standby);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 3);
    // driver notification
    handleProbeMuxState("active", 4);
    VALIDATE_STATE(Unknown, Wait, Down);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 4);

    postLinkProberEvent(link_prober::LinkProberState::Active, 2);
    VALIDATE_STATE(Active, Wait, Down);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 2);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbySwitchOver)
{
    setMuxStandby();

    // Verify posting extra standby won't change link prober state
    postLinkProberEvent(link_prober::LinkProberState::Standby, 2);
    VALIDATE_STATE(Standby, Standby, Up);

    // verify MUX enters wait state and that the diver is being probed
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    postLinkProberEvent(link_prober::LinkProberState::Active, 2);
    VALIDATE_STATE(Active, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 1);

    // fake mux statedb state to be standby
    mDbInterfacePtr->setNextMuxState(mux_state::MuxState::Standby);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    // driver notification
    handleProbeMuxState("active", 3);
    VALIDATE_STATE(Active, Active, Up);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 2);

    // get state db mux state
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    handleGetMuxState("standby", 2);
    VALIDATE_STATE(Active, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);

    // swss notification
    handleMuxState("active", 3);
    VALIDATE_STATE(Active, Active, Up);

    postLinkEvent(link_state::LinkState::Up);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxActiveCliSwitchOver)
{
    setMuxActive();

    handleMuxConfig("active");
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxActiveCliAuto)
{
    setMuxActive();

    handleMuxConfig("auto");
    VALIDATE_STATE(Active, Active, Up);

    runIoService(2);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
}

TEST_F(LinkManagerStateMachineTest, MuxAStandbyCliAuto)
{
    setMuxStandby();

    handleMuxConfig("auto");
    VALIDATE_STATE(Standby, Standby, Up);

    runIoService(2);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
}

TEST_F(LinkManagerStateMachineTest, MuxActiveCliManual)
{
    setMuxActive();

    handleMuxConfig("manual");
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxAStandbyCliManual)
{
    setMuxStandby();

    handleMuxConfig("manual");
    VALIDATE_STATE(Standby, Standby, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxActiveCliStandby)
{
    setMuxActive();

    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mSendPeerSwitchCommand, 0);
    handleMuxConfig("standby");
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mSendPeerSwitchCommand, 1);

    mFakeMuxPort.mFakeLinkProber->handleSendSwitchCommand();
    runIoService(2);
    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mPostSwitchCauseInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mLastPostedSwitchCause, link_manager::ActiveStandbyStateMachine::SwitchCause::ConfigMuxMode);

    // swss notification
    handleMuxState("standby", 3);
    VALIDATE_STATE(Wait, Standby, Up);

    // change state to active
    postLinkProberEvent(link_prober::LinkProberState::Standby, 2);
    VALIDATE_STATE(Standby, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("standby", 4);
    VALIDATE_STATE(Standby, Standby, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyRecvSwitchActiveTlv)
{
    setMuxStandby();

    mFakeMuxPort.mFakeLinkProber->handleSwitchCommandRecv();
    runIoService(2);
    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mPostSwitchCauseInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mLastPostedSwitchCause, link_manager::ActiveStandbyStateMachine::SwitchCause::TlvSwitchActiveCommand);

    // swss notification
    handleMuxState("active", 3);
    VALIDATE_STATE(Wait, Active, Up);

    // change state to active
    postLinkProberEvent(link_prober::LinkProberState::Active, 2);
    VALIDATE_STATE(Active, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("active", 4);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyCliSwitchOverMuxFirst)
{
    setMuxStandby();

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    handleMuxConfig("active", 2);

    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);

    // swss notification
    handleMuxState("active", 3);
    VALIDATE_STATE(Wait, Active, Up);

    // change state to active
    postLinkProberEvent(link_prober::LinkProberState::Active, 2);
    VALIDATE_STATE(Active, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("active", 4);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyCliSwitchOverLinkProberFirst)
{
    setMuxStandby();

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    handleMuxConfig("active", 4);

    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);

    // change state to active
    postLinkProberEvent(link_prober::LinkProberState::Active, 3);
    VALIDATE_STATE(Active, Wait, Up);

    // swss notification
    handleMuxState("active", 3);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxActiveLinkDown)
{
    setMuxActive();

    int postLinkProberMetricsInvokeCountBefore = mDbInterfacePtr->mPostLinkProberMetricsInvokeCount;

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    handleLinkState("down", 3);

    VALIDATE_STATE(Active, Wait, Down);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mPostLinkProberMetricsInvokeCount, postLinkProberMetricsInvokeCountBefore);
    EXPECT_EQ(mDbInterfacePtr->mLastPostedSwitchCause, link_manager::ActiveStandbyStateMachine::SwitchCause::LinkDown);

    // swss notification
    handleMuxState("standby", 4);
    VALIDATE_STATE(Active, Standby, Down);

    handleLinkState("up");
    VALIDATE_STATE(Standby, Standby, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyLinkDown)
{
    setMuxStandby();

    handleLinkState("down", 3);
    VALIDATE_STATE(Standby, Standby, Down);

    handleLinkState("up");
    VALIDATE_STATE(Standby, Standby, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyLinkDownMuxStandbyCliActiveCliAuto)
{
    setMuxStandby();
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);

    handleLinkState("down", 3);
    VALIDATE_STATE(Standby, Standby, Down);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);

    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    VALIDATE_STATE(Unknown, Standby, Down);

    runIoService(1);
    VALIDATE_STATE(Unknown, Wait, Down);

    handleProbeMuxState("standby", 3);
    VALIDATE_STATE(Unknown, Standby, Down);

    handleMuxConfig("active", 2);
    VALIDATE_STATE(Wait, Wait, Down);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);

    handleMuxState("active", 3);
    VALIDATE_STATE(Wait, Active, Down);

    handleMuxConfig("auto", 2);
    VALIDATE_STATE(Wait, Wait, Down);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 2);

    handleMuxState("standby", 3);
    VALIDATE_STATE(Wait, Standby, Down);
}

TEST_F(LinkManagerStateMachineTest, MuxActiveLinkProberUnknownPeerOvertakeLink)
{
    setMuxActive();

    // verify MUX enters wait state and that the diver is being probed
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    VALIDATE_STATE(Unknown, Active, Up);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mSuspendTxProbeCallCount, 1);

    postSuspendTimerExpiredEvent(2);
    VALIDATE_STATE(Unknown, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 1);

    // Verify posting extra unknown won't change link prober state
    postLinkProberEvent(link_prober::LinkProberState::Unknown, 3);
    VALIDATE_STATE(Unknown, Wait, Up);

    postLinkProberEvent(link_prober::LinkProberState::Standby, 2);
    VALIDATE_STATE(Standby, Wait, Up);

    handleProbeMuxState("standby", 3);
    VALIDATE_STATE(Standby, Standby, Up);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 2);
}

TEST_F(LinkManagerStateMachineTest, MuxActiveLinkProberUnknownReturnActive)
{
    setMuxActive();

    // verify MUX enters wait state and that the diver is being probed
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    VALIDATE_STATE(Unknown, Active, Up);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mSuspendTxProbeCallCount, 1);

    postSuspendTimerExpiredEvent(2);
    VALIDATE_STATE(Unknown, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 1);

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    handleProbeMuxState("standby", 3);
    VALIDATE_STATE(Wait, Wait, Up);

    // ensure link prober stays in wait state if unknown event was posted
    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    VALIDATE_STATE(Wait, Wait, Up);

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);
    // swss notification
    handleMuxState("active", 3);
    VALIDATE_STATE(Wait, Active, Up);

    // change state to active
    postLinkProberEvent(link_prober::LinkProberState::Active, 2);
    VALIDATE_STATE(Active, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("active", 4);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyLinkProberUnknown)
{
    setMuxStandby();

    // verify MUX enters wait state and that the diver is being probed
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mPostSwitchCauseInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mLastPostedSwitchCause, link_manager::ActiveStandbyStateMachine::SwitchCause::PeerHeartbeatMissing);

    // swss notification
    handleMuxState("active", 3);
    VALIDATE_STATE(Wait, Active, Up);

    // change state to active
    postLinkProberEvent(link_prober::LinkProberState::Active, 2);
    VALIDATE_STATE(Active, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("active", 4);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyLinkProberUnknownCliSwitchover)
{
    setMuxStandby();
    
    // verify MUX enters wait state
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1); 

    // mux mode config change is cached due to MuxState:Wait
    handleMuxConfig("active", 1);
    EXPECT_TRUE(mFakeMuxPort.getPendingMuxModeChange());
    EXPECT_EQ(mFakeMuxPort.getTargetMuxMode(), common::MuxPortConfig::Mode::Active);

    // swss notification, exiting MuxState:wait
    handleMuxState("standby", 4);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 2);
    // execute pending mux mode config change, switch to MuxState:Active   
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 2);
    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_FALSE(mFakeMuxPort.getPendingMuxModeChange());

    // swss notification
    handleMuxState("active", 3);
    VALIDATE_STATE(Wait, Active, Up);

    // change state to active
    postLinkProberEvent(link_prober::LinkProberState::Active, 2);
    VALIDATE_STATE(Active, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("active", 4);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyLinkProberUnknownReturnStandby)
{
    setMuxStandby();

    // verify MUX enters wait state and that the diver is being probed
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);

    // swss notification
    handleMuxState("standby", 3);
    VALIDATE_STATE(Wait, Standby, Up);

    // change state to standby
    postLinkProberEvent(link_prober::LinkProberState::Standby, 2);
    VALIDATE_STATE(Standby, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("standby", 4);
    VALIDATE_STATE(Standby, Standby, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxActiveAsymetricLinkDrop)
{
    setMuxActive();

    // verify MUX enters wait state and that the diver is being probed
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    postLinkProberEvent(link_prober::LinkProberState::Unknown);
    VALIDATE_STATE(Unknown, Active, Up);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mSuspendTxProbeCallCount, 1);

    postSuspendTimerExpiredEvent(2);
    VALIDATE_STATE(Unknown, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 1);

    // Peer took over the link
    handleProbeMuxState("standby", 3);
    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);
    // swss notification
    handleMuxState("active", 3);
    VALIDATE_STATE(Wait, Active, Up);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 2);

    runIoService(2);
    VALIDATE_STATE(Wait, Wait, Up);

    handleProbeMuxState("active", 3);
    VALIDATE_STATE(Wait, Active, Up);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 3);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mSuspendTxProbeCallCount, 2);

    runIoService(2);
    VALIDATE_STATE(Wait, Wait, Up);

    handleProbeMuxState("standby", 3);
    VALIDATE_STATE(Wait, Standby, Up);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 4);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mSuspendTxProbeCallCount, 2);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyAsymetricLinkDrop)
{
    setMuxStandby();

    // verify MUX enters wait state and that the diver is being probed
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 1);
    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    VALIDATE_STATE(Wait, Wait, Up);

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);
    // swss notification
    handleMuxState("active", 3);
    VALIDATE_STATE(Wait, Active, Up);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 2);

    runIoService(2);
    VALIDATE_STATE(Wait, Wait, Up);

    handleProbeMuxState("active", 3);
    VALIDATE_STATE(Wait, Active, Up);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 3);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mSuspendTxProbeCallCount, 1);

    runIoService(2);
    VALIDATE_STATE(Wait, Wait, Up);

    handleProbeMuxState("standby", 3);
    VALIDATE_STATE(Wait, Standby, Up);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 4);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mSuspendTxProbeCallCount, 1);
}

TEST_F(LinkManagerStateMachineTest, ActiveStateToProberUnknownMuxUnknownLinkUp)
{
    setMuxActive();

    postMuxEvent(mux_state::MuxState::Unknown, 2);
    VALIDATE_STATE(Active, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Active, Wait, Up);

    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Unknown, Unknown, Up);

    runIoService(2);
    VALIDATE_STATE(Unknown, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Unknown, Unknown, Up);

    runIoService(2);
    VALIDATE_STATE(Unknown, Wait, Up);

    handleProbeMuxState("active", 3);
    VALIDATE_STATE(Unknown, Active, Up);
    EXPECT_EQ(mDbInterfacePtr->mGetMuxStateInvokeCount, 4);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mSuspendTxProbeCallCount, 1);

    postLinkProberEvent(link_prober::LinkProberState::Active, 2);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, StandbyStateToProberUnknownMuxUnknownLinkUp)
{
    setMuxStandby();

    postMuxEvent(mux_state::MuxState::Unknown, 3);
    VALIDATE_STATE(Standby, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("unknown", 4);
    VALIDATE_STATE(Standby, Wait, Up);

    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    VALIDATE_STATE(Unknown, Wait, Up);

    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Unknown, Unknown, Up);

    runIoService(2);
    VALIDATE_STATE(Unknown, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Unknown, Unknown, Up);

    runIoService(2);
    VALIDATE_STATE(Unknown, Wait, Up);

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    handleProbeMuxState("standby", 3);
    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);

    // swss notification
    handleMuxState("unknown", 3);
    VALIDATE_STATE(Wait, Unknown, Up);

    runIoService(2);
    VALIDATE_STATE(Wait, Wait, Up);

    postLinkProberEvent(link_prober::LinkProberState::Active, 2);
    VALIDATE_STATE(Active, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("active", 3);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, ProberUnknownMuxUnknownLinkDown)
{
    setMuxActive();

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    handleLinkState("down", 3);

    VALIDATE_STATE(Active, Wait, Down);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);

    // swss notification
    handleMuxState("unknown", 4);
    VALIDATE_STATE(Active, Unknown, Down);

    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    VALIDATE_STATE(Unknown, Unknown, Down);

    runIoService(2);
    VALIDATE_STATE(Unknown, Wait, Down);

    handleProbeMuxState("unknown", 4);
    VALIDATE_STATE(Unknown, Wait, Down);
}

TEST_F(LinkManagerStateMachineTest, ProberWaitMuxUnknownLinkDown)
{
    setMuxStandby();

    postMuxEvent(mux_state::MuxState::Unknown, 3);
    VALIDATE_STATE(Standby, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("unknown", 4);
    VALIDATE_STATE(Standby, Wait, Up);

    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Unknown, Unknown, Up);

    runIoService(2);
    VALIDATE_STATE(Unknown, Wait, Up);

    // xcvrd notification
    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Unknown, Unknown, Up);

    runIoService(2);
    VALIDATE_STATE(Unknown, Wait, Up);

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    handleProbeMuxState("standby", 3);
    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);

    // swss notification
    handleMuxState("unknown", 3);
    VALIDATE_STATE(Wait, Unknown, Up);

    runIoService(2);
    VALIDATE_STATE(Wait, Wait, Up);

    handleLinkState("down", 3);
    VALIDATE_STATE(Wait, Wait, Down);

    // xcvrd notification
    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Wait, Unknown, Down);

    runIoService(2);
    VALIDATE_STATE(Wait, Wait, Down);

}

TEST_F(LinkManagerStateMachineTest, MuxUnknownGetMuxStateStandby)
{
    setMuxActive();

    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Unknown, Unknown, Up);

    postLinkProberEvent(link_prober::LinkProberState::Standby, 3);
    VALIDATE_STATE(Standby, Wait, Up);

    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Unknown, Unknown, Up);

    // even get mux state "standby" in state db, which is mismatching from "unknown", LinkManager shouldn't switch to "unknown"
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    handleGetMuxState("standby", 2);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
}

TEST_F(LinkManagerStateMachineTest, MuxActive2Error2Active)
{
    setMuxActive();

    // fake mux statedb state to be active
    mDbInterfacePtr->setNextMuxState(mux_state::MuxState::Error);

    handleMuxState("error", 3);
    VALIDATE_STATE(Active, Error, Up);

    handleMuxState("error", 3);
    VALIDATE_STATE(Active, Error, Up);

    handleProbeMuxState("active", 3);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxActive2ErrorStandby)
{
    setMuxActive();

    handleMuxState("error", 3);
    VALIDATE_STATE(Active, Error, Up);

    handleMuxState("error", 3);
    VALIDATE_STATE(Active, Error, Up);

    postLinkProberEvent(link_prober::LinkProberState::Standby, 2);
    VALIDATE_STATE(Standby, Wait, Up);

    handleProbeMuxState("standby", 3);
    VALIDATE_STATE(Standby, Standby, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandby2Error2Standby)
{
    setMuxStandby();

    // fake mux statedb state to be active
    mDbInterfacePtr->setNextMuxState(mux_state::MuxState::Error);

    handleMuxState("error", 3);
    VALIDATE_STATE(Standby, Error, Up);

    handleMuxState("error", 3);
    VALIDATE_STATE(Standby, Error, Up);

    handleProbeMuxState("standby", 3);
    VALIDATE_STATE(Standby, Standby, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandby2ErrorActive)
{
    setMuxStandby();

    handleMuxState("error", 3);
    VALIDATE_STATE(Standby, Error, Up);

    handleMuxState("error", 3);
    VALIDATE_STATE(Standby, Error, Up);

    postLinkProberEvent(link_prober::LinkProberState::Active, 2);
    VALIDATE_STATE(Active, Wait, Up);

    handleProbeMuxState("active", 3);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxActive2Error2Unknown)
{
    setMuxActive();

    postLinkEvent(link_state::LinkState::Down, 2);
    VALIDATE_STATE(Active, Wait, Down);

    // fake mux statedb state to be active
    mDbInterfacePtr->setNextMuxState(mux_state::MuxState::Error);

    handleMuxState("error", 4);
    VALIDATE_STATE(Active, Error, Down);

    handleMuxState("error", 3);
    VALIDATE_STATE(Active, Error, Down);

    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Active, Unknown, Down);

    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Active, Unknown, Down);

    handleProbeMuxState("active", 3);
    VALIDATE_STATE(Active, Active, Down);
}

TEST_F(LinkManagerStateMachineTest, MuxStandby2Error2Unknown)
{
    setMuxStandby();

    postLinkEvent(link_state::LinkState::Down, 2);
    VALIDATE_STATE(Standby, Standby, Down);

    // fake mux statedb state to be active
    mDbInterfacePtr->setNextMuxState(mux_state::MuxState::Error);

    handleMuxState("error", 3);
    VALIDATE_STATE(Standby, Error, Down);

    handleMuxState("error", 3);
    VALIDATE_STATE(Standby, Error, Down);

    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Standby, Unknown, Down);

    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Standby, Unknown, Down);

    handleProbeMuxState("standby", 3);
    VALIDATE_STATE(Standby, Standby, Down);
}

TEST_F(LinkManagerStateMachineTest, MuxActive2Unknown2Error)
{
    setMuxActive();

    postLinkEvent(link_state::LinkState::Down, 2);
    VALIDATE_STATE(Active, Wait, Down);

    handleProbeMuxState("unknown", 4);
    VALIDATE_STATE(Active, Unknown, Down);

    handleMuxState("error", 3);
    VALIDATE_STATE(Active, Error, Down);
}

TEST_F(LinkManagerStateMachineTest, MuxStandby2Unknown2Error)
{
    setMuxStandby();

    postLinkEvent(link_state::LinkState::Down, 2);
    VALIDATE_STATE(Standby, Standby, Down);

    handleProbeMuxState("unknown", 3);
    VALIDATE_STATE(Standby, Unknown, Down);

    handleMuxState("error", 3);
    VALIDATE_STATE(Standby, Error, Down);
}

TEST_F(LinkManagerStateMachineTest, MuxActivDefaultRouteStateNA) 
{
    setMuxActive();

    EXPECT_FALSE(mMuxConfig.getIfEnableDefaultRouteFeature());
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mShutdownTxProbeCallCount, 0);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mRestartTxProbeCallCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mLastSetMuxLinkmgrState, link_manager::LinkManagerStateMachineBase::Label::Healthy);

    postDefaultRouteEvent("na", 3);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mShutdownTxProbeCallCount, 0);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mRestartTxProbeCallCount, 2);
    EXPECT_EQ(mDbInterfacePtr->mLastSetMuxLinkmgrState, link_manager::LinkManagerStateMachineBase::Label::Healthy);

    mMuxConfig.enableDefaultRouteFeature(true);
    postDefaultRouteEvent("na", 3);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mShutdownTxProbeCallCount, 1);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mRestartTxProbeCallCount, 2);
    EXPECT_EQ(mDbInterfacePtr->mLastSetMuxLinkmgrState, link_manager::LinkManagerStateMachineBase::Label::Unhealthy);

    postDefaultRouteEvent("ok", 3);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mShutdownTxProbeCallCount,1);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mRestartTxProbeCallCount,3);
    EXPECT_EQ(mDbInterfacePtr->mLastSetMuxLinkmgrState, link_manager::LinkManagerStateMachineBase::Label::Healthy);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyPeerLinkStateDown)
{
    setMuxStandby();

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    postPeerLinkStateEvent("down", 3);

    VALIDATE_STATE(Wait, Wait, Up);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mPostSwitchCauseInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mLastPostedSwitchCause, link_manager::ActiveStandbyStateMachine::SwitchCause::PeerLinkDown);

    postLinkProberEvent(link_prober::LinkProberState::Active, 3);
    VALIDATE_STATE(Active, Wait, Up);

    handleMuxState("active", 3);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyLinkDownPeerLinkDown)
{
    setMuxStandby();

    handleLinkState("down", 3);
    VALIDATE_STATE(Standby, Standby, Down);

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    postPeerLinkStateEvent("down", 3);

    VALIDATE_STATE(Standby, Standby, Down);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
}

TEST_F(LinkManagerStateMachineTest, MuxActivePeerLinkStateUp)
{
    setMuxActive();

    postPeerLinkStateEvent("up", 3);

    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Active, Active, Up);
}

TEST_F(LinkManagerStateMachineTest, PostPckLossMetricsEvent) 
{
    setMuxStandby();

    int postLinkProberMetricsInvokeCountBefore = mDbInterfacePtr->mPostLinkProberMetricsInvokeCount;

    // post link_prober_standby_start
    EXPECT_EQ(mDbInterfacePtr->mPostLinkProberMetricsInvokeCount, postLinkProberMetricsInvokeCountBefore);
    postLinkProberEvent(link_prober::LinkProberState::Unknown, 3);

    // post link_prober_unknown_start, link_prober_wait_start
    EXPECT_EQ(mDbInterfacePtr->mPostLinkProberMetricsInvokeCount, postLinkProberMetricsInvokeCountBefore + 2);
    postLinkProberEvent(link_prober::LinkProberState::Active, 3);

    // post link_prober_unknown_start, post link_prober_active_start
    EXPECT_EQ(mDbInterfacePtr->mPostLinkProberMetricsInvokeCount, postLinkProberMetricsInvokeCountBefore + 3);
}

TEST_F(LinkManagerStateMachineTest, PostPckLossUpdateAndResetEvent)
{
    uint64_t unknownCount = 999;
    uint64_t totalCount = 10000;

    postPckLossRatioUpdateEvent(unknownCount,totalCount);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mIcmpUnknownEventCount, unknownCount);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mIcmpPacketCount, totalCount);
    EXPECT_EQ(mDbInterfacePtr->mUnknownEventCount, unknownCount);
    EXPECT_EQ(mDbInterfacePtr->mExpectedPacketCount, totalCount);

    postPckLossCountsResetEvent();
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mIcmpUnknownEventCount, 0);
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mIcmpPacketCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mUnknownEventCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mExpectedPacketCount, 0);    
}

TEST_F(LinkManagerStateMachineTest, EnableDecreaseLinkProberIntervalFeature)
{
    setMuxStandby();
    
    // feature is disabled by default 
    EXPECT_FALSE(mMuxConfig.getIfEnableSwitchoverMeasurement());
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mDecreaseIntervalCallCount, 0);

    // switch to active 
    handleMuxConfig("active", 4);
    postLinkProberEvent(link_prober::LinkProberState::Active, 3);
    handleMuxState("active", 3);
    VALIDATE_STATE(Active, Active, Up); 

    // interval is not decreased   
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mDecreaseIntervalCallCount, 0);

    // enable the feature 
    mMuxConfig.enableSwitchoverMeasurement(true);
    EXPECT_TRUE(mMuxConfig.getIfEnableSwitchoverMeasurement());

    // switch to standby (fake inconsistency between state db and mux probing)
    handleProbeMuxState("standby", 3);
    handleGetMuxState("active", 3);

    // interval is decreased once 
    EXPECT_EQ(mFakeMuxPort.mFakeLinkProber->mDecreaseIntervalCallCount, 1);
}

TEST_F(LinkManagerStateMachineTest, CableFirmwareFailure)
{
    // This test case is for scenario when we have a bad mux firmware.
    // Issue: If a switchover triggered in this case, tunnel route will be added/removed
    // unexpectedly, and cause traffic drop. 
    // Expected behavior: linkmgrd should enforce another toggle. 

    setMuxStandby();

    postMuxEvent(mux_state::MuxState::Unknown, 3);
    VALIDATE_STATE(Standby, Wait, Up);

    handleProbeMuxState("unknown", 4);
    VALIDATE_STATE(Standby, Wait, Up);

    handleProbeMuxState("unknown", 4);
    VALIDATE_STATE(Standby, Wait, Up);

    runIoService(2);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mLastPostedSwitchCause, link_manager::ActiveStandbyStateMachine::SwitchCause::HarewareStateUnknown);

    // if link prober state changes
    postLinkProberEvent(link_prober::LinkProberState::Active);
    VALIDATE_STATE(Active, Wait, Up);

    postMuxEvent(mux_state::MuxState::Unknown, 3);
    VALIDATE_STATE(Active, Wait, Up);

    handleProbeMuxState("unknown", 4);
    VALIDATE_STATE(Active, Wait, Up);

    handleProbeMuxState("unknown", 4);
    VALIDATE_STATE(Active, Wait, Up);

    runIoService(2);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 2);
    EXPECT_EQ(mDbInterfacePtr->mLastPostedSwitchCause, link_manager::ActiveStandbyStateMachine::SwitchCause::HarewareStateUnknown);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyLinkProberUnknownDefaultRouteNA)
{
    setMuxStandby();

    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);

    mMuxConfig.enableDefaultRouteFeature(true);
    EXPECT_TRUE(mMuxConfig.getIfEnableDefaultRouteFeature());

    postDefaultRouteEvent("na", 3);
    EXPECT_EQ(mFakeMuxPort.mActiveStandbyStateMachinePtr->getDefaultRouteState(), link_manager::LinkManagerStateMachineBase::DefaultRoute::NA);

    // LinkProberState::Unknown now will only trigger mux probe instead of mux toggle
    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Wait, Wait, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyLinkProberStandbyLinkDownMuxWaitLinkUp)
{
    setMuxStandby();
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Standby, Standby, Up);

    handleLinkState("down", 3);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Standby, Standby, Down);

    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Unknown, Standby, Down);

    runIoService(1);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Unknown, Wait, Down);

    handleLinkState("up");
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Wait, Wait, Up);

    handleProbeMuxState("standby", 3);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Wait, Standby, Up);

    runIoService(1);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 2);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Wait, Wait, Up);

    postLinkProberEvent(link_prober::LinkProberState::Standby);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 2);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Standby, Wait, Up);

    handleProbeMuxState("standby", 3);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 2);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Standby, Standby, Up);
}

TEST_F(LinkManagerStateMachineTest, MuxStandbyLinkProberStandbyLinkDownLinkUpResetLinkProberState)
{
    setMuxStandby();
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Standby, Standby, Up);

    handleLinkState("down", 3);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Standby, Standby, Down);

    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2, 2);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Standby, Standby, Down);

    handleLinkState("up");
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Standby, Standby, Up);

    postLinkProberEvent(link_prober::LinkProberState::Unknown, 2, 1);
    EXPECT_EQ(mDbInterfacePtr->mProbeMuxStateInvokeCount, 0);
    EXPECT_EQ(mDbInterfacePtr->mSetMuxStateInvokeCount, 0);
    VALIDATE_STATE(Standby, Standby, Up);
}


} /* namespace test */
