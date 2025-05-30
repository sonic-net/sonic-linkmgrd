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
 * FakeDbInterface.h
 *
 *  Created on: Oct 23, 2020
 *      Author: Tamer Ahmed
 */

#ifndef FAKEDBINTERFACE_H_
#define FAKEDBINTERFACE_H_

#include "DbInterface.h"

using IcmpHwOffloadEntries = std::vector<std::pair<std::string, std::string>>;
using IcmpHwOffloadEntriesPtr = std::unique_ptr<IcmpHwOffloadEntries>;

namespace test
{

class FakeDbInterface: public mux::DbInterface
{
public:
    FakeDbInterface(boost::asio::io_service *ioService);
    FakeDbInterface(mux::MuxManager *muxManager, boost::asio::io_service *ioService);
    virtual ~FakeDbInterface() = default;

    virtual void handleSetMuxState(const std::string portName, mux_state::MuxState::Label label) override;
    virtual void handleSetPeerMuxState(const std::string portName, mux_state::MuxState::Label label) override;
    virtual void getMuxState(const std::string &portName) override;
    virtual std::map<std::string, std::string> getMuxModeConfig() override;
    virtual void probeMuxState(const std::string &portName) override;
    virtual void handleProbeForwardingState(const std::string portName) override;
    virtual void updateIntervalv4(uint32_t tx_interval, uint32_t rx_interval) override;
    virtual void updateIntervalv6(uint32_t tx_interval, uint32_t rx_interval) override;
    virtual void createIcmpEchoSession(std::string key, IcmpHwOffloadEntriesPtr entries) override;
    virtual void deleteIcmpEchoSession(std::string key) override;
    virtual void handleSwssNotification() override;
    virtual void setMuxLinkmgrState(
        const std::string &portName,
        link_manager::LinkManagerStateMachineBase::Label label
    ) override;
    virtual void handlePostMuxMetrics(
        const std::string portName,
        link_manager::ActiveStandbyStateMachine::Metrics metrics,
        mux_state::MuxState::Label label,
        boost::posix_time::ptime time
    ) override;
    virtual void postLinkProberMetricsEvent(
        const std::string &portName, 
        link_manager::ActiveStandbyStateMachine::LinkProberMetrics metrics
    ) override;
    virtual void postPckLossRatio(
        const std::string &portName,
        const uint64_t unknownEventCount, 
        const uint64_t expectedPacketCount
    ) override;
    virtual bool isWarmStart() override;
    virtual uint32_t getWarmStartTimer() override;
    virtual void setWarmStartStateReconciled() override; 
    virtual void postSwitchCause(
        const std::string &portName,
        link_manager::ActiveStandbyStateMachine::SwitchCause cause
    ) override;

    void setNextMuxState(mux_state::MuxState::Label label) {mNextMuxState = label;};

private:
    virtual void handleSetMuxMode(const std::string &portName, const std::string state) override;

public:
    mux_state::MuxState::Label mNextMuxState;

    uint32_t mMuxStateRequest[mux_state::MuxState::Label::Count] = {0, 0, 0, 0};

    mux_state::MuxState::Label mLastSetMuxState;
    mux_state::MuxState::Label mLastSetPeerMuxState;
    link_manager::LinkManagerStateMachineBase::Label mLastSetMuxLinkmgrState;

    uint32_t mSetMuxStateInvokeCount = 0;
    uint32_t mSetPeerMuxStateInvokeCount = 0;
    uint32_t mGetMuxStateInvokeCount = 0;
    uint32_t mProbeMuxStateInvokeCount = 0;
    uint32_t mProbeForwardingStateInvokeCount = 0;
    uint32_t mUpdateIntervalV4Count = 0;
    uint32_t mUpdateIntervalV6Count = 0;
    uint32_t mSetMuxLinkmgrStateInvokeCount = 0;
    uint32_t mPostMetricsInvokeCount = 0;
    uint32_t mPostLinkProberMetricsInvokeCount = 0;
    uint64_t mUnknownEventCount = 0;
    uint64_t mExpectedPacketCount = 0;
    uint32_t mSetMuxModeInvokeCount = 0;
    uint32_t mSetWarmStartStateReconciledInvokeCount = 0;
    uint32_t mPostSwitchCauseInvokeCount = 0;
    uint32_t mGetMuxModeConfigInvokeCount = 0;
    uint32_t mIcmpSessionsCount = 0;

    link_manager::ActiveStandbyStateMachine::SwitchCause mLastPostedSwitchCause;
    
    bool mWarmStartFlag = false;

    bool mDbInterfaceRaceConditionCheckFailure = false;
};

} /* namespace test */

#endif /* FAKEDBINTERFACE_H_ */
