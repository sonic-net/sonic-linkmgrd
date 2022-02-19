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
 * MuxManagerTest.cpp
 *
 *  Created on: Jun 4, 2021
 *      Author: Tamer Ahmed
 */

#include "common/MuxException.h"
#include "swss/macaddress.h"

#include "MuxManager.h"
#include "MuxManagerTest.h"

namespace test
{

MuxManagerTest::MuxManagerTest() :
    mMuxManagerPtr(std::make_shared<mux::MuxManager> ()),
    mDbInterfacePtr(std::make_shared<FakeDbInterface> (mMuxManagerPtr.get(), &mMuxManagerPtr->getIoService())),
    mNetMsgInterface(*mDbInterfacePtr)
{
    mMuxManagerPtr->setDbInterfacePtr(mDbInterfacePtr);

    link_prober::IcmpPayload::generateGuid();
    link_manager::LinkManagerStateMachine::initializeTransitionFunctionTable();
}

void MuxManagerTest::runIoService(uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        mMuxManagerPtr->getIoService().run_one();
        mMuxManagerPtr->getIoService().reset();
    }
}

common::MuxPortConfig::Mode MuxManagerTest::getMode(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getMode();
}

uint32_t MuxManagerTest::getPositiveStateChangeRetryCount(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getPositiveStateChangeRetryCount();
}

uint32_t MuxManagerTest::getNegativeStateChangeRetryCount(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getNegativeStateChangeRetryCount();
}

uint32_t MuxManagerTest::getTimeoutIpv4_msec(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];
    return muxPortPtr->mMuxPortConfig.getTimeoutIpv4_msec();
}

uint32_t MuxManagerTest::getTimeoutIpv6_msec(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getTimeoutIpv6_msec();
}

uint32_t MuxManagerTest::getLinkWaitTimeout_msec(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getLinkWaitTimeout_msec();
}

boost::asio::ip::address MuxManagerTest::getBladeIpv4Address(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getBladeIpv4Address();
}

std::array<uint8_t, ETHER_ADDR_LEN> MuxManagerTest::getBladeMacAddress(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getBladeMacAddress();
}

boost::asio::ip::address MuxManagerTest::getLoopbackIpv4Address(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getLoopbackIpv4Address();
}

std::array<uint8_t, ETHER_ADDR_LEN> MuxManagerTest::getTorMacAddress(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getTorMacAddress();
}

void MuxManagerTest::processMuxPortConfigNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    mDbInterfacePtr->processMuxPortConfigNotifiction(entries);
}

link_manager::LinkManagerStateMachine::CompositeState MuxManagerTest::getCompositeStateMachineState(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->getLinkManagerStateMachine()->getCompositeState();
}

void MuxManagerTest::processServerIpAddress(std::vector<swss::KeyOpFieldsValuesTuple> &servers)
{
    mDbInterfacePtr->processServerIpAddress(servers);

    EXPECT_TRUE(mMuxManagerPtr->mPortMap.size() == 1);
}

void MuxManagerTest::processServerMacAddress(
    std::string port,
    std::array<char, MAX_ADDR_SIZE + 1> ip,
    std::array<char, MAX_ADDR_SIZE + 1> mac
)
{
    mNetMsgInterface.updateMacAddress(port, ip, mac);
}

void MuxManagerTest::processLoopback2InterfaceInfo(std::vector<std::string> &loopbackIntfs)
{
    mDbInterfacePtr->processLoopback2InterfaceInfo(loopbackIntfs);
}

void MuxManagerTest::processTorMacAddress(std::string &mac)
{
    mDbInterfacePtr->processTorMacAddress(mac);
}

void MuxManagerTest::processMuxResponseNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    mDbInterfacePtr->processMuxResponseNotifiction(entries);
}

void MuxManagerTest::processMuxLinkmgrConfigNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    mDbInterfacePtr->processMuxLinkmgrConfigNotifiction(entries);
}

void MuxManagerTest::updateServerMacAddress(boost::asio::ip::address serverIp, const uint8_t *serverMac)
{
    mDbInterfacePtr->updateServerMacAddress(serverIp, serverMac);
}

