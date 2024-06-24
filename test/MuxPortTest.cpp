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
 * MuxPortTest.cpp
 *
 *  Created on: Oct 23, 2020
 *      Author: Longxiang Lyu <lolv@microsoft.com>
 */

#include "common/MuxLogger.h"

#include "MuxPortTest.h"

namespace test
{

MuxPortTest::MuxPortTest() :
    mDbInterfacePtr(std::make_shared<FakeDbInterface>(&mIoService)),
    mFakeMuxPort(
        mDbInterfacePtr,
        mMuxConfig,
        mPortName,
        mServerId,
        mIoService,
        common::MuxPortConfig::PortCableType::ActiveStandby,
        true
    )
{
    mMuxConfig.setTimeoutIpv4_msec(10);
    mMuxConfig.setPositiveStateChangeRetryCount(mPositiveUpdateCount);
    mMuxConfig.setMuxStateChangeRetryCount(mPositiveUpdateCount);
    mMuxConfig.setLinkStateChangeRetryCount(mPositiveUpdateCount);
}

void MuxPortTest::runIoServiceThreaded(uint32_t count)
{
    mWork = std::make_unique<boost::asio::io_service::work>(mIoService);
    for (uint8_t i = 0; i < count; i++) {
        mThreadGroup.create_thread(boost::bind(&boost::asio::io_service::run, &mIoService));
    }
}

void MuxPortTest::stopIoServiceThreaded()
{
    mWork.reset();
    mIoService.stop();
}

void MuxPortTest::SetUp()
{
    runIoServiceThreaded(3);
}

void MuxPortTest::TearDown()
{
    stopIoServiceThreaded();
}

TEST_F(MuxPortTest, TestSwssBladeIpv4AddressUpdateHandler)
{
    auto fakeLinkManagerStateMachinePtr = mFakeMuxPort.getFakeLinkManagerStateMachinePtr();
    auto first_ip = boost::asio::ip::address::from_string("192.168.0.100");
    auto second_ip = boost::asio::ip::address::from_string("192.168.0.101");
    for (uint i = 0, called = 0; i < 10000; ++i)
    {
        MUXLOGDEBUG(boost::format("Iteration %d") % i);
        mFakeMuxPort.handleBladeIpv4AddressUpdate(first_ip);
        mFakeMuxPort.handleBladeIpv4AddressUpdate(second_ip);
        called += 2;

        uint32_t check = 0;
        while ((fakeLinkManagerStateMachinePtr->mSwssBladeIpv4AddressUpdateHandlerCalled < called) && (check < 10))
        {
            usleep(1000);
            ++check;
        }

        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mLastSwssBladeIpv4Address, second_ip);
        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mSwssBladeIpv4AddressUpdateHandlerCalled, called);
    }
}

TEST_F(MuxPortTest, TestSocIpv4AddressUpdateHandler)
{
    auto fakeLinkManagerStateMachinePtr = mFakeMuxPort.getFakeLinkManagerStateMachinePtr();
    auto first_ip = boost::asio::ip::address::from_string("192.168.0.100");
    auto second_ip = boost::asio::ip::address::from_string("192.168.0.101");
    for (uint i = 0, called = 0; i < 10000; ++i)
    {
        MUXLOGDEBUG(boost::format("Iteration %d") % i);
        mFakeMuxPort.handleSoCIpv4AddressUpdate(first_ip);
        mFakeMuxPort.handleSoCIpv4AddressUpdate(second_ip);
        called += 2;

        uint32_t check = 0;
        while ((fakeLinkManagerStateMachinePtr->mSwssSoCIpv4AddressUpdateHandlerCalled < called) && (check < 10))
        {
            usleep(1000);
            ++check;
        }

        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mLastSwssSoCIpv4Address, second_ip);
        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mSwssSoCIpv4AddressUpdateHandlerCalled, called);
    }
}

