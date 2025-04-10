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
 * DbInterface.cpp
 *
 *  Created on: Oct 23, 2020
 *      Author: Tamer Ahmed
 */

#include <algorithm>
#include <tuple>
#include <vector>
#include <functional>

#include <boost/algorithm/string.hpp>
#include <boost/bind/bind.hpp>
#include <boost/date_time/gregorian/gregorian.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>

#include "swss/netdispatcher.h"
#include "swss/netlink.h"
#include "swss/macaddress.h"
#include "swss/select.h"

#include "DbInterface.h"
#include "MuxManager.h"
#include "common/MuxLogger.h"
#include "common/MuxException.h"
#include "NetMsgInterface.h"

namespace mux
{
constexpr auto DEFAULT_TIMEOUT_MSEC = 1000;
std::vector<std::string> DbInterface::mMuxState = {"active", "standby", "unknown", "Error"};
std::vector<std::string> DbInterface::mMuxLinkmgrState = {"uninitialized", "unhealthy", "healthy"};
std::vector<std::string> DbInterface::mMuxMetrics = {"start", "end"};
std::vector<std::string> DbInterface::mLinkProbeMetrics = {"link_prober_unknown_start", "link_prober_unknown_end", "link_prober_wait_start", "link_prober_active_start", "link_prober_standby_start"};
std::vector<std::string> DbInterface::mActiveStandbySwitchCause = {"Peer_Heartbeat_Missing" , "Peer_Link_Down" , "Tlv_Switch_Active_Command" , "Link_Down" , "Transceiver_Daemon_Timeout" , "Matching_Hardware_State" , "Config_Mux_Mode", "Hardware_State_Unknown", "Timed_Oscillation"};

//
// ---> DbInterface(mux::MuxManager *muxManager);
//
// class constructor
//
DbInterface::DbInterface(mux::MuxManager *muxManager, boost::asio::io_service *ioService) :
    mMuxManagerPtr(muxManager),
    mBarrier(2),
    mStrand(*ioService)
{
}

// GCOVR_EXCL_START

//
// ---> getMuxState(const std::string &portName);
//
// retrieve the current MUX state
//
void DbInterface::getMuxState(const std::string &portName)
{
    MUXLOGDEBUG(portName);

    boost::asio::post(mStrand, boost::bind(
        &DbInterface::handleGetMuxState,
        this,
        portName
    ));
}

//
// ---> setMuxState(const std::string &portName, mux_state::MuxState::Label label);
//
// set MUX state in APP DB for orchagent processing
//
void DbInterface::setMuxState(const std::string &portName, mux_state::MuxState::Label label)
{
    MUXLOGDEBUG(boost::format("%s: setting mux to %s") % portName % mMuxState[label]);

    boost::asio::post(mStrand, boost::bind(
        &DbInterface::handleSetMuxState,
        this,
        portName,
        label
    ));
}

//
// ---> setPeerMuxState(const std::string &portName, mux_state::MuxState::Label label);
//
// set MUX state in APP DB for orchagent processing
//
void DbInterface::setPeerMuxState(const std::string &portName, mux_state::MuxState::Label label)
{
    MUXLOGDEBUG(boost::format("%s: setting peer mux to %s") % portName % mMuxState[label]);

    boost::asio::post(mStrand, boost::bind(
        &DbInterface::handleSetPeerMuxState,
        this,
        portName,
        label
    ));
}


//
// ---> probeMuxState(const std::string &portName)
//
// trigger xcvrd to read MUX state using i2c
//
void DbInterface::probeMuxState(const std::string &portName)
{
    MUXLOGDEBUG(portName);

    boost::asio::post(mStrand, boost::bind(
        &DbInterface::handleProbeMuxState,
        this,
        portName
    ));
}

//
// ---> probeForwardingState(const std::string &portName)
//
// trigger tranceiver daemon to read Fowarding state using gRPC
//
void DbInterface::probeForwardingState(const std::string &portName)
{
    MUXLOGDEBUG(portName);

    boost::asio::post(mStrand, boost::bind(
        &DbInterface::handleProbeForwardingState,
        this,
        portName
    ));
}

//
// ---> setMuxLinkmgrState(const std::string &portName, link_manager::ActiveStandbyStateMachine::Label label);
//
// set MUX LinkMgr state in State DB for cli processing
//
void DbInterface::setMuxLinkmgrState(const std::string &portName, link_manager::ActiveStandbyStateMachine::Label label)
{
    MUXLOGDEBUG(boost::format("%s: setting mux linkmgr to %s") % portName % mMuxLinkmgrState[static_cast<int> (label)]);

    boost::asio::post(mStrand, boost::bind(
        &DbInterface::handleSetMuxLinkmgrState,
        this,
        portName,
        label
    ));
}

//
// ---> postMetricsEvent(
//          const std::string &portName,
//          link_manager::ActiveStandbyStateMachine::Metrics metrics
//          mux_state::MuxState::Label label);
//
// post MUX metrics event
//
void DbInterface::postMetricsEvent(
    const std::string &portName,
    link_manager::ActiveStandbyStateMachine::Metrics metrics,
    mux_state::MuxState::Label label
)
{
    MUXLOGDEBUG(boost::format("%s: posting mux metrics event linkmgrd_switch_%s_%s") %
        portName %
        mMuxState[label] %
        mMuxMetrics[static_cast<int> (metrics)]
    );

    boost::asio::post(mStrand, boost::bind(
        &DbInterface::handlePostMuxMetrics,
        this,
        portName,
        metrics,
        label,
        boost::posix_time::microsec_clock::universal_time()
    ));
}

//
// ---> void postSwitchCause(
//         const std::string &portName,
//         link_manager::ActiveStandbyStateMachine::SwitchCause cause)
//     );
//
// post switch cause
//
void DbInterface::postSwitchCause(
        const std::string &portName,
        link_manager::ActiveStandbyStateMachine::SwitchCause cause
)
{
    MUXLOGDEBUG(boost::format("%s: post switch cause %s") %
        portName %
        mActiveStandbySwitchCause[static_cast<int>(cause)]
    );

    boost::asio::post(mStrand, boost::bind(
        &DbInterface::handlePostSwitchCause,
        this,
        portName,
        cause,
        boost::posix_time::microsec_clock::universal_time()
    ));
}

//
// ---> void handlePostSwitchCause(
//          const std::string portName,
//          link_manager::ActiveStandbyStateMachine::SwitchCause cause,
//          boost::posix_time::ptime time
//      );
//
// post switch cause to state db 
//
void DbInterface::handlePostSwitchCause(
        const std::string &portName,
        link_manager::ActiveStandbyStateMachine::SwitchCause cause,
        boost::posix_time::ptime time
)
{
    MUXLOGWARNING(boost::format("%s: post last switch cause %s") %
        portName %
        mActiveStandbySwitchCause[static_cast<int>(cause)]
    );

    mStateDbSwitchCauseTablePtr->hset(portName, "cause", mActiveStandbySwitchCause[static_cast<int>(cause)]);
    mStateDbSwitchCauseTablePtr->hset(portName, "time", boost::posix_time::to_simple_string(time));
}


// 
// ---> postLinkProberMetricsEvent(
//        const std::string &portName, 
//        link_manager::ActiveStandbyStateMachine::LinkProberMetrics metrics
//    );
//
// post link probe event to state db 
void DbInterface::postLinkProberMetricsEvent(
        const std::string &portName, 
        link_manager::ActiveStandbyStateMachine::LinkProberMetrics metrics
)
{
    MUXLOGWARNING(boost::format("%s: posting link prober event %s") %
        portName %
        mLinkProbeMetrics[static_cast<int> (metrics)]
    );

    boost::asio::post(mStrand, boost::bind(
        &DbInterface::handlePostLinkProberMetrics,
        this,
        portName,
        metrics,
        boost::posix_time::microsec_clock::universal_time()
    ));
}

//
// ---> postPckLossRatio(
//        const std::string &portName,
//        const uint64_t unknownEventCount, 
//        const uint64_t expectedPacketCount
//    );
//  post pck loss ratio update to state db 
void DbInterface::postPckLossRatio(
        const std::string &portName,
        const uint64_t unknownEventCount, 
        const uint64_t expectedPacketCount
)
{
    MUXLOGDEBUG(boost::format("%s: posting pck loss ratio, pck_loss_count / pck_expected_count : %d / %d") %
        portName %
        unknownEventCount % 
        expectedPacketCount
    );

    boost::asio::post(mStrand,boost::bind(
        &DbInterface::handlePostPckLossRatio,
        this,
        portName,
        unknownEventCount,
        expectedPacketCount
    ));
}

//
// ---> initialize();
//
// initialize DB tables and start SWSS listening thread
//
void DbInterface::initialize()
{
    try {
        mAppDbPtr = std::make_shared<swss::DBConnector> ("APPL_DB", 0);
        mStateDbPtr = std::make_shared<swss::DBConnector> ("STATE_DB", 0);

        mAppDbMuxTablePtr = std::make_shared<swss::ProducerStateTable> (
            mAppDbPtr.get(), APP_MUX_CABLE_TABLE_NAME
        );
        mAppDbPeerMuxTablePtr = std::make_shared<swss::Table> (
            mAppDbPtr.get(), APP_PEER_HW_FORWARDING_STATE_TABLE_NAME
        );
        mAppDbMuxCommandTablePtr = std::make_shared<swss::Table> (
            mAppDbPtr.get(), APP_MUX_CABLE_COMMAND_TABLE_NAME
        );
        mAppDbForwardingCommandTablePtr = std::make_shared<swss::Table> (
            mAppDbPtr.get(), APP_FORWARDING_STATE_COMMAND_TABLE_NAME
        );
        mAppDbIcmpEchoSessionTablePtr = std::make_shared<swss::ProducerStateTable> (
            mAppDbPtr.get(), APP_ICMP_ECHO_SESSION_TABLE_NAME
        );
        mStateDbMuxLinkmgrTablePtr = std::make_shared<swss::Table> (
            mStateDbPtr.get(), STATE_MUX_LINKMGR_TABLE_NAME
        );
        mStateDbMuxMetricsTablePtr = std::make_shared<swss::Table> (
            mStateDbPtr.get(), STATE_MUX_METRICS_TABLE_NAME
        );
        mStateDbLinkProbeStatsTablePtr = std::make_shared<swss::Table> (
            mStateDbPtr.get(), LINK_PROBE_STATS_TABLE_NAME
        );
        mStateDbSwitchCauseTablePtr = std::make_shared<swss::Table> (
            mStateDbPtr.get(),  STATE_MUX_SWITCH_CAUSE_TABLE_NAME
        );
        mStateDbIcmpEchoSessionTablePtr = std::make_shared<swss::Table> (
            mStateDbPtr.get(),  STATE_ICMP_ECHO_SESSION_TABLE_NAME
        );
        mMuxStateTablePtr = std::make_shared<swss::Table> (mStateDbPtr.get(), STATE_MUX_CABLE_TABLE_NAME);

        mSwssThreadPtr = std::make_shared<boost::thread> (&DbInterface::handleSwssNotification, this);
    }
    catch (const std::bad_alloc &ex) {
        std::ostringstream errMsg;
        errMsg << "Failed allocate memory. Exception details: " << ex.what();

        throw MUX_ERROR(BadAlloc, errMsg.str());
    }
}

//
// ---> deinitialize();
//
// deinitialize DB interface and join SWSS listening thread
//
void DbInterface::deinitialize()
{
    mSwssThreadPtr->join();
}

//
// ---> updateServerMacAddress(boost::asio::ip::address serverIp, uint8_t *serverMac);
//
// Update Server MAC address behind a MUX port
//
void DbInterface::updateServerMacAddress(boost::asio::ip::address serverIp, const uint8_t *serverMac)
{
    MUXLOGDEBUG(boost::format("server IP: %s") % serverIp.to_string());

    ServerIpPortMap::const_iterator cit = mServerIpPortMap.find(serverIp);
    if (cit != mServerIpPortMap.cend()) {
        std::array<uint8_t, ETHER_ADDR_LEN> macAddress;

        memcpy(macAddress.data(), serverMac, macAddress.size());

        mMuxManagerPtr->processGetServerMacAddress(cit->second, macAddress);
    }
}

//
// ---> handleGetMuxState(const std::string portName);
//
// get state db MUX state
//
void DbInterface::handleGetMuxState(const std::string portName)
{
    MUXLOGDEBUG(portName);

    std::string state;
    if (mMuxStateTablePtr->hget(portName, "state", state)) {
        mMuxManagerPtr->processGetMuxState(portName, state);
    }
}

//
// ---> handleSetMuxState(const std::string portName, mux_state::MuxState::Label label);
//
// set MUX state in APP DB for orchagent processing
//
void DbInterface::handleSetMuxState(const std::string portName, mux_state::MuxState::Label label)
{
    MUXLOGDEBUG(boost::format("%s: setting mux state to %s") % portName % mMuxState[label]);

    if (label <= mux_state::MuxState::Label::Unknown) {
        std::vector<swss::FieldValueTuple> values = {
            {"state", mMuxState[label]},
        };
        mAppDbMuxTablePtr->set(portName, values);
    }
}

//
// ---> handleSetPeerMuxState(const std::string portName, mux_state::MuxState::Label label);
//
// set MUX state in APP DB for orchagent processing
//
void DbInterface::handleSetPeerMuxState(const std::string portName, mux_state::MuxState::Label label)
{
    MUXLOGDEBUG(boost::format("%s: setting peer mux state to %s") % portName % mMuxState[label]);

    if (label <= mux_state::MuxState::Label::Unknown) {
        mAppDbPeerMuxTablePtr->hset(portName, "state", mMuxState[label]);
    }
}

//
// ---> handleProbeMuxState(const std::string portName)
//
// trigger xcvrd to read MUX state using i2c
//
void DbInterface::handleProbeMuxState(const std::string portName)
{
    MUXLOGDEBUG(portName);

    mAppDbMuxCommandTablePtr->hset(portName, "command", "probe");
}

//
// ---> handleProbeForwardingState(const std::string portName)
//
// trigger xcvrd to read forwarding state using gRPC
//
void DbInterface::handleProbeForwardingState(const std::string portName)
{
    MUXLOGDEBUG(portName);

    mAppDbForwardingCommandTablePtr->hset(portName, "command", "probe");
}

//
// ---> handleSetMuxLinkmgrState(const std::string portName, link_manager::ActiveStandbyStateMachine::Label label);
//
// set MUX LinkMgr state in State DB for cli processing
//
void DbInterface::handleSetMuxLinkmgrState(
    const std::string portName,
    link_manager::ActiveStandbyStateMachine::Label label
)
{
    MUXLOGDEBUG(boost::format("%s: setting mux linkmgr state to %s") % portName % mMuxLinkmgrState[static_cast<int> (label)]);

    if (label < link_manager::ActiveStandbyStateMachine::Label::Count) {
        mStateDbMuxLinkmgrTablePtr->hset(portName, "state", mMuxLinkmgrState[static_cast<int> (label)]);
    }
}

//
// ---> handlePostMuxMetrics(
//          const std::string portName,
//          link_manager::ActiveStandbyStateMachine::Metrics metrics,
//          mux_state::MuxState::Label label,
//          boost::posix_time::ptime time);
//
// set MUX metrics to state db
//
void DbInterface::handlePostMuxMetrics(
    const std::string portName,
    link_manager::ActiveStandbyStateMachine::Metrics metrics,
    mux_state::MuxState::Label label,
    boost::posix_time::ptime time
)
{
    MUXLOGDEBUG(boost::format("%s: posting mux metrics event linkmgrd_switch_%s_%s") %
        portName %
        mMuxState[label] %
        mMuxMetrics[static_cast<int> (metrics)]
    );

    if (metrics == link_manager::ActiveStandbyStateMachine::Metrics::SwitchingStart) {
        mStateDbMuxMetricsTablePtr->del(portName);
    }

    mStateDbMuxMetricsTablePtr->hset(
        portName,
        "linkmgrd_switch_" + mMuxState[label] + "_" + mMuxMetrics[static_cast<int> (metrics)],
        boost::posix_time::to_simple_string(time)
    );
}

// 
// ---> handlePostLinkProberMetrics(
//        const std::string portName,
//        link_manager::ActiveStandbyStateMachine::LinkProberMetrics,
//        boost::posix_time::ptime time
//    );
//
// post link prober pck loss event to state db 
void DbInterface::handlePostLinkProberMetrics(
    const std::string portName,
    link_manager::ActiveStandbyStateMachine::LinkProberMetrics metrics,
    boost::posix_time::ptime time
)
{
    MUXLOGWARNING(boost::format("%s: posting link prober event %s") %
        portName %
        mLinkProbeMetrics[static_cast<int> (metrics)]
    );

    for (auto linkProberMetricsEvent:mLinkProbeMetrics) {
        mStateDbLinkProbeStatsTablePtr->hdel(portName, linkProberMetricsEvent);
    }

    mStateDbLinkProbeStatsTablePtr->hset(portName, mLinkProbeMetrics[static_cast<int> (metrics)], boost::posix_time::to_simple_string(time));
}

// 
// ---> handlePostPckLossRatio(
//        const std::string portName,
//        const uint64_t unknownEventCount, 
//        const uint64_t expectedPacketCount
//    );
//
// handle post pck loss ratio 
void DbInterface::handlePostPckLossRatio(
        const std::string portName,
        const uint64_t unknownEventCount, 
        const uint64_t expectedPacketCount
)
{
    MUXLOGDEBUG(boost::format("%s: posting pck loss ratio, pck_loss_count / pck_expected_count : %d / %d") %
        portName %
        unknownEventCount % 
        expectedPacketCount
    );


    std::vector<swss::FieldValueTuple> fieldValues;
    fieldValues.push_back(std::make_pair("pck_loss_count", std::to_string(unknownEventCount)));
    fieldValues.push_back(std::make_pair("pck_expected_count", std::to_string(expectedPacketCount)));
    mStateDbLinkProbeStatsTablePtr->set(portName, fieldValues);
}

//
// ---> processTorMacAddress(std::string& mac);
//
// retrieve ToR MAC address information
//
void DbInterface::processTorMacAddress(std::string& mac)
{
    try {
        swss::MacAddress swssMacAddress(mac);
        std::array<uint8_t, ETHER_ADDR_LEN> macAddress;

        memcpy(macAddress.data(), swssMacAddress.getMac(), macAddress.size());
        mMuxManagerPtr->setTorMacAddress(macAddress);
    }
    catch (const std::invalid_argument &invalidArgument) {
        throw MUX_ERROR(ConfigNotFound, "Invalid ToR MAC address " + mac);
    }
}

//
// ---> getTorMacAddress(std::shared_ptr<swss::DBConnector> configDbConnector);
//
// retrieve ToR MAC address information
//
void DbInterface::getTorMacAddress(std::shared_ptr<swss::DBConnector> configDbConnector)
{
    MUXLOGINFO("Reading ToR MAC Address");

    swss::Table configDbMetadataTable(configDbConnector.get(), CFG_DEVICE_METADATA_TABLE_NAME);
    const std::string localhost = "localhost";
    const std::string key = "mac";
    std::string mac;

    if (configDbMetadataTable.hget(localhost, key, mac)) {
        processTorMacAddress(mac);
    } else {
        throw MUX_ERROR(ConfigNotFound, "ToR MAC address is not found");
    }
}

//
// ---> getVlanNames(std::shared_ptr<swss::DBConnector> configDbConnector);
// 
// get vlan names
//
void DbInterface::getVlanNames(std::shared_ptr<swss::DBConnector> configDbConnector)
{
    MUXLOGINFO("Reading Vlan MAC Address");
    swss::Table configDbVlanTable(configDbConnector.get(), CFG_VLAN_TABLE_NAME);
    std::vector<std::string> vlanNames;

    configDbVlanTable.getKeys(vlanNames);
    getVlanMacAddress(vlanNames);
}

//
// ---> getVlanMacAddress(std::vector<std::string> &vlanNames);
//
// retrieve Vlan MAC address informtaion
//
void DbInterface::getVlanMacAddress(std::vector<std::string> &vlanNames)
{
    MUXLOGINFO("Reading Vlan MAC Address");

    if (vlanNames.size() > 0) {
        std::shared_ptr<swss::DBConnector> configDbPtr = std::make_shared<swss::DBConnector> ("CONFIG_DB", 0);
        swss::Table configDbVlanTable(configDbPtr.get(), CFG_VLAN_TABLE_NAME);
        const std::string vlanName = vlanNames[0];
        const std::string field = "mac";
        std::string mac;
        
        if (configDbVlanTable.hget(vlanName, field, mac)) {
            processVlanMacAddress(mac);
        } else {
            MUXLOGWARNING(boost::format("MAC address is not found for %s, fall back to use device MAC for link prober.") % vlanName);
            mMuxManagerPtr->setIfUseTorMacAsSrcMac(true);
        }
    } else {
        MUXLOGWARNING("VLAN table is not found in CONFIG DB, fall back to use device MAC for link prober.");
        mMuxManagerPtr->setIfUseTorMacAsSrcMac(true);
    }
}

//
// ---> processVlanMacAddress(std::string& mac);
//
// process Vlan Mac Address 
//
void DbInterface::processVlanMacAddress(std::string& mac)
{
    try {
        swss::MacAddress swssMacAddress(mac);
        std::array<uint8_t, ETHER_ADDR_LEN> macAddress;

        memcpy(macAddress.data(), swssMacAddress.getMac(), macAddress.size());
        mMuxManagerPtr->setVlanMacAddress(macAddress);
    }
    catch (const std::invalid_argument &invalidArgument) {
        MUXLOGWARNING("Invalid Vlan MAC address " + mac);
        mMuxManagerPtr->setIfUseTorMacAsSrcMac(true);
    }
}

//
// ---> processLoopback2InterfaceInfo(std::vector<std::string> &loopbackIntfs)
//
// process Loopback2 interface information
//
void DbInterface::processLoopback2InterfaceInfo(std::vector<std::string> &loopbackIntfs)
{
    const std::string loopback2 = "Loopback2|";
    bool loopback2IPv4Found = false;

    for (auto &loopbackIntf: loopbackIntfs) {
        size_t pos = loopbackIntf.find(loopback2);
        if (pos != std::string::npos) {
            std::string ip = loopbackIntf.substr(loopback2.size(), loopbackIntf.size() - loopback2.size());
            MUXLOGINFO(boost::format("configDb Loopback2: ip: %s") % ip);

            pos = ip.find("/");
            if (pos != std::string::npos) {
                ip.erase(pos);
            }
            boost::system::error_code errorCode;
            boost::asio::ip::address ipAddress = boost::asio::ip::make_address(ip, errorCode);
            if (!errorCode) {
                if (ipAddress.is_v4()) {
                    mMuxManagerPtr->setLoopbackIpv4Address(ipAddress);
                    loopback2IPv4Found = true;
                } else if (ipAddress.is_v6()) {
                    // handle IPv6 probing
                }
            } else {
                MUXLOGFATAL(boost::format("Received Loopback2 IP: %s, error code: %d") % ip % errorCode);
            }
        }
    }

    if (!loopback2IPv4Found) {
        MUXLOGFATAL(boost::format("Config not found: Loopback2 IPv4 address missing, using default value %s ") % mMuxManagerPtr->getLoopbackIpv4Address().to_string());
    }
}

//
// ---> getLoopback2InterfaceInfo(std::shared_ptr<swss::DBConnector> configDbConnector);
//
// retrieve Loopback2 interface information and block until it shows as OK in the state db
//
void DbInterface::getLoopback2InterfaceInfo(std::shared_ptr<swss::DBConnector> configDbConnector)
{
    MUXLOGINFO("Reading Loopback2 interface information");
    swss::Table configDbLoopbackTable(configDbConnector.get(), CFG_LOOPBACK_INTERFACE_TABLE_NAME);
    std::vector<std::string> loopbackIntfs;

    configDbLoopbackTable.getKeys(loopbackIntfs);
    processLoopback2InterfaceInfo(loopbackIntfs);
}

//
// ---> processServerIpAddress(std::vector<swss::KeyOpFieldsValuesTuple> &entries);
//
// process server/blades IP address and builds a map of IP to port name
//
void DbInterface::processServerIpAddress(std::vector<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string portName = kfvKey(entry);
        std::string operation = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "server_ipv4";}
        );
        if (cit != fieldValues.cend()) {
            const std::string f = cit->first;
            std::string smartNicIpAddress = cit->second;

            MUXLOGDEBUG(boost::format("port: %s, %s = %s") % portName % f % smartNicIpAddress);

            size_t pos = smartNicIpAddress.find("/");
            if (pos != std::string::npos) {
                smartNicIpAddress.erase(pos);
            }

            boost::system::error_code errorCode;
            boost::asio::ip::address ipAddress = boost::asio::ip::make_address(smartNicIpAddress, errorCode);
            if (!errorCode) {
                mMuxManagerPtr->addOrUpdateMuxPort(portName, ipAddress);
                mServerIpPortMap[ipAddress] = portName;
            } else {
                MUXLOGFATAL(boost::format("%s: Received invalid server IP: %s, error code: %d") %
                    portName %
                    smartNicIpAddress %
                    errorCode
                );
            }
        }
    }
}