void MuxManagerTest::processGetMuxState(const std::string &portName, const std::string &muxState)
{
    mMuxManagerPtr->processGetMuxState(portName, muxState);
}

void MuxManagerTest::createPort(std::string port)
{
    EXPECT_TRUE(mMuxManagerPtr->mPortMap.size() == 0);

    std::deque<swss::KeyOpFieldsValuesTuple> entries = {
        {port, "SET", {{"oper_status", "up"}}},
    };

    mDbInterfacePtr->processLinkStateNotifiction(entries);
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap["Ethernet0"];
    link_manager::LinkManagerStateMachine* linkManagerStateMachine = muxPortPtr->getLinkManagerStateMachine();

    EXPECT_TRUE(mMuxManagerPtr->mPortMap.size() == 1);
    EXPECT_TRUE(linkManagerStateMachine->mComponentInitState.test(link_manager::LinkManagerStateMachine::LinkStateComponent) == 0);

    runIoService();

    EXPECT_TRUE(linkManagerStateMachine->mComponentInitState.test(link_manager::LinkManagerStateMachine::LinkStateComponent) == 1);

    // Initialize a FakeLinkProber
    mFakeLinkProber = std::make_shared<FakeLinkProber> (&linkManagerStateMachine->getLinkProberStateMachine());
    linkManagerStateMachine->setInitializeProberFnPtr(
        boost::bind(&FakeLinkProber::initialize, mFakeLinkProber.get())
    );
    linkManagerStateMachine->setStartProbingFnPtr(
        boost::bind(&FakeLinkProber::startProbing, mFakeLinkProber.get())
    );
    linkManagerStateMachine->setUpdateEthernetFrameFnPtr(
        boost::bind(&FakeLinkProber::updateEthernetFrame, mFakeLinkProber.get())
    );
    linkManagerStateMachine->setSuspendTxFnPtr(
        boost::bind(&FakeLinkProber::suspendTxProbes, mFakeLinkProber.get(), boost::placeholders::_1)
    );

    linkManagerStateMachine->mComponentInitState.set(0);

    std::string ipAddress = "192.168.0.1";
    std::vector<swss::KeyOpFieldsValuesTuple> servers;
    servers = {
        {port, "SET", {{"server_ipv4", ipAddress + "/32"}, {"server_ipv6", "2603:10e1:100:f::1/128"}}},
        {"Ethernet1234", "SET", {{"server_ipv4", "250.260.270.280/32"}}},
    };

    processServerIpAddress(servers);

    runIoService();

    entries.clear();
    entries = {
        {port, "SET", {{"state", "active"}}},
    };

    mDbInterfacePtr->processMuxStateNotifiction(entries);
    EXPECT_TRUE(mMuxManagerPtr->mPortMap.size() == 1);
    EXPECT_TRUE(linkManagerStateMachine->mComponentInitState.test(link_manager::LinkManagerStateMachine::MuxStateComponent) == 0);

    runIoService();

    EXPECT_TRUE(linkManagerStateMachine->mComponentInitState.test(link_manager::LinkManagerStateMachine::MuxStateComponent) == 1);
}

TEST_F(MuxManagerTest, AddPort)
{
    std::string port = "Ethernet0";
    std::string ipAddress = "192.168.0.1";

    createPort(port);

    std::array<uint8_t, ETHER_ADDR_LEN> serverMac = {0, 'b', 2, 'd', 4, 'f'};
    boost::asio::ip::address serverAddress = boost::asio::ip::address::from_string(ipAddress);

    updateServerMacAddress(serverAddress, serverMac.data());

    runIoService();

    std::array<uint8_t, ETHER_ADDR_LEN> bladeMacAddress = getBladeMacAddress(port);

    EXPECT_TRUE(bladeMacAddress == serverMac);
    EXPECT_TRUE(getBladeIpv4Address(port).to_string() == ipAddress);
}

