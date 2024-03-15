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

#include "MockMuxPort.h"

namespace test
{
MockMuxPort::MockMuxPort(
    std::shared_ptr<mux::DbInterface> dbInterfacePtr,
    common::MuxConfig &muxConfig,
    const std::string &portName,
    uint16_t serverId,
    boost::asio::io_service &ioService,
    common::MuxPortConfig::PortCableType portCableType
)
    : MuxPort(dbInterfacePtr, muxConfig, portName, serverId, ioService, portCableType)
{
    mLinkManagerStateMachinePtr.reset(new MockLinkManagerStateMachine(this, getStrand(), mMuxPortConfig));
    if (muxConfig.getIfEnableSimulateLfdOffload()) {
        initLinkProberSessions();
    }
}

} // namespace test