TEST_F(MuxPortTest, TestGetServerMacAddressHandler)
{
    auto fakeLinkManagerStateMachinePtr = mFakeMuxPort.getFakeLinkManagerStateMachinePtr();
    std::array<uint8_t, ETHER_ADDR_LEN> first_mac = {0, 'b', 2, 'd', 4, 'f'};
    std::array<uint8_t, ETHER_ADDR_LEN> second_mac = {0, 'b', 2, 'd', 8, 'f'};
    for (uint i = 0, called = 0; i < 10000; ++i)
    {
        MUXLOGDEBUG(boost::format("Iteration %d") % i);
        mFakeMuxPort.handleGetServerMacAddress(first_mac);
        mFakeMuxPort.handleGetServerMacAddress(second_mac);
        called += 2;

        uint32_t check = 0;
        while ((fakeLinkManagerStateMachinePtr->mGetServerMacAddressNotificationHandlerCalled < called) && (check < 10))
        {
            usleep(1000);
            ++check;
        }

        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mLastServerMacAddress, second_mac);
        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mGetServerMacAddressNotificationHandlerCalled, called);
    }
}

TEST_F(MuxPortTest, TestGetMuxStateNotificationHandler)
{
    auto fakeLinkManagerStateMachinePtr = mFakeMuxPort.getFakeLinkManagerStateMachinePtr();
    for (uint i = 0, called = 0; i < 10000; ++i)
    {
        MUXLOGDEBUG(boost::format("Iteration %d") % i);
        mFakeMuxPort.handleGetMuxState("active");
        mFakeMuxPort.handleGetMuxState("standby");
        called += 2;

        uint32_t check = 0;
        while ((fakeLinkManagerStateMachinePtr->mGetMuxStateNotificationHandlerCalled < called) && (check < 10))
        {
            usleep(1000);
            ++check;
        }

        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mLastGetMuxState, mux_state::MuxState::Label::Standby);
        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mGetMuxStateNotificationHandlerCalled, called);
    }
}

TEST_F(MuxPortTest, TestProbeMuxStateNotificationHandler)
{
    auto fakeLinkManagerStateMachinePtr = mFakeMuxPort.getFakeLinkManagerStateMachinePtr();
    for (uint i = 0, called = 0; i < 10000; ++i)
    {
        MUXLOGDEBUG(boost::format("Iteration %d") % i);
        mFakeMuxPort.handleProbeMuxState("active");
        mFakeMuxPort.handleProbeMuxState("standby");
        called += 2;

        uint32_t check = 0;
        while ((fakeLinkManagerStateMachinePtr->mProbeMuxStateNotificationHandlerCalled < called) && (check < 10))
        {
            usleep(1000);
            ++check;
        }

        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mLastProbeMuxState, mux_state::MuxState::Label::Standby);
        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mProbeMuxStateNotificationHandlerCalled, called);
    }
}

TEST_F(MuxPortTest, TestMuxStateNotificationHandler)
{
    auto fakeLinkManagerStateMachinePtr = mFakeMuxPort.getFakeLinkManagerStateMachinePtr();
    for (uint i = 0, called = 0; i < 10000; ++i)
    {
        MUXLOGDEBUG(boost::format("Iteration %d") % i);
        mFakeMuxPort.handleMuxState("active");
        mFakeMuxPort.handleMuxState("standby");
        called += 2;

        uint32_t check = 0;
        while ((fakeLinkManagerStateMachinePtr->mMuxStateNotificationHandlerCalled < called) && (check < 10))
        {
            usleep(1000);
            ++check;
        }

        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mLastMuxStateNotification, mux_state::MuxState::Label::Standby);
        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mMuxStateNotificationHandlerCalled, called);
    }
}

TEST_F(MuxPortTest, TestSwssLinkStateNotificationHandler)
{
    auto fakeLinkManagerStateMachinePtr = mFakeMuxPort.getFakeLinkManagerStateMachinePtr();
    for (uint i = 0, called = 0; i < 10000; ++i)
    {
        MUXLOGDEBUG(boost::format("Iteration %d") % i);
        mFakeMuxPort.handleLinkState("up");
        mFakeMuxPort.handleLinkState("down");
        called += 2;

        uint32_t check = 0;
        while ((fakeLinkManagerStateMachinePtr->mSwssLinkStateNotificationHandlerCalled < called) && (check < 10))
        {
            usleep(1000);
            ++check;
        }

        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mLastSwssLinkState, link_state::LinkState::Label::Down);
        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mSwssLinkStateNotificationHandlerCalled, called);
    }
}

