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

#ifndef MOCKMUXPORT_H_
#define MOCKMUXPORT_H_

#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "common/MuxLogger.h"
#include "MuxPort.h"
#include "MockLinkManagerStateMachine.h"

namespace test
{

class MockMuxPort : public mux::MuxPort
{
public:
    MockMuxPort() = delete;

    MockMuxPort(
        std::shared_ptr<mux::DbInterface> dbInterfacePtr,
        common::MuxConfig &muxConfig,
        const std::string &portName,
        uint16_t serverId,
        boost::asio::io_service &ioService,
        common::MuxPortConfig::PortCableType portCableType
    );

    MockMuxPort(const MockMuxPort &) = delete;

    boost::asio::io_service::strand &getStrand() { return mStrand; };
    std::shared_ptr<link_manager::LinkManagerStateMachineBase> getLinkManagerStateMachinePtr() { return mLinkManagerStateMachinePtr; };
    std::shared_ptr<link_prober::LinkProberStateMachineBase> getLinkProberStateMachinePtr() { return getLinkManagerStateMachinePtr()->getLinkProberStateMachinePtr(); };
    std::shared_ptr<link_prober::LinkProberSessionStateMachine> getLinkProberSessionStateMachinePtr() {return getLinkManagerStateMachinePtr()->getLinkProberSessionStateMachinePtr();};
    const std::string &getSelfSessionId() { return mSelfSessionId; };
    const std::string &getPeerSessionId() { return mPeerSessionId; };

    ~MockMuxPort() = default;

    MOCK_METHOD2(postLinkProberSessionStateNotificationToDb, void(const std::string &sessionId, link_prober::LinkProberState::Label label));
};

} // namespace test

#endif