//
// ---> getServerIpAddress(std::shared_ptr<swss::DBConnector> configDbConnector);
//
// retrieve server/blades IP address and builds a map of IP to port name
//
void DbInterface::getServerIpAddress(std::shared_ptr<swss::DBConnector> configDbConnector)
{
    MUXLOGINFO("Reading MUX Server IPs");
    swss::Table configDbMuxCableTable(configDbConnector.get(), CFG_MUX_CABLE_TABLE_NAME);
    std::vector<swss::KeyOpFieldsValuesTuple> entries;

    configDbMuxCableTable.getContent(entries);
    mMuxManagerPtr->updateWarmRestartReconciliationCount(entries.size());
    processServerIpAddress(entries);
}

//
// ---> processPortCableType(std::vector<swss::KeyOpFieldsValuesTuple> &entries);
//
// process port cable type and build a map of port name to cable type
//
void DbInterface::processPortCableType(std::vector<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string portName = kfvKey(entry);
        std::string operation = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::string field = "cable_type";
        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [&field] (const swss::FieldValueTuple &fv) {return fvField(fv) == field;}
        );
        std::string portCableType = (cit != fieldValues.cend() ? cit->second : "active-standby");

        MUXLOGDEBUG(boost::format("port: %s, %s = %s") % portName % field % portCableType);

        mMuxManagerPtr->updatePortCableType(portName, portCableType);
    }
}