TEST_F(MuxPortTest, TestPeerLinkStateNotificationHandler)
{
    auto fakeLinkManagerStateMachinePtr = mFakeMuxPort.getFakeLinkManagerStateMachinePtr();
    for (uint i = 0, called = 0; i < 10000; ++i)
    {
        MUXLOGDEBUG(boost::format("Iteration %d") % i);
        mFakeMuxPort.handlePeerLinkState("up");
        mFakeMuxPort.handlePeerLinkState("down");
        called += 2;

        uint32_t check = 0;
        while ((fakeLinkManagerStateMachinePtr->mPeerLinkStateNotificationHandlerCalled < called) && (check < 10))
        {
            usleep(1000);
            ++check;
        }

        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mLastPeerLinkState, link_state::LinkState::Label::Down);
        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mPeerLinkStateNotificationHandlerCalled, called);
    }
}

TEST_F(MuxPortTest, TestPeerMuxStateNotificationHandler)
{
    auto fakeLinkManagerStateMachinePtr = mFakeMuxPort.getFakeLinkManagerStateMachinePtr();
    for (uint i = 0, called = 0; i < 10000; ++i)
    {
        MUXLOGDEBUG(boost::format("Iteration %d") % i);
        mFakeMuxPort.handlePeerMuxState("active");
        mFakeMuxPort.handlePeerMuxState("standby");
        called += 2;

        uint32_t check = 0;
        while ((fakeLinkManagerStateMachinePtr->mPeerMuxStateNotificationHandlerCalled < called) && (check < 10))
        {
            usleep(1000);
            ++check;
        }

        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mLastPeerMuxState, mux_state::MuxState::Label::Standby);
        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mPeerMuxStateNotificationHandlerCalled, called);
    }
}

TEST_F(MuxPortTest, TestMuxConfigNotificationHandler)
{
    auto fakeLinkManagerStateMachinePtr = mFakeMuxPort.getFakeLinkManagerStateMachinePtr();
    for (uint i = 0, called = 0; i < 10000; ++i)
    {
        MUXLOGDEBUG(boost::format("Iteration %d") % i);
        mFakeMuxPort.handleMuxConfig("active");
        mFakeMuxPort.handleMuxConfig("standby");
        called += 2;

        uint32_t check = 0;
        while ((fakeLinkManagerStateMachinePtr->mMuxConfigNotificationHandlerCalled < called) && (check < 10))
        {
            usleep(1000);
            ++check;
        }

        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mLastMuxConfig, common::MuxPortConfig::Mode::Standby);
        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mMuxConfigNotificationHandlerCalled, called);
    }
}

TEST_F(MuxPortTest, TestDefaultRouteStateNotificationHandler)
{
    mMuxConfig.enableDefaultRouteFeature(true);
    auto fakeLinkManagerStateMachinePtr = mFakeMuxPort.getFakeLinkManagerStateMachinePtr();
    for (uint i = 0, called = 0; i < 10000; ++i)
    {
        MUXLOGDEBUG(boost::format("Iteration %d") % i);
        mFakeMuxPort.handleDefaultRouteState("na");
        mFakeMuxPort.handleDefaultRouteState("ok");
        called += 2;

        uint32_t check = 0;
        while ((fakeLinkManagerStateMachinePtr->mDefaultRouteStateNotificationHandlerCalled < called) && (check < 10))
        {
            usleep(1000);
            ++check;
        }

        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mLastDefaultRouteState, link_manager::LinkManagerStateMachineBase::DefaultRoute::OK);
        EXPECT_EQ(fakeLinkManagerStateMachinePtr->mDefaultRouteStateNotificationHandlerCalled, called);
    }
}

} /* namespace test */
