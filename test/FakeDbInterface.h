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

namespace test
{

class FakeDbInterface: public mux::DbInterface
{
public:
    FakeDbInterface(boost::asio::io_service *ioService);
    FakeDbInterface(mux::MuxManager *muxManager, boost::asio::io_service *ioService);
    virtual ~FakeDbInterface() = default;

    virtual void setMuxState(const std::string &portName, mux_state::MuxState::Label label) override;
    virtual void getMuxState(const std::string &portName) override;
    virtual void probeMuxState(const std::string &portName) override;
    virtual void setMuxLinkmgrState(
        const std::string &portName,
        link_manager::LinkManagerStateMachine::Label label
    ) override;
    virtual void postMetricsEvent(
        const std::string &portName,
        link_manager::LinkManagerStateMachine::Metrics metrics,
        mux_state::MuxState::Label label
    ) override;
    virtual void postLinkProberMetricsEvent(
        const std::string &portName, 
        link_manager::LinkManagerStateMachine::LinkProberMetrics metrics
    ) override;
    virtual void postPckLossRatio(
        const std::string &portName,
        const uint64_t unknownEventCount, 
        const uint64_t expectedPacketCount
    ) override;


    void setNextMuxState(mux_state::MuxState::Label label) {mNextMuxState = label;};


public:
    mux_state::MuxState::Label mNextMuxState;

    uint32_t mMuxStateRequest[mux_state::MuxState::Label::Count] = {0, 0, 0, 0};

    uint32_t mSetMuxStateInvokeCount = 0;
    uint32_t mGetMuxStateInvokeCount = 0;
    uint32_t mProbeMuxStateInvokeCount = 0;
    uint32_t mSetMuxLinkmgrStateInvokeCount = 0;
    uint32_t mPostMetricsInvokeCount = 0;
    uint32_t mPostLinkProberMetricsInvokeCount = 0;
    uint64_t mUnknownEventCount = 0;
    uint64_t mExpectedPacketCount = 0;
};

} /* namespace test */

#endif /* FAKEDBINTERFACE_H_ */