//
// ---> getPortCableType(std::shared_ptr<swss::DBConnector> configDbConnector);
//
// retrieve per port cable type
//
void DbInterface::getPortCableType(std::shared_ptr<swss::DBConnector> configDbConnector)
{
    MUXLOGINFO("Reading port cable types");
    swss::Table configDbMuxCableTable(configDbConnector.get(), CFG_MUX_CABLE_TABLE_NAME);
    std::vector<swss::KeyOpFieldsValuesTuple> entries;

    configDbMuxCableTable.getContent(entries);
    processPortCableType(entries);
}

//
// ---> processLinkFailureDetectionType(std::vector<swss::KeyOpFieldsValuesTuple> &entries);
//
// process Mux Cable Table enteries to get linkFailureDetectionType by defaut its software
//
void DbInterface::processLinkFailureDetectionType(std::vector<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string portName = kfvKey(entry);
        std::string operation = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::string field = "link_failure_detection_type";
        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [&field] (const swss::FieldValueTuple &fv) {return fvField(fv) == field;}
        );
        std::string linkFailureDetectionType = (cit != fieldValues.cend() ? cit->second : "software");

        MUXLOGDEBUG(boost::format("port: %s, %s = %s") % portName % field % linkFailureDetectionType);

        mMuxManagerPtr->updateLinkFailureDetectionType(portName, linkFailureDetectionType);
    }
}