TEST_F(MuxManagerTest, Loopback2Address)
{
    std::string port = "Ethernet0";

    createPort(port);

    std::string ipAddress = "10.10.10.2";
    std::vector<std::string> loopbackIntfs = {
        "Loopback2|2603:10e1:100:d::1/128",
        "Loopback2|" + ipAddress + "/32",
        "Loopback2"
    };

    processLoopback2InterfaceInfo(loopbackIntfs);

    EXPECT_TRUE(getLoopbackIpv4Address(port).to_string() == ipAddress);
}

TEST_F(MuxManagerTest, Loopback2AddressException)
{
    std::string port = "Ethernet0";

    createPort(port);

    std::vector<std::string> loopbackIntfs = {
        "Loopback2|2603:10e1:100:d::1/128",
        "Loopback2|250.260.270.280/32",
        "Loopback2"
    };

    EXPECT_THROW(processLoopback2InterfaceInfo(loopbackIntfs), common::ConfigNotFoundException);
}

TEST_F(MuxManagerTest, ToRMacAddress)
{
    std::string port = "Ethernet0";

    createPort(port);

    std::string mac = "0a:b1:2c:d3:4e:f5";
    swss::MacAddress swssMacAddress(mac);
    std::array<uint8_t, ETHER_ADDR_LEN> macAddress;
    memcpy(macAddress.data(), swssMacAddress.getMac(), macAddress.size());

    processTorMacAddress(mac);

    std::array<uint8_t, ETHER_ADDR_LEN> torMac = getTorMacAddress(port);

    EXPECT_TRUE(torMac == macAddress);
}

TEST_F(MuxManagerTest, ToRMacAddressException)
{
    std::string port = "Ethernet0";

    createPort(port);

    std::string mac = "invalid mac";

    EXPECT_THROW(processTorMacAddress(mac), common::ConfigNotFoundException);
}

TEST_F(MuxManagerTest, ServerMacAddress)
{
    std::string port = "Ethernet0";
    std::string ipAddress = "192.168.0.1";

    createPort(port);

    std::string mac = "a0:1b:c2:3d:e4:5f";
    std::array<char, MAX_ADDR_SIZE + 1> macAddress = {0};
    memcpy(macAddress.data(), mac.c_str(), mac.size());
    std::array<char, MAX_ADDR_SIZE + 1> serverIpAddress = {0};
    memcpy(serverIpAddress.data(), ipAddress.c_str(), ipAddress.size());

    processServerMacAddress(port, serverIpAddress, macAddress);

    runIoService();

    std::array<uint8_t, ETHER_ADDR_LEN> serverMac = getBladeMacAddress(port);

    swss::MacAddress swssMacAddress(mac);
    std::array<uint8_t, ETHER_ADDR_LEN> expectedMac;
    memcpy(expectedMac.data(), swssMacAddress.getMac(), expectedMac.size());

    EXPECT_TRUE(serverMac == expectedMac);
}

TEST_F(MuxManagerTest, ServerMacAddressException)
{
    std::string port = "Ethernet0";
    std::string ipAddress = "192.168.0.1";

    createPort(port);

    std::string mac = "invalid mac";
    std::array<char, MAX_ADDR_SIZE + 1> macAddress = {0};
    memcpy(macAddress.data(), mac.c_str(), mac.size());
    std::array<char, MAX_ADDR_SIZE + 1> serverIpAddress = {0};
    memcpy(serverIpAddress.data(), ipAddress.c_str(), ipAddress.size());

    std::array<uint8_t, ETHER_ADDR_LEN> serverMacBefore = getBladeMacAddress(port);

    processServerMacAddress(port, serverIpAddress, macAddress);;

    std::array<uint8_t, ETHER_ADDR_LEN> serverMacAfter = getBladeMacAddress(port);

    EXPECT_TRUE(serverMacBefore == serverMacAfter);
}

