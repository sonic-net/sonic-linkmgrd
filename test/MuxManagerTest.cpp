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

const std::string MuxManagerTest::PortName = "Ethernet0";
const std::string MuxManagerTest::ServerAddress = "192.168.0.1";
const std::string MuxManagerTest::SoCAddress = "192.168.0.2";

MuxManagerTest::MuxManagerTest() :
    mMuxManagerPtr(std::make_shared<mux::MuxManager> ()),
    mDbInterfacePtr(std::make_shared<FakeDbInterface> (mMuxManagerPtr.get(), &mMuxManagerPtr->getIoService())),
    mNetMsgInterface(*mDbInterfacePtr)
{
    mMuxManagerPtr->setDbInterfacePtr(mDbInterfacePtr);

    //link_prober::IcmpPayload::generateGuid();
}

void MuxManagerTest::runIoService(uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        mMuxManagerPtr->getIoService().run_one();
        mMuxManagerPtr->getIoService().reset();
    }
}

void MuxManagerTest::pollIoService(uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        mMuxManagerPtr->getIoService().poll_one();
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

uint32_t MuxManagerTest::getLinkProberStatUpdateIntervalCount(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getLinkProberStatUpdateIntervalCount();
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

bool MuxManagerTest::getIfUseWellKnownMac(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getIfUseWellKnownMacActiveActive();
}

bool MuxManagerTest::setUseWellKnownMacActiveActive(bool use)
{
    mMuxManagerPtr->setUseWellKnownMacActiveActive(use);
}

bool MuxManagerTest::getIfUseToRMac(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.ifEnableUseTorMac();
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

std::array<uint8_t, ETHER_ADDR_LEN> MuxManagerTest::getLastUpdatedMacAddress(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getLastUpdatedMacAddress();
}

std::array<uint8_t, ETHER_ADDR_LEN> MuxManagerTest::getWellKnownMacAddress(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getWellKnownMacAddress();
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

std::array<uint8_t, ETHER_ADDR_LEN> MuxManagerTest::getVlanMacAddress(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getVlanMacAddress();
}

void MuxManagerTest::processMuxPortConfigNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    mDbInterfacePtr->processMuxPortConfigNotifiction(entries);
}

void MuxManagerTest::processTsaEnableNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    mDbInterfacePtr->processTsaEnableNotification(entries);
}

common::MuxPortConfig::LinkProberType MuxManagerTest::getLinkProberType(const std::string &port)
{
   std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

   return muxPortPtr->mMuxPortConfig.getLinkProberType();
}

link_manager::LinkManagerStateMachineBase::CompositeState MuxManagerTest::getCompositeStateMachineState(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->getLinkManagerStateMachinePtr()->getCompositeState();
}

common::MuxPortConfig::PortCableType MuxManagerTest::getPortCableType(const std::string &port)
{
    return mMuxManagerPtr->getMuxPortCableType(port);
}

void MuxManagerTest::processServerIpAddress(std::vector<swss::KeyOpFieldsValuesTuple> &servers)
{
    mDbInterfacePtr->processServerIpAddress(servers);
}

void MuxManagerTest::processSoCIpAddress(std::vector<swss::KeyOpFieldsValuesTuple> &servers)
{
    mDbInterfacePtr->processSoCIpAddress(servers);
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

void MuxManagerTest::getVlanMacAddress(std::vector<std::string> &vlanNames)
{
    mDbInterfacePtr->getVlanMacAddress(vlanNames);
}

void MuxManagerTest::processVlanMacAddress(std::string &mac)
{
    mDbInterfacePtr->processVlanMacAddress(mac);
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

void MuxManagerTest::updatePortCableType(const std::string &port, const std::string &cableType)
{
    mMuxManagerPtr->updatePortCableType(port, cableType);
}

void MuxManagerTest::updateLinkFailureDetectionState(const std::string &portName, const std::string
    &linkFailureDetectionState, const std::string &session_type)
{
    mMuxManagerPtr->updateLinkFailureDetectionState(portName, linkFailureDetectionState, session_type);
}

void MuxManagerTest::updateProberType(const std::string &portName, const std::string &proberType)
{
    mMuxManagerPtr->updateProberType(portName, proberType);
}

void MuxManagerTest::setTimeoutIpv4_msec(uint32_t timeout_msec)
{
    mMuxManagerPtr->setTimeoutIpv4_msec(timeout_msec);
}

void MuxManagerTest::setTimeoutIpv6_msec(uint32_t timeout_msec)
{
    mMuxManagerPtr->setTimeoutIpv6_msec(timeout_msec);
}

void MuxManagerTest::warmRestartReconciliation(const std::string &portName)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[portName];

    muxPortPtr->warmRestartReconciliation();
}

void MuxManagerTest::updatePortReconciliationCount(int increment)
{
    mMuxManagerPtr->updateWarmRestartReconciliationCount(increment);
    runIoService(1);
}

void MuxManagerTest::startWarmRestartReconciliationTimer(uint32_t timeout)
{
    mMuxManagerPtr->startWarmRestartReconciliationTimer(
        timeout
    );
}

void MuxManagerTest::postMetricsEvent(const std::string &portName, mux_state::MuxState::Label label)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[portName];

    return muxPortPtr->postMetricsEvent(link_manager::ActiveStandbyStateMachine::Metrics::SwitchingStart, label);
}

void MuxManagerTest::setMuxState(const std::string &portName, mux_state::MuxState::Label label)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[portName];

    return muxPortPtr->setMuxState(label);
}

void MuxManagerTest::initializeThread()
{
    for (uint8_t i = 0; i < 3; i++) {
        mMuxManagerPtr->mThreadGroup.create_thread(
            boost::bind(&boost::asio::io_service::run, &(mMuxManagerPtr->getIoService()))
        );
    }
}

void MuxManagerTest::terminate()
{
    mMuxManagerPtr->getIoService().stop();
    mMuxManagerPtr->mWork.~work();  // destructor is used to inform work is finished, only then run() is permitted to exit.
    mMuxManagerPtr->mThreadGroup.join_all();
}


void MuxManagerTest::initLinkProberActiveActive(std::shared_ptr<link_manager::ActiveActiveStateMachine> linkManagerStateMachineActiveActive)
{
    mFakeLinkProber = std::make_shared<FakeLinkProber> (linkManagerStateMachineActiveActive->getLinkProberStateMachinePtr().get());
    linkManagerStateMachineActiveActive->setInitializeProberFnPtr(
        boost::bind(&FakeLinkProber::initialize, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveActive->setStartProbingFnPtr(
        boost::bind(&FakeLinkProber::startProbing, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveActive->setUpdateEthernetFrameFnPtr(
        boost::bind(&FakeLinkProber::updateEthernetFrame, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveActive->setProbePeerTorFnPtr(
        boost::bind(&FakeLinkProber::probePeerTor, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveActive->setSuspendTxFnPtr(
        boost::bind(&FakeLinkProber::suspendTxProbes, mFakeLinkProber.get(), boost::placeholders::_1)
    );
    linkManagerStateMachineActiveActive->setResumeTxFnPtr(
        boost::bind(&FakeLinkProber::resumeTxProbes, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveActive->setShutdownTxFnPtr(
        boost::bind(&FakeLinkProber::shutdownTxProbes, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveActive->setRestartTxFnPtr(
        boost::bind(&FakeLinkProber::restartTxProbes, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveActive->setIcmpEchoSessionStateUpdate(
        boost::bind(&FakeLinkProber::handleStateDbStateUpdate, mFakeLinkProber.get())
    );

    linkManagerStateMachineActiveActive->mComponentInitState.set(0);
}

void MuxManagerTest::initLinkProberActiveStandby(std::shared_ptr<link_manager::ActiveStandbyStateMachine> linkManagerStateMachineActiveStandby)
{
    mFakeLinkProber = std::make_shared<FakeLinkProber> (linkManagerStateMachineActiveStandby->getLinkProberStateMachinePtr().get());
    linkManagerStateMachineActiveStandby->setInitializeProberFnPtr(
        boost::bind(&FakeLinkProber::initialize, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveStandby->setStartProbingFnPtr(
        boost::bind(&FakeLinkProber::startProbing, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveStandby->setUpdateEthernetFrameFnPtr(
        boost::bind(&FakeLinkProber::updateEthernetFrame, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveStandby->setSuspendTxFnPtr(
        boost::bind(&FakeLinkProber::suspendTxProbes, mFakeLinkProber.get(), boost::placeholders::_1)
    );
    linkManagerStateMachineActiveStandby->setShutdownTxFnPtr(
        boost::bind(&FakeLinkProber::shutdownTxProbes, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveStandby->setRestartTxFnPtr(
        boost::bind(&FakeLinkProber::restartTxProbes, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveStandby->setDecreaseIntervalFnPtr(
        boost::bind(&FakeLinkProber::decreaseProbeIntervalAfterSwitch, mFakeLinkProber.get(), boost::placeholders::_1)
    );
    linkManagerStateMachineActiveStandby->setRevertIntervalFnPtr(
        boost::bind(&FakeLinkProber::revertProbeIntervalAfterSwitchComplete, mFakeLinkProber.get())
    );
    linkManagerStateMachineActiveStandby->setSendPeerSwitchCommandFnPtr(
        boost::bind(&FakeLinkProber::sendPeerSwitchCommand, mFakeLinkProber.get())
    );

    linkManagerStateMachineActiveStandby->mComponentInitState.set(0);
}

void MuxManagerTest::generateServerMac(const std::string &port, std::array<uint8_t, ETHER_ADDR_LEN> &address)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];
    mMuxManagerPtr->generateServerMac(muxPortPtr->mMuxPortConfig.getServerId(), address);
}

void MuxManagerTest::createPort(std::string port, common::MuxPortConfig::PortCableType portCableType)
{
    EXPECT_TRUE(mMuxManagerPtr->mPortMap.size() == 0);
    EXPECT_TRUE(mMuxManagerPtr->mPortCableTypeMap.size() == 0);

    updatePortCableType(port, PortCableTypeValues[portCableType]);
    EXPECT_TRUE(mMuxManagerPtr->mPortCableTypeMap.size() == 1);

    std::deque<swss::KeyOpFieldsValuesTuple> entries = {
        {port, "SET", {{"oper_status", "up"}}},
    };

    mDbInterfacePtr->processLinkStateNotifiction(entries);
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];
    std::shared_ptr<link_manager::LinkManagerStateMachineBase> linkManagerStateMachine = muxPortPtr->getLinkManagerStateMachinePtr();

    EXPECT_TRUE(mMuxManagerPtr->mPortMap.size() == 1);
    EXPECT_TRUE(linkManagerStateMachine->mComponentInitState.test(link_manager::LinkManagerStateMachineBase::LinkStateComponent) == 0);

    runIoService();

    EXPECT_TRUE(linkManagerStateMachine->mComponentInitState.test(link_manager::LinkManagerStateMachineBase::LinkStateComponent) == 1);

    // Initialize a FakeLinkProber
    switch (portCableType) {
        case common::MuxPortConfig::PortCableType::ActiveActive: {
            initLinkProberActiveActive(std::dynamic_pointer_cast<link_manager::ActiveActiveStateMachine>(linkManagerStateMachine));
            break;
        }
        case common::MuxPortConfig::PortCableType::ActiveStandby: {
            initLinkProberActiveStandby(std::dynamic_pointer_cast<link_manager::ActiveStandbyStateMachine>(linkManagerStateMachine));
            break;
        }
        default:
            break;
    }

    std::string ipAddress = ServerAddress;
    std::string iPAddressSoC = SoCAddress;
    std::vector<swss::KeyOpFieldsValuesTuple> servers;
    if (portCableType == common::MuxPortConfig::PortCableType::ActiveActive) {
        servers = {
            {port, "SET", {{"server_ipv4", ipAddress + "/32"}, {"server_ipv6", "2603:10e1:100:f::1/128"}, {"soc_ipv4", iPAddressSoC + "/32"}, {"cable_type", "active-active"}}},
            {"Ethernet1234", "SET", {{"server_ipv4", "250.260.270.280/32"}}},
        };

    } else if (portCableType == common::MuxPortConfig::PortCableType::ActiveStandby) {
        servers = {
            {port, "SET", {{"server_ipv4", ipAddress + "/32"}, {"server_ipv6", "2603:10e1:100:f::1/128"}}},
            {"Ethernet1234", "SET", {{"server_ipv4", "250.260.270.280/32"}}},
        };
    }

    processServerIpAddress(servers);
    pollIoService();
    EXPECT_TRUE(mMuxManagerPtr->mPortMap.size() == 1);
    processSoCIpAddress(servers);
    pollIoService();
    EXPECT_TRUE(mMuxManagerPtr->mPortMap.size() == 1);


    entries.clear();
    entries = {
        {port, "SET", {{"state", "active"}}},
    };

    mDbInterfacePtr->processMuxStateNotifiction(entries);
    EXPECT_TRUE(mMuxManagerPtr->mPortMap.size() == 1);
    EXPECT_TRUE(linkManagerStateMachine->mComponentInitState.test(link_manager::LinkManagerStateMachineBase::MuxStateComponent) == 0);

    runIoService();

    EXPECT_TRUE(linkManagerStateMachine->mComponentInitState.test(link_manager::LinkManagerStateMachineBase::MuxStateComponent) == 1);
}

void MuxManagerTest::resetUpdateEthernetFrameFn(const std::string &portName)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[portName];
    std::shared_ptr<link_manager::LinkManagerStateMachineBase> linkManagerStateMachine = muxPortPtr->getLinkManagerStateMachinePtr();

    boost::function<void()> fnPtr = NULL;
    switch (getPortCableType(portName)) {
        case common::MuxPortConfig::PortCableType::ActiveActive: {
            (std::dynamic_pointer_cast<link_manager::ActiveActiveStateMachine>(linkManagerStateMachine))->setUpdateEthernetFrameFnPtr(fnPtr);
            break;
        }
        case common::MuxPortConfig::PortCableType::ActiveStandby: {
            (std::dynamic_pointer_cast<link_manager::ActiveStandbyStateMachine>(linkManagerStateMachine))->setUpdateEthernetFrameFnPtr(fnPtr);
            break;
        }
        default:
            break;
    }
}

uint32_t MuxManagerTest::getOscillationInterval_sec(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getOscillationInterval_sec();
}

bool MuxManagerTest::getOscillationEnabled(std::string port)
{
    std::shared_ptr<mux::MuxPort> muxPortPtr = mMuxManagerPtr->mPortMap[port];

    return muxPortPtr->mMuxPortConfig.getIfOscillationEnabled();
}

TEST_F(MuxManagerTest, UpdatePortCableTypeActiveStandby)
{
    std::string port = MuxManagerTest::PortName;
    updatePortCableType(port, "active-standby");
    EXPECT_TRUE(getPortCableType(port) == common::MuxPortConfig::PortCableType::ActiveStandby);
}

TEST_F(MuxManagerTest, UpdatePortCableTypeUnsupported)
{
    std::string port = MuxManagerTest::PortName;
    updatePortCableType(port, "active-standby-active");
    EXPECT_TRUE(getPortCableType(port) == common::MuxPortConfig::PortCableType::ActiveStandby);
}

TEST_F(MuxManagerTest, AddPort)
{
    std::string port = MuxManagerTest::PortName;
    std::string ipAddress = MuxManagerTest::ServerAddress;

    createPort(port);

    std::array<uint8_t, ETHER_ADDR_LEN> serverMac = {0, 'b', 2, 'd', 4, 'f'};
    boost::asio::ip::address serverAddress = boost::asio::ip::address::from_string(ipAddress);

    updateServerMacAddress(serverAddress, serverMac.data());

    runIoService();

    std::array<uint8_t, ETHER_ADDR_LEN> bladeMacAddress = getBladeMacAddress(port);

    EXPECT_TRUE(bladeMacAddress == serverMac);
    EXPECT_TRUE(getBladeIpv4Address(port).to_string() == ipAddress);
}

TEST_F(MuxManagerTest, AddPortActiveActive)
{
    std::string port = MuxManagerTest::PortName;
    std::string ipAddress = MuxManagerTest::ServerAddress;
    std::string ipAddressSoC = MuxManagerTest::ServerAddress;

    createPort(port, common::MuxPortConfig::ActiveActive);

    std::array<uint8_t, ETHER_ADDR_LEN> serverMac = {0, 'b', 2, 'd', 4, 'f'};
    boost::asio::ip::address serverAddress = boost::asio::ip::address::from_string(ipAddress);

    updateServerMacAddress(serverAddress, serverMac.data());

    runIoService();

    std::array<uint8_t, ETHER_ADDR_LEN> bladeMacAddress = getBladeMacAddress(port);
    EXPECT_TRUE(bladeMacAddress != serverMac);

    std::array<uint8_t, ETHER_ADDR_LEN> wellKnownMac;
    generateServerMac(port, wellKnownMac);
    EXPECT_TRUE(bladeMacAddress == wellKnownMac);
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
    EXPECT_TRUE(mMuxManagerPtr->getLoopbackIpv4Address() == getLoopbackIpv4Address(port));
}

TEST_F(MuxManagerTest, Loopback2AddressException)
{
    std::string port = "Ethernet0";

    createPort(port);

    std::string defaultAddress = "10.212.64.0";
    std::vector<std::string> loopbackIntfs = {
        "Loopback2|2603:10e1:100:d::1/128",
        "Loopback2|250.260.270.280/32",
        "Loopback2"
    };

    EXPECT_TRUE(getLoopbackIpv4Address(port).to_string() == defaultAddress);
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

TEST_F(MuxManagerTest, VlanMacAddress)
{
    std::string port = "Ethernet0";

    createPort(port);

    std::string mac = "00:aa:bb:cc:dd:ee";
    swss::MacAddress swssMacAddress(mac);
    std::array<uint8_t, ETHER_ADDR_LEN> macAddress;
    memcpy(macAddress.data(), swssMacAddress.getMac(), macAddress.size());

    processVlanMacAddress(mac);

    std::array<uint8_t, ETHER_ADDR_LEN> vlanMac = getVlanMacAddress(port);

    EXPECT_TRUE(vlanMac == macAddress);
}

TEST_F(MuxManagerTest, GetVlanMacAddressException)
{
    std::string port = "Ethernet0";

    createPort(port);

    EXPECT_TRUE(getIfUseToRMac(port) == false);

    std::vector<std::string> emptyVlanNameInput;
    getVlanMacAddress(emptyVlanNameInput);

    EXPECT_TRUE(getIfUseToRMac(port) == true);
}

TEST_F(MuxManagerTest, ProcessVlanMacAddressException)
{
    std::string port = "Ethernet0";

    createPort(port);

    EXPECT_TRUE(getIfUseToRMac(port) == false);

    std::string mac = "invalid mac";
    processVlanMacAddress(mac);

    EXPECT_TRUE(getIfUseToRMac(port) == true);
}

TEST_F(MuxManagerTest, SrcMacAddressUpdate)
{
    std::string port = "Etheret0";
    
    createPort(port);

    uint32_t updateEthernetFrameCallCountBefore = mFakeLinkProber->mUpdateEthernetFrameCallCount;
    
    std::deque<swss::KeyOpFieldsValuesTuple> entries = {
        {"LINK_PROBER", "SET", {{"src_mac", "ToRMac"}}}
    };
    processMuxLinkmgrConfigNotifiction(entries);

    runIoService();
    EXPECT_TRUE(mFakeLinkProber->mUpdateEthernetFrameCallCount == updateEthernetFrameCallCountBefore + 1);
}

TEST_P(OscillationIntervalTest, OscillationInterval)
{
    std::string port = "Ethernet0";
    createPort(port);

    std::deque<swss::KeyOpFieldsValuesTuple> entries = {
        {"TIMED_OSCILLATION", "SET", {{"interval_sec", std::get<0>(GetParam())}}}
    };
    processMuxLinkmgrConfigNotifiction(entries);

    EXPECT_TRUE(getOscillationInterval_sec(port) == std::get<1>(GetParam()));
}

INSTANTIATE_TEST_CASE_P(
    OscillationInterval,
    OscillationIntervalTest,
    ::testing::Values(
        std::make_tuple("1200", 1200),
        std::make_tuple("1", 300),
        std::make_tuple("invalid", 300)
    )
);

TEST_F(MuxManagerTest, OscillationDisabled)
{
    std::string port = "Ethernet0";
    createPort(port);

    std::deque<swss::KeyOpFieldsValuesTuple> entries = {
        {"TIMED_OSCILLATION", "SET", {{"oscillation_enabled", "false"}}}
    };
    processMuxLinkmgrConfigNotifiction(entries);

    EXPECT_TRUE(getOscillationEnabled(port) == false);
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

TEST_F(MuxManagerTest, updateLinkFailureDetectionState)
{
    std::string port = "Ethernet0";
    std::string linkFailureDetectionState = "Up";
    std::string session_type = "NORMAL";

    createPort(port, common::MuxPortConfig::ActiveActive);

    uint32_t handleStateDbStateUpdateCallCountBefore = mFakeLinkProber->mIcmpEchoSessionStateUpdateCallCount;

    updateLinkFailureDetectionState(port, linkFailureDetectionState, session_type);

    runIoService(1);

    EXPECT_EQ(mFakeLinkProber->mIcmpEchoSessionStateUpdateCallCount, handleStateDbStateUpdateCallCountBefore + 1);
}

TEST_F(MuxManagerTest, updateProberType)
{
    std::string port = "Ethernet0";
    std::string proberType = "hardware";

    createPort(port, common::MuxPortConfig::ActiveActive);

    updateProberType(port, proberType);

    runIoService(1);

    EXPECT_EQ(getLinkProberType(port), common::MuxPortConfig::LinkProberType::Hardware);
}

TEST_F(MuxManagerTest, TxIntervalChangeTest)
{
    std::string port = "Ethernet0";

    createPort(port);

    setTimeoutIpv4_msec(3000);

    EXPECT_EQ(3000,getTimeoutIpv4_msec(port));

    setTimeoutIpv6_msec(1000);

    EXPECT_EQ(1000,getTimeoutIpv6_msec(port));

}

TEST_F(MuxManagerTest, ServerMacBeforeLinkProberInit)
{
    std::string port = "Ethernet0";
    std::string ipAddress = "192.168.0.1";

    createPort(port);
    resetUpdateEthernetFrameFn(port);

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

TEST_F(MuxManagerTest, ServerMacActiveActive)
{
    std::string port = MuxManagerTest::PortName;
    std::string ipAddress = MuxManagerTest::ServerAddress;
    std::string ipAddressSoC = MuxManagerTest::ServerAddress;

    createPort(port, common::MuxPortConfig::ActiveActive);

    uint32_t updateEthernetFrameCallCountBefore = mFakeLinkProber->mUpdateEthernetFrameCallCount;

    std::array<uint8_t, ETHER_ADDR_LEN> serverMac = {0, 'b', 2, 'd', 4, 'f'};
    boost::asio::ip::address serverAddress = boost::asio::ip::address::from_string(ipAddress);
    updateServerMacAddress(serverAddress, serverMac.data());

    runIoService(1);

    std::array<uint8_t, ETHER_ADDR_LEN> bladeMacAddress = getBladeMacAddress(port);
    std::array<uint8_t, ETHER_ADDR_LEN> lastUpdatedMacAddress;
    EXPECT_TRUE(bladeMacAddress != serverMac);

    std::array<uint8_t, ETHER_ADDR_LEN> wellKnownMac;
    generateServerMac(port, wellKnownMac);
    EXPECT_TRUE(mFakeLinkProber->mUpdateEthernetFrameCallCount == updateEthernetFrameCallCountBefore);
    EXPECT_TRUE(bladeMacAddress == wellKnownMac);

    std::deque<swss::KeyOpFieldsValuesTuple> entries = {
        {"LINK_PROBER", "SET", {{"use_well_known_mac", "enable"}}}
    };
    processMuxLinkmgrConfigNotifiction(entries);
    updateServerMacAddress(serverAddress, serverMac.data());
    runIoService(2);

    bladeMacAddress = getBladeMacAddress(port);
    lastUpdatedMacAddress = getLastUpdatedMacAddress(port);
    EXPECT_TRUE(mFakeLinkProber->mUpdateEthernetFrameCallCount == updateEthernetFrameCallCountBefore + 1);
    EXPECT_TRUE(bladeMacAddress == wellKnownMac);
    EXPECT_TRUE(lastUpdatedMacAddress == serverMac);

    entries = {
        {"LINK_PROBER", "SET", {{"use_well_known_mac", "disable"}}}
    };
    processMuxLinkmgrConfigNotifiction(entries);
    runIoService(1);

    bladeMacAddress = getBladeMacAddress(port);
    lastUpdatedMacAddress = getLastUpdatedMacAddress(port);
    EXPECT_TRUE(mFakeLinkProber->mUpdateEthernetFrameCallCount == updateEthernetFrameCallCountBefore + 2);
    EXPECT_TRUE(bladeMacAddress == lastUpdatedMacAddress);
    EXPECT_TRUE(lastUpdatedMacAddress == serverMac);

    serverMac = {0, 'b', 2, 'd', 4, 'a'};
    updateServerMacAddress(serverAddress, serverMac.data());
    runIoService(1);

    bladeMacAddress = getBladeMacAddress(port);
    lastUpdatedMacAddress = getLastUpdatedMacAddress(port);
    EXPECT_TRUE(mFakeLinkProber->mUpdateEthernetFrameCallCount == updateEthernetFrameCallCountBefore + 3);
    EXPECT_TRUE(bladeMacAddress == lastUpdatedMacAddress);
    EXPECT_TRUE(lastUpdatedMacAddress == serverMac);

    entries = {
        {"LINK_PROBER", "SET", {{"use_well_known_mac", "enable"}}}
    };
    processMuxLinkmgrConfigNotifiction(entries);
    runIoService(1);

    bladeMacAddress = getBladeMacAddress(port);
    EXPECT_TRUE(mFakeLinkProber->mUpdateEthernetFrameCallCount == updateEthernetFrameCallCountBefore + 4);
    EXPECT_TRUE(bladeMacAddress == wellKnownMac);
}

TEST_F(MuxManagerTest, ServerMacActiveActiveBeforeLinkProberInit)
{
    std::string port = MuxManagerTest::PortName;
    std::string ipAddress = MuxManagerTest::ServerAddress;
    std::string ipAddressSoC = MuxManagerTest::ServerAddress;

    createPort(port, common::MuxPortConfig::ActiveActive);
    resetUpdateEthernetFrameFn(port);

    std::array<uint8_t, ETHER_ADDR_LEN> serverMac = {0, 'b', 2, 'd', 4, 'f'};
    boost::asio::ip::address serverAddress = boost::asio::ip::address::from_string(ipAddress);
    setUseWellKnownMacActiveActive(false);
    updateServerMacAddress(serverAddress, serverMac.data());

    runIoService(2);

    std::array<uint8_t, ETHER_ADDR_LEN> bladeMacAddress = getBladeMacAddress(port);
    EXPECT_TRUE(bladeMacAddress == serverMac);
}

TEST_F(MuxManagerTest, LinkmgrdConfig)
{
    std::string port = "Ethernet0";

    createPort(port);

    uint32_t v4PorbeInterval = 70;
    uint32_t v6ProveInterval = 700;
    uint32_t positiveSignalCount = 2;
    uint32_t negativeSignalCount = 3;
    uint32_t pckLossStatUpdateInterval = 900;
    uint32_t suspendTimer = 5;
    bool useWellKnownMac = true;
    bool useTorMacAddress = true; 
    std::deque<swss::KeyOpFieldsValuesTuple> entries = {
        {"LINK_PROBER", "SET", {{"interval_v4", boost::lexical_cast<std::string> (v4PorbeInterval)}}},
        {"LINK_PROBER", "SET", {{"interval_v6", boost::lexical_cast<std::string> (v6ProveInterval)}}},
        {"LINK_PROBER", "SET", {{"positive_signal_count", boost::lexical_cast<std::string> (positiveSignalCount)}}},
        {"LINK_PROBER", "SET", {{"negative_signal_count", boost::lexical_cast<std::string> (negativeSignalCount)}}},
        {"LINK_PROBER", "SET", {{"suspend_timer", boost::lexical_cast<std::string> (suspendTimer)}}},
        {"LINK_PROBER", "SET", {{"use_well_known_mac", "enable"}}},
        {"LINK_PROBER", "SET", {{"interval_v4", "abc"}}},
        {"LINK_PROBER", "SET", {{"src_mac", "ToRMac"}}},
        {"LINK_PROBER", "SET", {{"interval_pck_loss_count_update", "900"}}},
        {"LINK_PROBER", "SET", {{"reset_suspend_timer", "Ethernet0"}}},
        {"MUXLOGGER", "SET", {{"log_verbosity", "warning"}}},
    };
    EXPECT_EQ(mDbInterfacePtr->mUpdateIntervalV4Count, 0);
    EXPECT_EQ(mDbInterfacePtr->mUpdateIntervalV6Count, 0);
    processMuxLinkmgrConfigNotifiction(entries);
    runIoService(2);

    EXPECT_EQ(mDbInterfacePtr->mUpdateIntervalV4Count, 1);
    EXPECT_EQ(mDbInterfacePtr->mUpdateIntervalV6Count, 1);
    EXPECT_TRUE(getTimeoutIpv4_msec(port) == v4PorbeInterval);
    EXPECT_TRUE(getTimeoutIpv6_msec(port) == v6ProveInterval);
    EXPECT_TRUE(getPositiveStateChangeRetryCount(port) == positiveSignalCount);
    EXPECT_TRUE(getNegativeStateChangeRetryCount(port) == negativeSignalCount);
    EXPECT_TRUE(getLinkProberStatUpdateIntervalCount(port) == pckLossStatUpdateInterval);
    EXPECT_TRUE(getLinkWaitTimeout_msec(port) == (negativeSignalCount + 1) * v4PorbeInterval);
    EXPECT_TRUE(common::MuxLogger::getInstance()->getLevel() == boost::log::trivial::warning);
    EXPECT_TRUE(getIfUseWellKnownMac(port) == useWellKnownMac);
    EXPECT_TRUE(getIfUseToRMac(port) == useTorMacAddress);

    std::deque<swss::KeyOpFieldsValuesTuple> entry = {
        {"LINK_PROBER", "SET", {{"interval_pck_loss_count_update", "1"}}},
    };
    processMuxLinkmgrConfigNotifiction(entry);
    EXPECT_TRUE(getLinkProberStatUpdateIntervalCount(port) == 50);
}

TEST_P(MuxResponseTest, MuxResponse)
{
    std::string port = "Ethernet0";

    createPort(port, std::get<3> (GetParam()));

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
        std::make_tuple("active", 1, mux_state::MuxState::Label::Active, common::MuxPortConfig::PortCableType::ActiveStandby),
        std::make_tuple("standby", 3, mux_state::MuxState::Label::Standby, common::MuxPortConfig::PortCableType::ActiveStandby),
        std::make_tuple("unknown", 3, mux_state::MuxState::Label::Wait, common::MuxPortConfig::PortCableType::ActiveStandby),
        std::make_tuple("error", 3, mux_state::MuxState::Label::Wait, common::MuxPortConfig::PortCableType::ActiveStandby),
        std::make_tuple("active", 1, mux_state::MuxState::Label::Active, common::MuxPortConfig::PortCableType::ActiveActive),
        std::make_tuple("standby", 3, mux_state::MuxState::Label::Standby, common::MuxPortConfig::PortCableType::ActiveActive),
        std::make_tuple("unknown", 3, mux_state::MuxState::Label::Unknown, common::MuxPortConfig::PortCableType::ActiveActive),
        std::make_tuple("error", 3, mux_state::MuxState::Label::Unknown, common::MuxPortConfig::PortCableType::ActiveActive)
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
        std::make_tuple("active", mux_state::MuxState::Label::Wait),
        std::make_tuple("standby", mux_state::MuxState::Label::Wait),
        std::make_tuple("unknown", mux_state::MuxState::Label::Wait),
        std::make_tuple("error", mux_state::MuxState::Label::Wait)
    )
);

TEST_P(MuxConfigUpdateTest, MuxPortConfigUpdate)
{
    std::string port = "Ethernet0";

    createPort("Ethernet0", std::get<2> (GetParam()));

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
        std::make_tuple("auto", common::MuxPortConfig::Mode::Auto, common::MuxPortConfig::PortCableType::ActiveStandby),
        std::make_tuple("active", common::MuxPortConfig::Mode::Active, common::MuxPortConfig::PortCableType::ActiveStandby),
        std::make_tuple("standby", common::MuxPortConfig::Mode::Standby, common::MuxPortConfig::PortCableType::ActiveStandby),
        std::make_tuple("manual", common::MuxPortConfig::Mode::Manual, common::MuxPortConfig::PortCableType::ActiveStandby),
        std::make_tuple("auto", common::MuxPortConfig::Mode::Auto, common::MuxPortConfig::PortCableType::ActiveActive),
        std::make_tuple("active", common::MuxPortConfig::Mode::Active, common::MuxPortConfig::PortCableType::ActiveActive),
        std::make_tuple("standby", common::MuxPortConfig::Mode::Standby, common::MuxPortConfig::PortCableType::ActiveActive),
        std::make_tuple("manual", common::MuxPortConfig::Mode::Manual, common::MuxPortConfig::PortCableType::ActiveActive)
    )
);

TEST_F(MuxManagerTest, WarmRestart)
{
    std::string port = "Ethernet0";

    createPort(port);

    mDbInterfacePtr->mWarmStartFlag = true;
    startWarmRestartReconciliationTimer(UINT32_MAX);
    updatePortReconciliationCount(1);
    warmRestartReconciliation(port);

    runIoService(3);

    EXPECT_EQ(mDbInterfacePtr->mSetMuxModeInvokeCount, 1);
    EXPECT_EQ(mDbInterfacePtr->mSetWarmStartStateReconciledInvokeCount, 1);
}

TEST_F(MuxManagerTest, WarmRestartTimeout)
{
    std::string port = "Ethernet0";

    createPort(port);

    mDbInterfacePtr->mWarmStartFlag = true;
    startWarmRestartReconciliationTimer(mDbInterfacePtr->getWarmStartTimer());

    runIoService(1);
    EXPECT_EQ(mDbInterfacePtr->mSetWarmStartStateReconciledInvokeCount, 1);
}

TEST_F(MuxManagerTest, TsaEnable)
{
    createPort("Ethernet0");

    std::deque<swss::KeyOpFieldsValuesTuple> entries = {
        {"BGP_DEVICE_GLOBAL", "SET", {{"tsa_enabled", "true"}}},
    };
    processTsaEnableNotification(entries);
    runIoService();

    EXPECT_TRUE(getMode("Ethernet0") == common::MuxPortConfig::Mode::Standby);

    std::deque<swss::KeyOpFieldsValuesTuple> disable_entries = {
        {"BGP_DEVICE_GLOBAL", "SET", {{"tsa_enabled", "false"}}},
    };
    processTsaEnableNotification(disable_entries);
    runIoService();

    EXPECT_TRUE(getMode("Ethernet0") == common::MuxPortConfig::Mode::Auto);
}

TEST_F(MuxManagerTest, DbInterfaceRaceConditionCheck)
{
    createPort("Ethernet0");

    // create thread pool
    initializeThread();

    uint32_t TOGGLE_COUNT = 1000;

    for (uint32_t i=0; i<TOGGLE_COUNT; i++) {
        postMetricsEvent("Ethernet0", mux_state::MuxState::Label::Active);
        setMuxState("Ethernet0", mux_state::MuxState::Label::Active);

        // wait for handlers to be completed 
        while(mDbInterfacePtr->mSetMuxStateInvokeCount != i+1 || mDbInterfacePtr->mPostMetricsInvokeCount != i+1) {
            usleep(10000);
        }
        EXPECT_FALSE(mDbInterfacePtr->mDbInterfaceRaceConditionCheckFailure);
    }

    terminate();
}

} /* namespace test */