//
// ---> getPortCableType(std::shared_ptr<swss::DBConnector> configDbConnector);
//
// retrieve the Link Failure Detection Type (HW/SW) from config
//
void DbInterface::getLinkFailureDetectionType(std::shared_ptr<swss::DBConnector> configDbConnector)
{
    MUXLOGINFO("Reading link_failure_detection_type");
    swss::Table configDbMuxCableTable(configDbConnector.get(), CFG_MUX_CABLE_TABLE_NAME);
    std::vector<swss::KeyOpFieldsValuesTuple> entries;

    configDbMuxCableTable.getContent(entries);
    processLinkFailureDetectionType(entries);
}

//
// ---> processSoCIpAddress(std::vector<swss::KeyOpFieldsValuesTuple> &entries);
//
// process SoC addresses and build a map of port name to SoC address
//
void DbInterface::processSoCIpAddress(std::vector<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string portName = kfvKey(entry);
        std::string operation = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "soc_ipv4";}
        );
        if (cit != fieldValues.cend()) {
            const std::string f = cit->first;
            std::string SoCIpAddress = cit->second;

            MUXLOGDEBUG(boost::format("port: %s, %s = %s") % portName % f % SoCIpAddress);

            size_t pos = SoCIpAddress.find("/");
            if (pos != std::string::npos) {
                SoCIpAddress.erase(pos);
            }

            boost::system::error_code errorCode;
            boost::asio::ip::address ipAddress = boost::asio::ip::make_address(SoCIpAddress, errorCode);
            if (!errorCode) {
                mMuxManagerPtr->addOrUpdateMuxPortSoCAddress(portName, ipAddress);
            } else {
                MUXLOGFATAL(boost::format("%s: Received invalid SoC IP: %s, error code: %d") %
                    portName %
                    SoCIpAddress %
                    errorCode
                );
            }
        }
    }
}

