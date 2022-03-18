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

namespace test
{

FakeMuxPort::FakeMuxPort(
    std::shared_ptr<FakeDbInterface> dbInterface,
    common::MuxConfig &muxConfig,
    std::string &portName,
    uint16_t serverId,
    boost::asio::io_service &ioService
) :
    mux::MuxPort(
        dbInterface,
        muxConfig,
        portName,
        serverId,
        ioService,
        common::MuxPortConfig::PortCableType::ActiveStandby
    ),
    mActiveStandbyStateMachinePtr(
        std::dynamic_pointer_cast<link_manager::ActiveStandbyStateMachine>(getLinkManagerStateMachinePtr())
    ),
    mFakeLinkProber(
        std::make_shared<FakeLinkProber> (mActiveStandbyStateMachinePtr->getLinkProberStateMachinePtr().get())
    )
{
    std::string prog_name = "linkmgrd-test";
    std::string log_filename = "/tmp/" + prog_name + ".log";
    bool extraLogFile = true;
    common::MuxLogger::getInstance()->initialize(prog_name, log_filename, boost::log::trivial::debug, extraLogFile);
    common::MuxLogger::getInstance()->setLevel(boost::log::trivial::trace);
    mMuxPortConfig.setMode(common::MuxPortConfig::Mode::Auto);
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
    getActiveStandbyStateMachinePtr()->setSuspendTxFnPtr(
        boost::bind(&FakeLinkProber::suspendTxProbes, mFakeLinkProber.get(), boost::placeholders::_1)
    );
    getActiveStandbyStateMachinePtr()->setResumeTxFnPtr(
        boost::bind(&FakeLinkProber::resumeTxProbes, mFakeLinkProber.get())
    );
    getActiveStandbyStateMachinePtr()->setSendPeerSwitchCommandFnPtr(
        boost::bind(&FakeLinkProber::sendPeerSwitchCommand, mFakeLinkProber.get())
    );
    getActiveStandbyStateMachinePtr()->setShutdownTxFnPtr(
        boost::bind(&FakeLinkProber::shutdownTxProbes, mFakeLinkProber.get())
    );
    getActiveStandbyStateMachinePtr()->setRestartTxFnPtr(
        boost::bind(&FakeLinkProber::restartTxProbes, mFakeLinkProber.get())
    );
}

void FakeMuxPort::activateStateMachine()
{
    setComponentInitState(0);
    setComponentInitState(1);
    setComponentInitState(2);
}

} /* namespace test */
