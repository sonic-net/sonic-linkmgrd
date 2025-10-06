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
 * MuxPort.cpp
 *
 *  Created on: Oct 23, 2020
 *      Author: tamer
 */

#include <memory>

#include "common/MuxLogger.h"
#include "FakeMuxPort.h"
#include "FakeLinkProber.h"
#include "FakeLinkManagerStateMachine.h"

namespace test
{

FakeMuxPort::FakeMuxPort(
    std::shared_ptr<FakeDbInterface> dbInterface,
    common::MuxConfig &muxConfig,
    std::string &portName,
    uint16_t serverId,
    boost::asio::io_service &ioService,
    common::MuxPortConfig::PortCableType portCableType,
    bool useFakeLinkManagerStateMachine
)
    : mux::MuxPort(
          dbInterface,
          muxConfig,
          portName,
          serverId,
          ioService,
          portCableType
      ),
      mActiveActiveStateMachinePtr(
          std::dynamic_pointer_cast<link_manager::ActiveActiveStateMachine>(getLinkManagerStateMachinePtr())
      ),
      mActiveStandbyStateMachinePtr(
          std::dynamic_pointer_cast<link_manager::ActiveStandbyStateMachine>(getLinkManagerStateMachinePtr())
      ),
      mFakeLinkProber(
          std::make_shared<FakeLinkProber>(getLinkManagerStateMachinePtr()->getLinkProberStateMachinePtr().get())
      )
{
    mMuxPortConfig.setMode(common::MuxPortConfig::Mode::Auto);
    if (useFakeLinkManagerStateMachine)
    {
        resetLinkManagerStateMachinePtr(
            new FakeLinkManagerStateMachine(
                this,
                mStrand,
                mMuxPortConfig
            )
        );
        mFakeLinkManagerStateMachinePtr = std::dynamic_pointer_cast<FakeLinkManagerStateMachine>(getLinkManagerStateMachinePtr());
    }
    else
    {
        switch (portCableType) {
            case common::MuxPortConfig::PortCableType::ActiveStandby:
                initLinkProberActiveStandby();
                break;
            case common::MuxPortConfig::PortCableType::ActiveActive:
                initLinkProberActiveActive();
                break;
            default:
                break;
        }
    }
}

inline void FakeMuxPort::initLinkProberActiveActive()
{
    getActiveActiveStateMachinePtr()->setInitializeProberFnPtr(
        boost::bind(&FakeLinkProber::initialize, mFakeLinkProber.get())
    );
    getActiveActiveStateMachinePtr()->setStartProbingFnPtr(
        boost::bind(&FakeLinkProber::startProbing, mFakeLinkProber.get())
    );
    getActiveActiveStateMachinePtr()->setUpdateEthernetFrameFnPtr(
        boost::bind(&FakeLinkProber::updateEthernetFrame, mFakeLinkProber.get())
    );
    getActiveActiveStateMachinePtr()->setProbePeerTorFnPtr(
        boost::bind(&FakeLinkProber::probePeerTor, mFakeLinkProber.get())
    );
    getActiveActiveStateMachinePtr()->setSuspendTxFnPtr(
        boost::bind(&FakeLinkProber::suspendTxProbes, mFakeLinkProber.get(), boost::placeholders::_1)
    );
    getActiveActiveStateMachinePtr()->setResumeTxFnPtr(
        boost::bind(&FakeLinkProber::resumeTxProbes, mFakeLinkProber.get())
    );
    getActiveActiveStateMachinePtr()->setShutdownTxFnPtr(
        boost::bind(&FakeLinkProber::shutdownTxProbes, mFakeLinkProber.get())
    );
    getActiveActiveStateMachinePtr()->setRestartTxFnPtr(
        boost::bind(&FakeLinkProber::restartTxProbes, mFakeLinkProber.get())
    );
    getActiveActiveStateMachinePtr()->setResetIcmpPacketCountsFnPtr(
        boost::bind(&FakeLinkProber::resetIcmpPacketCounts, mFakeLinkProber.get())
    );
    getActiveActiveStateMachinePtr()->setSendPeerProbeCommandFnPtr(
        boost::bind(&FakeLinkProber::sendPeerProbeCommand, mFakeLinkProber.get())
    );
}

inline void FakeMuxPort::initLinkProberActiveStandby()
{
    getActiveStandbyStateMachinePtr()->setInitializeProberFnPtr(
        boost::bind(&FakeLinkProber::initialize, mFakeLinkProber.get())
    );
    getActiveStandbyStateMachinePtr()->setStartProbingFnPtr(
        boost::bind(&FakeLinkProber::startProbing, mFakeLinkProber.get())
    );
    getActiveStandbyStateMachinePtr()->setUpdateEthernetFrameFnPtr(
        boost::bind(&FakeLinkProber::updateEthernetFrame, mFakeLinkProber.get())
    );
    getActiveStandbyStateMachinePtr()->setProbePeerTorFnPtr(
        boost::bind(&FakeLinkProber::probePeerTor, mFakeLinkProber.get())
    );
    getActiveStandbyStateMachinePtr()->setDetectLinkFnPtr(
        boost::bind(&FakeLinkProber::detectLink, mFakeLinkProber.get())
    );
    getActiveStandbyStateMachinePtr()->setSuspendTxFnPtr(
        boost::bind(&FakeLinkProber::suspendTxProbes, mFakeLinkProber.get(), boost::placeholders::_1)
    );
    getActiveStandbyStateMachinePtr()->setResumeTxFnPtr(
        boost::bind(&FakeLinkProber::resumeTxProbes, mFakeLinkProber.get())
    );
    getActiveStandbyStateMachinePtr()->setSendPeerSwitchCommandFnPtr(
        boost::bind(&FakeLinkProber::sendPeerSwitchCommand, mFakeLinkProber.get())
    );
   getActiveStandbyStateMachinePtr()->mSendPeerSwitchCommandFnPtr_pc = boost::bind(
       &FakeLinkProber::sendPeerSwitchCommand_pc, mFakeLinkProber.get()
   );
    getActiveStandbyStateMachinePtr()->setShutdownTxFnPtr(
        boost::bind(&FakeLinkProber::shutdownTxProbes, mFakeLinkProber.get())
    );
    getActiveStandbyStateMachinePtr()->setRestartTxFnPtr(
        boost::bind(&FakeLinkProber::restartTxProbes, mFakeLinkProber.get())
    );
    getActiveStandbyStateMachinePtr()->setDecreaseIntervalFnPtr(
        boost::bind(&FakeLinkProber::decreaseProbeIntervalAfterSwitch, mFakeLinkProber.get(), boost::placeholders::_1)
    );
    getActiveStandbyStateMachinePtr()->setRevertIntervalFnPtr(
        boost::bind(&FakeLinkProber::revertProbeIntervalAfterSwitchComplete, mFakeLinkProber.get())
    );
}

void FakeMuxPort::activateStateMachine()
{
    setComponentInitState(0);
    setComponentInitState(1);
    setComponentInitState(2);

    if (mMuxPortConfig.ifEnableDefaultRouteFeature() == true){
        mFakeLinkProber->shutdownTxProbes();
    }

    if (mMuxPortConfig.getPortCableType() == common::MuxPortConfig::PortCableType::ActiveActive) {
        getActiveActiveStateMachinePtr()->mWaitStateMachineInit.notify_all();
    }
}

} /* namespace test */