//
// ---> getSoCIpAddress(std::shared_ptr<swss::DBConnector> configDbConnector);
//
// retrieve SoC IP address for port in active-active cable type
//
void DbInterface::getSoCIpAddress(std::shared_ptr<swss::DBConnector> configDbConnector)
{
    MUXLOGINFO("Reading SoC IP addresses");
    swss::Table configDbMuxCableTable(configDbConnector.get(), CFG_MUX_CABLE_TABLE_NAME);
    std::vector<swss::KeyOpFieldsValuesTuple> entries;

    configDbMuxCableTable.getContent(entries);
    processSoCIpAddress(entries);
}

//
// ---> getMuxModeConfig();
//
// retrieve MUX mode configuration
//
std::map<std::string, std::string> DbInterface::getMuxModeConfig()
{
    MUXLOGINFO("Reading MUX mode configuration");
    std::shared_ptr<swss::DBConnector> configDbPtr = std::make_shared<swss::DBConnector> ("CONFIG_DB", 0);
    swss::Table configDbMuxCableTable(configDbPtr.get(), CFG_MUX_CABLE_TABLE_NAME);
    std::vector<swss::KeyOpFieldsValuesTuple> entries;
    std::map<std::string, std::string> PortToMuxModeConfigMapping;

    configDbMuxCableTable.getContent(entries);

    for (auto &entry: entries) {
        std::string portName = kfvKey(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "state";}
        );

        if (cit != fieldValues.cend()) {
            const std::string f = cit->first;
            std::string muxMode = cit->second;

            MUXLOGDEBUG(boost::format("port: %s, mode mux %s = %s") % portName % f % muxMode);

            PortToMuxModeConfigMapping[portName] = muxMode;
        } else {
            MUXLOGERROR(boost::format("port: %s, mode mux is not found in %s table") % portName % CFG_MUX_CABLE_TABLE_NAME);
        }
    }

    return PortToMuxModeConfigMapping;
}

// ---> warmRestartReconciliation(const std::string &portName);
//
// port warm restart reconciliation procedure
//
void DbInterface::warmRestartReconciliation(const std::string &portName)
{
    MUXLOGDEBUG(portName);

    if (isWarmStart()) {
        setMuxMode(portName, "auto");
        mMuxManagerPtr->updateWarmRestartReconciliationCount(-1);
    }
}

//
// ---> setMuxMode
//
// set config db mux mode
//
void DbInterface::setMuxMode(const std::string &portName, const std::string state)
{
    MUXLOGDEBUG(portName);

    boost::asio::post(mStrand,boost::bind(
        &DbInterface::handleSetMuxMode,
        this,
        portName,
        state
    ));
}

//
// ---> handleSetMuxmode
//
// handle set mux mode
//
void DbInterface::handleSetMuxMode(const std::string &portName, const std::string state)
{
    MUXLOGWARNING(boost::format("%s: configuring mux mode to %s after warm restart") % portName % state);

    std::shared_ptr<swss::DBConnector> configDbPtr = std::make_shared<swss::DBConnector> ("CONFIG_DB", 0);
    std::shared_ptr<swss::Table> configDbMuxCableTablePtr = std::make_shared<swss::Table> (
        configDbPtr.get(), CFG_MUX_CABLE_TABLE_NAME
    );
    configDbMuxCableTablePtr->hset(portName, "state", state);
}

//
// ---> processMuxPortConfigNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);
//
// process MUX port configuration change notification
//
void DbInterface::processMuxPortConfigNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string port = kfvKey(entry);
        std::string operation = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "state";}
        );
        if (cit != fieldValues.cend()) {
            const std::string f = cit->first;
            const std::string v = cit->second;

            MUXLOGDEBUG(boost::format("key: %s, Operation: %s, f: %s, v: %s") %
                port %
                operation %
                f %
                v
            );
            
            mMuxManagerPtr->updateMuxPortConfig(port, v);
        }

        std::vector<swss::FieldValueTuple>::const_iterator c_it = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "pck_loss_data_reset";}
        );
        if (c_it != fieldValues.cend()) {
            const std::string f = c_it->first;
            const std::string v = c_it->second;

            MUXLOGDEBUG(boost::format("key: %s, Operation: %s, f: %s, v: %s") %
                port %
                operation %
                f %
                v
            );
            
            mMuxManagerPtr->resetPckLossCount(port);
        }
    }
}

//
// ---> handleMuxPortConfigNotifiction(swss::SubscriberStateTable &configMuxTable);
//
// handles MUX port configuration change notification
//
void DbInterface::handleMuxPortConfigNotifiction(swss::SubscriberStateTable &configMuxTable)
{
    std::deque<swss::KeyOpFieldsValuesTuple> entries;

    configMuxTable.pops(entries);
    processMuxPortConfigNotifiction(entries);
}

//
// ---> processMuxLinkmgrConfigNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);
//
// process MUX Linkmgr configuration change notification
//
void DbInterface::processMuxLinkmgrConfigNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string key = kfvKey(entry);
        if (key == "LINK_PROBER") {
            std::string operation = kfvOp(entry);
            std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

            try {
                for (auto &fieldValue: fieldValues) {
                    std::string f = fvField(fieldValue);
                    std::string v = fvValue(fieldValue);
                    if (f == "interval_v4") {
                        mMuxManagerPtr->setTimeoutIpv4_msec(boost::lexical_cast<uint32_t> (v));
                    } else if (f == "interval_v6") {
                        mMuxManagerPtr->setTimeoutIpv6_msec(boost::lexical_cast<uint32_t> (v));
                    } else if (f == "positive_signal_count") {
                        mMuxManagerPtr->setPositiveStateChangeRetryCount(boost::lexical_cast<uint32_t> (v));
                    } else if (f == "negative_signal_count") {
                        mMuxManagerPtr->setNegativeStateChangeRetryCount(boost::lexical_cast<uint32_t> (v));
                    } else if (f == "suspend_timer") {
                        mMuxManagerPtr->setSuspendTimeout_msec(boost::lexical_cast<uint32_t> (v));
                    } else if (f == "use_well_known_mac") {
                        mMuxManagerPtr->setUseWellKnownMacActiveActive(v == "enable");
                    } else if (f == "src_mac") {
                        mMuxManagerPtr->processSrcMac(v == "ToRMac");
                    } else if (f == "interval_pck_loss_count_update") {
                        mMuxManagerPtr->setLinkProberStatUpdateIntervalCount(boost::lexical_cast<uint32_t> (v));
                    } else if (f == "reset_suspend_timer") {
                        boost::tokenizer<> tok(v);
                        std::vector<std::string> ports(tok.begin(), tok.end());
                        mMuxManagerPtr->processResetSuspendTimer(ports);
                    }

                    MUXLOGINFO(boost::format("key: %s, Operation: %s, f: %s, v: %s") %
                        key %
                        operation %
                        f %
                        v
                    );
                }
            }
            catch (boost::bad_lexical_cast const &badLexicalCast) {
                MUXLOGWARNING(boost::format("bad lexical cast: %s") % badLexicalCast.what());
            }
        } else if (key == "MUXLOGGER") {
            std::string operation = kfvOp(entry);
            std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

            for (auto &fieldValue: fieldValues) {
                std::string f = fvField(fieldValue);
                std::string v = fvValue(fieldValue);
                if (f == "log_verbosity") {
                    if (!common::MuxLogger::getInstance()->isLinkToSwssLogger()) {
                        mMuxManagerPtr->updateLogVerbosity(v);
                    }
                }

                MUXLOGINFO(boost::format("key: %s, Operation: %s, f: %s, v: %s") %
                    key %
                    operation %
                    f %
                    v
                );
            }
        } else if (key == "TIMED_OSCILLATION") {
            std::string operation = kfvOp(entry);
            std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

            for (auto &fieldValue: fieldValues) {
                std::string f = fvField(fieldValue);
                std::string v = fvValue(fieldValue);
                if (f == "oscillation_enabled") {
                        if (v == "true") {
                            mMuxManagerPtr->setOscillationEnabled(true);
                        } else if (v == "false"){
                            mMuxManagerPtr->setOscillationEnabled(false);
                        }
                } else if (f == "interval_sec") {
                    try {
                        mMuxManagerPtr->setOscillationInterval_sec(boost::lexical_cast<uint32_t> (v));
                    } catch (boost::bad_lexical_cast const &badLexicalCast) {
                        MUXLOGWARNING(boost::format("bad lexical cast: %s") % badLexicalCast.what());
                    }
                }

                MUXLOGWARNING(boost::format("key: %s, Operation: %s, f: %s, v: %s") %
                    key %
                    operation %
                    f %
                    v
                );
            }
        }
    }
}

