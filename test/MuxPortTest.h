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
 * MuxPortTest.h
 *
 *  Created on: Oct 23, 2020
 *      Author: Longxiang Lyu <lolv@microsoft.com>
 */

#ifndef MUXPORTTEST_H_
#define MUXPORTTEST_H_

#include <memory>
#include "gtest/gtest.h"

#include "FakeMuxPort.h"
#include "FakeLinkManagerStateMachine.h"

namespace test
{

class MuxPortTest: public testing::Test
{
public:
    MuxPortTest();
    virtual ~MuxPortTest() = default;

    void runIoServiceThreaded(uint32_t count = 3);
    void stopIoServiceThreaded();

    void SetUp() override;

    void TearDown() override;

public:
    boost::asio::io_service mIoService;
    std::unique_ptr<boost::asio::io_service::work> mWork;
    boost::thread_group mThreadGroup;
    common::MuxConfig mMuxConfig;
    std::shared_ptr<FakeDbInterface> mDbInterfacePtr;
    std::string mPortName = "EtherTest01";
    std::string mSmartNicIpAddress = "192.168.1.20";
    uint16_t mServerId = 01;

    FakeMuxPort mFakeMuxPort;
    link_manager::ActiveStandbyStateMachine::CompositeState mTestCompositeState;

    uint8_t mPositiveUpdateCount = 2;
};


} /* namespace test */

#endif /* MUXPORTTEST_H_ */
