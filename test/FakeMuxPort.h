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

#include <common/BoostAsioBehavior.h>
#include <boost/asio.hpp>

#include "MuxPort.h"
#include "FakeDbInterface.h"
#include "FakeLinkProber.h"
#include "FakeLinkManagerStateMachine.h"

namespace test
{

class FakeMuxPort : public ::mux::MuxPort {
public:
    FakeMuxPort(
        std::shared_ptr<FakeDbInterface> dbInterface,
        common::MuxConfig& muxConfig,
        std::string& portName,
        uint16_t serverId,
        boost::asio::io_service& ioService,
        common::MuxPortConfig::PortCableType portCableType = common::MuxPortConfig::PortCableType::ActiveStandby,
        bool useFakeLinkManagerStateMachine = false);
    virtual ~FakeMuxPort() = default;

    void activateStateMachine();

    virtual inline void postMetricsEvent(
        link_manager::ActiveStandbyStateMachine::Metrics metrics,
        mux_state::MuxState::Label label
    ) {
        mDbInterfacePtr->handlePostMuxMetrics(mMuxPortConfig.getPortName(), metrics, label, boost::posix_time::microsec_clock::universal_time());
    };
    virtual inline void setMuxState(mux_state::MuxState::Label label) {mDbInterfacePtr->handleSetMuxState(mMuxPortConfig.getPortName(), label);};

    std::shared_ptr<link_manager::ActiveActiveStateMachine> getActiveActiveStateMachinePtr() { return mActiveActiveStateMachinePtr; }
    std::shared_ptr<link_manager::ActiveStandbyStateMachine> getActiveStandbyStateMachinePtr() { return mActiveStandbyStateMachinePtr; }
    std::shared_ptr<FakeLinkManagerStateMachine> getFakeLinkManagerStateMachinePtr() { return mFakeLinkManagerStateMachinePtr; };
    const link_manager::LinkManagerStateMachineBase::CompositeState& getCompositeState() { return getLinkManagerStateMachinePtr()->getCompositeState(); };
    link_prober::LinkProberStateMachineBase* getLinkProberStateMachinePtr() { return getLinkManagerStateMachinePtr()->getLinkProberStateMachinePtr().get(); };
    mux_state::MuxStateMachine& getMuxStateMachine() { return getLinkManagerStateMachinePtr()->getMuxStateMachine(); };
    link_state::LinkStateMachine& getLinkStateMachine() { return getLinkManagerStateMachinePtr()->getLinkStateMachine(); };
    link_manager::LinkManagerStateMachineBase::DefaultRoute getDefaultRouteState() { return getLinkManagerStateMachinePtr()->getDefaultRouteState(); };

    bool getPendingMuxModeChange() { return getActiveStandbyStateMachinePtr()->mPendingMuxModeChange; };
    common::MuxPortConfig::Mode getTargetMuxMode() { return getActiveStandbyStateMachinePtr()->mTargetMuxMode; };

    link_prober::LinkProberState::Label getPeerLinkProberState() { return getActiveActiveStateMachinePtr()->mPeerLinkProberState; };
    mux_state::MuxState::Label getPeerMuxState() { return getActiveActiveStateMachinePtr()->mPeerMuxState; };
    uint32_t getActiveStandbyStateMachineSuspendBackoffFactor() { return getActiveStandbyStateMachinePtr()->mUnknownActiveUpBackoffFactor; }

    inline void initLinkProberActiveActive();
    inline void initLinkProberActiveStandby();

    std::shared_ptr<link_manager::ActiveActiveStateMachine> mActiveActiveStateMachinePtr;
    std::shared_ptr<link_manager::ActiveStandbyStateMachine> mActiveStandbyStateMachinePtr;
    std::shared_ptr<FakeLinkManagerStateMachine> mFakeLinkManagerStateMachinePtr = nullptr;
    std::shared_ptr<FakeLinkProber> mFakeLinkProber;
};

} /* namespace test */

#endif /* FAKEMUXPORT_H_ */