//
// ---> handleMuxLinkmgrConfigNotifiction(swss::SubscriberStateTable &configLocalhostTable);
//
// handles MUX linkmgr configuration change notification
//
void DbInterface::handleMuxLinkmgrConfigNotifiction(swss::SubscriberStateTable &configMuxLinkmgrTable)
{
    std::deque<swss::KeyOpFieldsValuesTuple> entries;

    configMuxLinkmgrTable.pops(entries);
    processMuxLinkmgrConfigNotifiction(entries);
}

//
// ---> processLinkStateNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);
//
// process link state change notification
//
void DbInterface::processLinkStateNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string port = kfvKey(entry);
        std::string operation = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "oper_status";}
        );
        if (cit != fieldValues.cend()) {
            const std::string f = cit->first;
            const std::string v = cit->second;

            MUXLOGDEBUG(boost::format("port: %s, operation: %s, f: %s, v: %s") %
                port %
                operation %
                f %
                v
            );
            mMuxManagerPtr->addOrUpdateMuxPortLinkState(port, v);
        }
    }
}

//
// ---> handleLinkStateNotifiction(swss::SubscriberStateTable &appdbPortTable);
//
// handles link state change notification
//
void DbInterface::handleLinkStateNotifiction(swss::SubscriberStateTable &appdbPortTable)
{
    std::deque<swss::KeyOpFieldsValuesTuple> entries;

    appdbPortTable.pops(entries);
    processLinkStateNotifiction(entries);
}

// 
// ---> processPeerLinkStateNotification(std::deque<swss:KeyOpFieldsValuesTuple> &entries);
//
// process peer's link state notification 
// 
void DbInterface::processPeerLinkStateNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string port = kfvKey(entry);
        std::string operation = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "link_status_peer";}
        );
        if (cit != fieldValues.cend()) {
            const std::string f = cit->first;
            const std::string v = cit->second;

            MUXLOGDEBUG(boost::format("port: %s, operation: %s, f: %s, v: %s") %
                port %
                operation %
                f %
                v
            );
            mMuxManagerPtr->addOrUpdatePeerLinkState(port, v);
        }
    }
}

//
// ---> handlePeerLinkStateNotification(swss::SubscriberStateTable &stateDbMuxInfoTable);
//
// handle peer's link status change notification 
//
void DbInterface::handlePeerLinkStateNotification(swss::SubscriberStateTable &stateDbMuxInfoTable)
{
    std::deque<swss::KeyOpFieldsValuesTuple> entries;

    stateDbMuxInfoTable.pops(entries);
    processPeerLinkStateNotification(entries);
}

//
// ---> processMuxResponseNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);
//
// process MUX response (from xcvrd) notification
//
void DbInterface::processMuxResponseNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string port = kfvKey(entry);
        std::string oprtation = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "response";}
        );
        if (cit != fieldValues.cend()) {
            const std::string f = cit->first;
            const std::string v = cit->second;

            MUXLOGDEBUG(boost::format("port: %s, operation: %s, f: %s, v: %s") %
                port %
                oprtation %
                f %
                v
            );
//            swss::Table table(mAppDbPtr.get(), APP_MUX_CABLE_RESPONSE_TABLE_NAME);
//            table.hdel(port, "response");
            mMuxManagerPtr->processProbeMuxState(port, v);
        }
    }
}

//
// ---> processForwardingResponseNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries);
//
// process forwarding response (from xcvrd probing) notification
//
void DbInterface::processForwardingResponseNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string port = kfvKey(entry);
        std::string oprtation = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit_self = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "response";}
        );
        if (cit_self != fieldValues.cend()) {
            const std::string f = cit_self->first;
            const std::string v = cit_self->second;

            MUXLOGDEBUG(boost::format("port: %s, operation: %s, f: %s, v: %s") %
                port %
                oprtation %
                f %
                v
            );
            mMuxManagerPtr->processProbeMuxState(port, v);
        }

        std::vector<swss::FieldValueTuple>::const_iterator cit_peer = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "response_peer";}
        );
        if (cit_peer != fieldValues.cend()) {
            const std::string f = cit_peer->first;
            const std::string v = cit_peer->second;

            MUXLOGDEBUG(boost::format("port: %s, operation: %s, f: %s, v: %s") %
                port %
                oprtation %
                f %
                v
            );
            mMuxManagerPtr->processPeerMuxState(port, v);
        }
    }
}

//
// ---> handleMuxResponseNotifiction(swss::SubscriberStateTable &appdbPortTable);
//
// handles MUX response (from xcvrd) notification
//
void DbInterface::handleMuxResponseNotifiction(swss::SubscriberStateTable &appdbPortTable)
{
    std::deque<swss::KeyOpFieldsValuesTuple> entries;

    appdbPortTable.pops(entries);
    processMuxResponseNotifiction(entries);
}

//
// ---> handleForwardingResponseNotification(swss::SubscriberStateTable &appdbForwardingResponseTable)
//
// handle fowarding response (from xcvrd) notification
//
void DbInterface::handleForwardingResponseNotification(swss::SubscriberStateTable &appdbForwardingResponseTable)
{
    std::deque<swss::KeyOpFieldsValuesTuple> entries;

    appdbForwardingResponseTable.pops(entries);
    processForwardingResponseNotification(entries);
}


//
// ---> processPeerMuxResponseNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries);
//
// process peer MUX state (from xcvrd) notification
//
void DbInterface::processPeerMuxNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string port = kfvKey(entry);
        std::string oprtation = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "state";}
        );
        if (cit != fieldValues.cend()) {
            const std::string f = cit->first;
            const std::string v = cit->second;

            MUXLOGDEBUG(boost::format("port: %s, operation: %s, f: %s, v: %s") %
                port %
                oprtation %
                f %
                v
            );
            mMuxManagerPtr->processPeerMuxState(port, v);
        }
    }
}

//
// ---> handlePeerMuxStateNotification(swss::SubscriberStateTable &stateDbPeerMuxTable);
//
// handles peer MUX notification (from xcvrd) 
//
void DbInterface::handlePeerMuxStateNotification(swss::SubscriberStateTable &stateDbPeerMuxTable)
{
    std::deque<swss::KeyOpFieldsValuesTuple> entries;

    stateDbPeerMuxTable.pops(entries);
    processPeerMuxNotification(entries);
}

