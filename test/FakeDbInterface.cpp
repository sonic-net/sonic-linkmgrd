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
 * FakeDbInterface.cpp
 *
 *  Created on: Oct 23, 2020
 *      Author: Tamer Ahmed
 */

#include "FakeDbInterface.h"

namespace test
{

FakeDbInterface::FakeDbInterface(boost::asio::io_service *ioService) :
    mux::DbInterface(nullptr, ioService),
    mNextMuxState(mux_state::MuxState::Label::Unknown)
{
}

FakeDbInterface::FakeDbInterface(mux::MuxManager *muxManager, boost::asio::io_service *ioService) :
    mux::DbInterface(muxManager, ioService),
    mNextMuxState(mux_state::MuxState::Label::Unknown)
{
}

void FakeDbInterface::setMuxState(const std::string &portName, mux_state::MuxState::Label label)
{
    mSetMuxStateInvokeCount++;
}

void FakeDbInterface::getMuxState(const std::string &portName)
{
    mGetMuxStateInvokeCount++;
}

void FakeDbInterface::probeMuxState(const std::string &portName)
{
    mProbeMuxStateInvokeCount++;
}

void FakeDbInterface::setMuxLinkmgrState(const std::string &portName, link_manager::LinkManagerStateMachine::Label label)
{
    mSetMuxLinkmgrStateInvokeCount++;
}

void FakeDbInterface::postMetricsEvent(
    const std::string &portName,
    link_manager::LinkManagerStateMachine::Metrics metrics,
    mux_state::MuxState::Label label
)
{
    mPostMetricsInvokeCount++;
}

void FakeDbInterface::postLinkProberMetricsEvent(
        const std::string &portName, 
        link_manager::LinkManagerStateMachine::LinkProberMetrics metrics
)
{
    mPostLinkProberMetricsInvokeCount++;
}

void FakeDbInterface::postPckLossRatio(
        const std::string &portName,
        const double_t ratio
)
{
    mPostPckLossRatioInvokeCount;
} 

} /* namespace test */