TEST_F(MuxManagerTest, LinkmgrdConfig)
{
    std::string port = "Ethernet0";

    createPort(port);

    uint32_t v4PorbeInterval = 70;
    uint32_t v6ProveInterval = 700;
    uint32_t positiveSignalCount = 2;
    uint32_t negativeSignalCount = 3;
    uint32_t suspendTimer = 5;
    std::deque<swss::KeyOpFieldsValuesTuple> entries = {
        {"LINK_PROBER", "SET", {{"interval_v4", boost::lexical_cast<std::string> (v4PorbeInterval)}}},
        {"LINK_PROBER", "SET", {{"interval_v6", boost::lexical_cast<std::string> (v6ProveInterval)}}},
        {"LINK_PROBER", "SET", {{"positive_signal_count", boost::lexical_cast<std::string> (positiveSignalCount)}}},
        {"LINK_PROBER", "SET", {{"negative_signal_count", boost::lexical_cast<std::string> (negativeSignalCount)}}},
        {"LINK_PROBER", "SET", {{"suspend_timer", boost::lexical_cast<std::string> (suspendTimer)}}},
        {"LINK_PROBER", "SET", {{"interval_v4", "abc"}}},
        {"MUXLOGGER", "SET", {{"log_verbosity", "warning"}}},
    };
    processMuxLinkmgrConfigNotifiction(entries);

    EXPECT_TRUE(getTimeoutIpv4_msec(port) == v4PorbeInterval);
    EXPECT_TRUE(getTimeoutIpv6_msec(port) == v6ProveInterval);
    EXPECT_TRUE(getPositiveStateChangeRetryCount(port) == positiveSignalCount);
    EXPECT_TRUE(getNegativeStateChangeRetryCount(port) == negativeSignalCount);
    EXPECT_TRUE(getLinkWaitTimeout_msec(port) == (negativeSignalCount + 1) * v4PorbeInterval);
    EXPECT_TRUE(common::MuxLogger::getInstance()->getLevel() == boost::log::trivial::warning);
}

TEST_P(MuxResponseTest, MuxResponse)
{
    std::string port = "Ethernet0";

    createPort(port);

    std::deque<swss::KeyOpFieldsValuesTuple> entries = {
        {port, "SET", {{"response", std::get<0> (GetParam())}}},
    };
    processMuxResponseNotifiction(entries);

    runIoService(std::get<1> (GetParam()));

    EXPECT_TRUE(ms(getCompositeStateMachineState(port)) == std::get<2> (GetParam()));
}

INSTANTIATE_TEST_CASE_P(
    MuxState,
    MuxResponseTest,
    ::testing::Values(
        std::make_tuple("active", 1, mux_state::MuxState::Label::Active),
        std::make_tuple("standby", 3, mux_state::MuxState::Label::Standby),
        std::make_tuple("unknown", 3, mux_state::MuxState::Label::Wait),
        std::make_tuple("error", 3, mux_state::MuxState::Label::Wait)
    )
);

TEST_P(GetMuxStateTest, GetMuxState)
{
    std::string port = "Ethernet0";

    createPort(port);

    std::string state = std::get<0> (GetParam());

    processGetMuxState(port, state);

    runIoService();

    EXPECT_TRUE(ms(getCompositeStateMachineState(port)) == std::get<1> (GetParam()));
}

INSTANTIATE_TEST_CASE_P(
    MuxState,
    GetMuxStateTest,
    ::testing::Values(
        std::make_tuple("active", mux_state::MuxState::Label::Active),
        std::make_tuple("standby", mux_state::MuxState::Label::Wait),
        std::make_tuple("unknown", mux_state::MuxState::Label::Wait),
        std::make_tuple("error", mux_state::MuxState::Label::Wait)
    )
);

TEST_P(MuxConfigUpdateTest, MuxPortConfigUpdate)
{
    std::string port = "Ethernet0";

    createPort("Ethernet0");

    std::string state = std::get<0> (GetParam());
    std::deque<swss::KeyOpFieldsValuesTuple> entries = {
        {port, "SET", {{"state", state}}},
    };
    processMuxPortConfigNotifiction(entries);
    runIoService();

    EXPECT_TRUE(getMode("Ethernet0") == std::get<1> (GetParam()));
}

INSTANTIATE_TEST_CASE_P(
    AutoActiveManual,
    MuxConfigUpdateTest,
    ::testing::Values(
        std::make_tuple("auto", common::MuxPortConfig::Mode::Auto),
        std::make_tuple("active", common::MuxPortConfig::Mode::Active),
        std::make_tuple("manual", common::MuxPortConfig::Mode::Manual)
    )
);

} /* namespace test */