//
// ---> processMuxStateNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);
//
// processes MUX state (from orchagent) notification
//
void DbInterface::processMuxStateNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string port = kfvKey(entry);
        std::string oprtation = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "state";}
        );
        if (cit != fieldValues.cend()) {
            const std::string f = cit->first;
            const std::string v = cit->second;

            MUXLOGDEBUG(boost::format("port: %s, operation: %s, f: %s, v: %s") %
                port %
                oprtation %
                f %
                v
            );
            mMuxManagerPtr->addOrUpdateMuxPortMuxState(port, v);
        }
    }
}

//
// ---> handleMuxStateNotifiction(swss::SubscriberStateTable &statedbPortTable);
//
// handles MUX state (from orchagent) notification
//
void DbInterface::handleMuxStateNotifiction(swss::SubscriberStateTable &statedbPortTable)
{
    std::deque<swss::KeyOpFieldsValuesTuple> entries;

    statedbPortTable.pops(entries);
    processMuxStateNotifiction(entries);
}

//
// ---> processDefaultRouteStateNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
// 
// process default route state notification from orchagent
//
void DbInterface::processDefaultRouteStateNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string key = kfvKey(entry);
        std::string op = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "state";}
        );

        if (cit != fieldValues.cend()) {
            const std::string field = cit->first;
            const std::string value = cit->second;

            MUXLOGDEBUG(boost::format("key: %s, operation: %s, field: %s, value: %s") %
                key %
                op %
                field %
                value
            );

            if (key == "0.0.0.0/0") {
                mMuxManagerPtr->addOrUpdateDefaultRouteState(true, value);
            } else if (key == "::/0") {
                mMuxManagerPtr->addOrUpdateDefaultRouteState(false, value);
            } else {
                MUXLOGFATAL(boost::format("Received Invalid IP: %s") % key );
            }
        }
    }
}

//
// ---> handleDefaultRouteStateNotification(swss::SubscriberStateTable &statedbRouteTable);
//
// handle Default Route State notification from orchagent
//
void DbInterface::handleDefaultRouteStateNotification(swss::SubscriberStateTable &statedbRouteTable)
{
    std::deque<swss::KeyOpFieldsValuesTuple> entries;

    statedbRouteTable.pops(entries);
    processDefaultRouteStateNotification(entries);
}

//
// ---> handleBgpDeviceGlobal(swss::SubscriberStateTable &configDbBgpDeviceGlobalTable);
//
// handle tsa_enable notification from BGP Device Global table 
//
void DbInterface::handleBgpDeviceGlobal(swss::SubscriberStateTable &configDbBgpDeviceGlobalTable)
{
    std::deque<swss::KeyOpFieldsValuesTuple> entries;

    configDbBgpDeviceGlobalTable.pops(entries);
    processTsaEnableNotification(entries);
}

//
// ---> processTsaEnableNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries);
//
// process Tsa Enable Notification
//
void DbInterface::processTsaEnableNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
    for (auto &entry: entries) {
        std::string key = kfvKey(entry);
        std::string op = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "tsa_enabled";}
        );

        if (cit != fieldValues.cend()) {
            const std::string field = cit->first;
            const std::string value = cit->second;

            MUXLOGDEBUG(boost::format("key: %s, operation: %s, field: %s, value: %s") %
                key %
                op %
                field %
                value
            );

            mMuxManagerPtr->handleTsaEnableNotification(value == "true");
        }
    }
}

//
// ---> createIcmpEchoSession(std::string key, IcmpHwOffloadEntriesPtr entries);
//
// triggers creation of a ICMP_ECHO_SESSION in APP_ICMP_ECHO_SESSION_TABLE
//
void DbInterface::createIcmpEchoSession(std::string key, IcmpHwOffloadEntriesPtr entries)
{
    MUXLOGDEBUG(key);

    boost::asio::post(mStrand, boost::bind(
        &DbInterface::handleIcmpEchoSession,
        this,
        key,
        entries.release()
    ));
}

//
// ---> handleIcmpEchoSession(std::string key, IcmpHwOffloadEntries *entries);
//
// handles creation of a ICMP_ECHO_SESSION in APP_ICMP_ECHO_SESSION_TABLE
//
void DbInterface::handleIcmpEchoSession(std::string key, IcmpHwOffloadEntries *entries)
{
    std::vector<swss::FieldValueTuple> fvs;
    for(auto& entry : *entries) {
        const std::string field = entry.first;
        const std::string value = entry.second;
        fvs.emplace_back(field, value);
        MUXLOGDEBUG(boost::format("APP_ICMP_ECHO_SESSION_TABLE::key: %s, field: %s, value: %s") %
                key %
                field %
                value
            );
    }
    mAppDbIcmpEchoSessionTablePtr->set(key, fvs);
    delete entries;
}

//
// ---> deleteIcmpEchoSession(std::string key);
//
// handles deletion of a ICMP_ECHO_SESSION in APP_ICMP_ECHO_SESSION_TABLE
//
void DbInterface::deleteIcmpEchoSession(std::string key)
{
    MUXLOGDEBUG(boost::format("APP_ICMP_ECHO_SESSION_TABLE::key: %s ") %
            key
        );
    mAppDbIcmpEchoSessionTablePtr->del(key);
}

//
// ---> extractIfnameAndSessionType(const std::string &key, std::string &ifname, std::string &sessionType);
//
// helper function to extract interface name and session_type from the key
//
void DbInterface::extractIfnameAndSessionType(const std::string &key, std::string &ifname, std::string &sessionType)
{
    size_t firstColon = key.find(':');
    size_t secondColon = key.find(':', firstColon + 1);
    size_t thirdColon = key.find(':', secondColon + 1);
    size_t fourthColon = key.find(':', thirdColon + 1);

    if (firstColon != std::string::npos &&
        secondColon != std::string::npos &&
        thirdColon != std::string::npos &&
        fourthColon != std::string::npos) {
        ifname = key.substr(secondColon + 1, thirdColon - secondColon - 1);
        sessionType = key.substr(fourthColon + 1);
    } else {
        std::cerr << "Unexpected format: " << key << std::endl;
    }
}

//
// ---> processIcmpEchoSessionStateNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries);
//
// process each entery of ICMP_ECHO_SESSION_TABLE extract Port, Session_Type(Normal/Rx) and state itself
//
void DbInterface::processIcmpEchoSessionStateNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries)
{
      for (auto &entry: entries) {
        std::string key = kfvKey(entry);
        std::string port;
        std::string session_type;
        extractIfnameAndSessionType(key, port, session_type);
        std::string opertation = kfvOp(entry);
        std::vector<swss::FieldValueTuple> fieldValues = kfvFieldsValues(entry);

        std::vector<swss::FieldValueTuple>::const_iterator cit = std::find_if(
            fieldValues.cbegin(),
            fieldValues.cend(),
            [] (const swss::FieldValueTuple &fv) {return fvField(fv) == "state";}
        );
        if (cit != fieldValues.cend()) {
            const std::string field = cit->first;
            const std::string value = cit->second;

            MUXLOGDEBUG(boost::format("port: %s, f: %s, v: %s") %
                port %
                field %
                value
            );
            mMuxManagerPtr->updateLinkFailureDetectionState(port, value, session_type);
        }
    }
}

