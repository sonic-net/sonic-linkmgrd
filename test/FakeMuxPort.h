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
 * MuxPort.h
 *
 *  Created on: Oct 23, 2020
 *      Author: Tamer Ahmed
 */

#ifndef FAKEMUXPORT_H_
#define FAKEMUXPORT_H_

#include <gtest/gtest.h>
#include <boost/asio.hpp>

#include "MuxPort.h"
#include "FakeDbInterface.h"
#include "FakeLinkProber.h"

namespace test {

class FakeMuxPort : public ::mux::MuxPort {
public:
    FakeMuxPort(
        std::shared_ptr<FakeDbInterface> dbInterface,
        common::MuxConfig& muxConfig,
        std::string& portName,
        uint16_t serverId,
        boost::asio::io_service& ioService);
    virtual ~FakeMuxPort() = default;

    void activateStateMachine();

    std::shared_ptr<link_manager::ActiveStandbyStateMachine> getActiveStandbyStateMachinePtr() { return mActiveStandbyStateMachinePtr; }
    const link_manager::ActiveStandbyStateMachine::CompositeState& getCompositeState() { return getActiveStandbyStateMachinePtr()->getCompositeState(); };
    link_prober::LinkProberStateMachine& getLinkProberStateMachine() { return getActiveStandbyStateMachinePtr()->getLinkProberStateMachine(); };
    mux_state::MuxStateMachine& getMuxStateMachine() { return getActiveStandbyStateMachinePtr()->getMuxStateMachine(); };
    link_state::LinkStateMachine& getLinkStateMachine() { return getActiveStandbyStateMachinePtr()->getLinkStateMachine(); };

    bool getPendingMuxModeChange() { return getActiveStandbyStateMachinePtr()->mPendingMuxModeChange; };
    common::MuxPortConfig::Mode getTargetMuxMode() { return getActiveStandbyStateMachinePtr()->mTargetMuxMode; };

    std::shared_ptr<link_manager::ActiveStandbyStateMachine> mActiveStandbyStateMachinePtr;
    std::shared_ptr<FakeLinkProber> mFakeLinkProber;
};

} /* namespace test */

#endif /* FAKEMUXPORT_H_ */
