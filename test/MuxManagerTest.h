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
 * MuxManagerTest.h
 *
 *  Created on: Jun 4, 2021
 *      Author: Tamer Ahmed
 */

#ifndef MUXMANAGERTEST_H_
#define MUXMANAGERTEST_H_

#include <memory>
#include <tuple>
#include "gtest/gtest.h"

#include "FakeDbInterface.h"
#include "FakeLinkProber.h"
#include "NetMsgInterface.h"

namespace mux {
class MuxManager;
}

namespace test
{

const char* PortCableTypeValues[] = {"active-standby", "active-active"};

class MuxManagerTest: public testing::Test
{
public:
    MuxManagerTest();
    virtual ~MuxManagerTest() = default;

    void runIoService(uint32_t count = 1);
    void pollIoService(uint32_t count = 1);
    common::MuxPortConfig::Mode getMode(std::string port);
    uint32_t getPositiveStateChangeRetryCount(std::string port);
    uint32_t getNegativeStateChangeRetryCount(std::string port);
    uint32_t getTimeoutIpv4_msec(std::string port);
    uint32_t getTimeoutIpv6_msec(std::string port);
    uint32_t getLinkWaitTimeout_msec(std::string port);
    bool getIfUseWellKnownMac(std::string port);
    bool setUseWellKnownMacActiveActive(bool use);
    bool getIfUseToRMac(std::string port);
    boost::asio::ip::address getBladeIpv4Address(std::string port);
    std::array<uint8_t, ETHER_ADDR_LEN> getBladeMacAddress(std::string port);
    std::array<uint8_t, ETHER_ADDR_LEN> getLastUpdatedMacAddress(std::string port);
    std::array<uint8_t, ETHER_ADDR_LEN> getWellKnownMacAddress(std::string port);
    boost::asio::ip::address getLoopbackIpv4Address(std::string port);
    std::array<uint8_t, ETHER_ADDR_LEN> getTorMacAddress(std::string port);
    std::array<uint8_t, ETHER_ADDR_LEN> getVlanMacAddress(std::string port);
    common::MuxPortConfig::PortCableType getPortCableType(const std::string &port);
    void processMuxPortConfigNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);
    link_manager::LinkManagerStateMachineBase::CompositeState getCompositeStateMachineState(std::string port);
    void processServerIpAddress(std::vector<swss::KeyOpFieldsValuesTuple> &servers);
    void processSoCIpAddress(std::vector<swss::KeyOpFieldsValuesTuple> &servers);
    void processServerMacAddress(std::string port, std::array<char, MAX_ADDR_SIZE + 1> ip, std::array<char, MAX_ADDR_SIZE + 1> mac);
    void processLoopback2InterfaceInfo(std::vector<std::string> &loopbackIntfs);
    void processTorMacAddress(std::string &mac);
    void getVlanMacAddress(std::vector<std::string> &vlanNames);
    void processVlanMacAddress(std::string &mac);
    void processMuxResponseNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);
    void processMuxLinkmgrConfigNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);
    void updateServerMacAddress(boost::asio::ip::address serverIp, const uint8_t *serverMac);
    void processGetMuxState(const std::string &portName, const std::string &muxState);
    void updatePortCableType(const std::string &port, const std::string &cableType);
    void initLinkProberActiveActive(std::shared_ptr<link_manager::ActiveActiveStateMachine> linkManagerStateMachine);
    void initLinkProberActiveStandby(std::shared_ptr<link_manager::ActiveStandbyStateMachine> linkManagerStateMachine);
    void generateServerMac(const std::string &portName, std::array<uint8_t, ETHER_ADDR_LEN> &address);
    void createPort(std::string port, common::MuxPortConfig::PortCableType portCableType = common::MuxPortConfig::PortCableType::ActiveStandby);
    void warmRestartReconciliation(const std::string &portName);
    void updatePortReconciliationCount(int increment);
    void startWarmRestartReconciliationTimer(uint32_t timeout);
    void resetUpdateEthernetFrameFn(const std::string &portName);

public:
    static const std::string PortName;
    static const std::string ServerAddress;
    static const std::string SoCAddress;

public:
    std::shared_ptr<mux::MuxManager> mMuxManagerPtr;
    std::shared_ptr<FakeDbInterface> mDbInterfacePtr;
    mux::NetMsgInterface mNetMsgInterface;

    std::shared_ptr<FakeLinkProber> mFakeLinkProber;
};

class MuxResponseTest: public MuxManagerTest,
                       public testing::WithParamInterface<std::tuple<std::string, uint32_t, mux_state::MuxState::Label, common::MuxPortConfig::PortCableType>>
{
};

class GetMuxStateTest: public MuxManagerTest,
                       public testing::WithParamInterface<std::tuple<std::string, mux_state::MuxState::Label>>
{
};

class MuxConfigUpdateTest: public MuxManagerTest,
                           public testing::WithParamInterface<std::tuple<std::string, common::MuxPortConfig::Mode, common::MuxPortConfig::PortCableType>>
{
};

} /* namespace test */

#endif /* MUXMANAGERTEST_H_ */