//
// ---> handleIcmpEchoSessionStateNotification(swss::SubscriberStateTable &stateDbIcmpEchoSessionTable);
//
// extract enteries from State_DB ICMP_ECHO_SESSION_TABLE and call processing function
//
void DbInterface::handleIcmpEchoSessionStateNotification(swss::SubscriberStateTable &stateDbIcmpEchoSessionTable)
{
    std::deque<swss::KeyOpFieldsValuesTuple> entries;

    stateDbIcmpEchoSessionTable.pops(entries);
    processIcmpEchoSessionStateNotification(entries);
}

//
// ---> handleSwssNotification();
//
// main thread method for handling SWSS notification
//
void DbInterface::handleSwssNotification()
{
    std::shared_ptr<swss::DBConnector> configDbPtr = std::make_shared<swss::DBConnector> ("CONFIG_DB", 0);
    std::shared_ptr<swss::DBConnector> appDbPtr = std::make_shared<swss::DBConnector> ("APPL_DB", 0);
    std::shared_ptr<swss::DBConnector> stateDbPtr = std::make_shared<swss::DBConnector> ("STATE_DB", 0);

    // For reading Link Prober configurations from the MUX linkmgr table name
    swss::SubscriberStateTable configDbMuxLinkmgrTable(configDbPtr.get(), CFG_MUX_LINKMGR_TABLE_NAME);
    // for tsa_enable notification
    swss::SubscriberStateTable configDbBgpDeviceGlobalTable(configDbPtr.get(), CFG_BGP_DEVICE_GLOBAL_TABLE_NAME);

    swss::SubscriberStateTable configDbMuxTable(configDbPtr.get(), CFG_MUX_CABLE_TABLE_NAME);

    // for link up/down, should be in state db down the road
    swss::SubscriberStateTable appDbPortTable(appDbPtr.get(), APP_PORT_TABLE_NAME);
    // for command responses from the driver
    swss::SubscriberStateTable appDbMuxResponseTable(appDbPtr.get(), APP_MUX_CABLE_RESPONSE_TABLE_NAME);
    // for command response of forwarding state probing from driver 
    swss::SubscriberStateTable appDbForwardingResponseTable(appDbPtr.get(), APP_FORWARDING_STATE_RESPONSE_TABLE_NAME);
    // for getting state db MUX state when orchagent updates it
    swss::SubscriberStateTable stateDbPortTable(stateDbPtr.get(), STATE_MUX_CABLE_TABLE_NAME);
    // for getting state db default route state 
    swss::SubscriberStateTable stateDbRouteTable(stateDbPtr.get(), STATE_ROUTE_TABLE_NAME);
    // for getting peer's link status
    swss::SubscriberStateTable stateDbMuxInfoTable(stateDbPtr.get(), MUX_CABLE_INFO_TABLE);
    // for getting peer's admin forwarding state
    swss::SubscriberStateTable stateDbPeerMuxTable(stateDbPtr.get(), STATE_PEER_HW_FORWARDING_STATE_TABLE_NAME);
    // for getting icmp echo session state
    swss::SubscriberStateTable stateDbIcmpEchoSessionTable(stateDbPtr.get(), STATE_ICMP_ECHO_SESSION_TABLE_NAME);

    getTorMacAddress(configDbPtr);
    getVlanNames(configDbPtr);
    getLoopback2InterfaceInfo(configDbPtr);
    getPortCableType(configDbPtr);
    getLinkFailureDetectionType(configDbPtr);
    getServerIpAddress(configDbPtr);
    getSoCIpAddress(configDbPtr);

    NetMsgInterface netMsgInterface(*this);
    swss::NetDispatcher::getInstance().registerMessageHandler(RTM_NEWNEIGH, &netMsgInterface);
    swss::NetDispatcher::getInstance().registerMessageHandler(RTM_DELNEIGH, &netMsgInterface);

    swss::NetLink netlinkNeighbor;
    netlinkNeighbor.registerGroup(RTNLGRP_NEIGH);
    netlinkNeighbor.dumpRequest(RTM_GETNEIGH);

    swss::Select swssSelect;
    swssSelect.addSelectable(&configDbMuxLinkmgrTable);
    swssSelect.addSelectable(&configDbBgpDeviceGlobalTable);
    swssSelect.addSelectable(&configDbMuxTable);
    swssSelect.addSelectable(&appDbPortTable);
    swssSelect.addSelectable(&appDbMuxResponseTable);
    swssSelect.addSelectable(&appDbForwardingResponseTable);
    swssSelect.addSelectable(&stateDbPortTable);
    swssSelect.addSelectable(&stateDbRouteTable);
    swssSelect.addSelectable(&stateDbMuxInfoTable);
    swssSelect.addSelectable(&stateDbPeerMuxTable);
    swssSelect.addSelectable(&netlinkNeighbor);
    swssSelect.addSelectable(&stateDbIcmpEchoSessionTable);

    while (mPollSwssNotifcation) {
        swss::Selectable *selectable;
        int ret = swssSelect.select(&selectable, DEFAULT_TIMEOUT_MSEC);

        if (ret == swss::Select::ERROR) {
            MUXLOGERROR("Error had been returned in select");
            continue;
        } else if (ret == swss::Select::TIMEOUT) {
            continue;
        } else if (ret != swss::Select::OBJECT) {
            MUXLOGERROR(boost::format("Unknown return value from Select: %d") % ret);
            continue;
        }

        if (selectable == static_cast<swss::Selectable *> (&configDbMuxLinkmgrTable)) {
            handleMuxLinkmgrConfigNotifiction(configDbMuxLinkmgrTable);
        } else if (selectable == static_cast<swss::Selectable *> (&configDbMuxTable)) {
            handleMuxPortConfigNotifiction(configDbMuxTable);
        } else if (selectable == static_cast<swss::Selectable *> (&configDbBgpDeviceGlobalTable)) {
            handleBgpDeviceGlobal(configDbBgpDeviceGlobalTable);
        } else if (selectable == static_cast<swss::Selectable *> (&appDbPortTable)) {
            handleLinkStateNotifiction(appDbPortTable);
        } else if (selectable == static_cast<swss::Selectable *> (&appDbMuxResponseTable)) {
            handleMuxResponseNotifiction(appDbMuxResponseTable);
        } else if (selectable == static_cast<swss::Selectable *> (&appDbForwardingResponseTable)) {
            handleForwardingResponseNotification(appDbForwardingResponseTable);
        } else if (selectable == static_cast<swss::Selectable *> (&stateDbPortTable)) {
            handleMuxStateNotifiction(stateDbPortTable);
        } else if (selectable == static_cast<swss::Selectable *> (&stateDbRouteTable)) {
            handleDefaultRouteStateNotification(stateDbRouteTable);
        } else if (selectable == static_cast<swss::Selectable *> (&stateDbMuxInfoTable)) {
            handlePeerLinkStateNotification(stateDbMuxInfoTable);
        } else if (selectable == static_cast<swss::Selectable *> (&stateDbPeerMuxTable)) {
            handlePeerMuxStateNotification(stateDbPeerMuxTable);
        } else if (selectable == static_cast<swss::Selectable *> (&stateDbIcmpEchoSessionTable)) {
            handleIcmpEchoSessionStateNotification(stateDbIcmpEchoSessionTable);
        } else if (selectable == static_cast<swss::Selectable *> (&netlinkNeighbor)) {
            continue;
        } else {
            MUXLOGERROR("Unknown object returned by select");
        }
    }

    mBarrier.wait();
    mBarrier.wait();
    mMuxManagerPtr->terminate();
}

// GCOVR_EXCL_STOP

} /* namespace common */
