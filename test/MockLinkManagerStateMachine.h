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

#ifndef MOCKLINKMANAGERSTATEMACHINE_H_
#define MOCKLINKMANAGERSTATEMACHINE_H_

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "common/MuxLogger.h"
#include "link_manager/LinkManagerStateMachineBase.h"
#include "MuxPort.h"

namespace test
{

class MockLinkManagerStateMachine : public link_manager::LinkManagerStateMachineBase
{
public:
    MockLinkManagerStateMachine() = delete;

    MockLinkManagerStateMachine(
        mux::MuxPort *muxPortPtr,
        boost::asio::io_service::strand &strand,
        common::MuxPortConfig &muxPortConfig
    );

    MockLinkManagerStateMachine(const MockLinkManagerStateMachine &) = delete;

    ~MockLinkManagerStateMachine() = default;

    MOCK_METHOD0(initializeTransitionFunctionTable, void());

    void handleStateChange(link_manager::LinkProberEvent &event, link_prober::LinkProberState::Label state) override;

    void handleStateChange(link_manager::MuxStateEvent &event, mux_state::MuxState::Label state) override;

    void handleStateChange(link_manager::LinkStateEvent &event, link_state::LinkState::Label state) override;

    void handlePeerStateChange(link_manager::LinkProberEvent &event, link_prober::LinkProberState::Label state) override;

    MOCK_METHOD1(handleLinkProberStateChange, void(link_prober::LinkProberState::Label state));

    MOCK_METHOD1(handleMuxStateChange, void(mux_state::MuxState::Label state));

    MOCK_METHOD1(handleLinkStateChange, void(link_state::LinkState::Label state));

    MOCK_METHOD1(handlePeerLinkProberStateChange, void(link_prober::LinkProberState::Label state));

    MOCK_METHOD1(setLabel, void(link_manager::LinkManagerStateMachineBase::Label label));
};

} // namespace test

#endif
