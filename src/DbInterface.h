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
 * DbInterface.h
 *
 *  Created on: Oct 23, 2020
 *      Author: Tamer Ahmed
 */

#ifndef DBINTERFACE_H_
#define DBINTERFACE_H_

#include <map>
#include <memory>

#include <boost/thread.hpp>
#include <boost/thread/barrier.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "swss/dbconnector.h"
#include "swss/producerstatetable.h"
#include "swss/subscriberstatetable.h"
#include "swss/notificationproducer.h"
#include "swss/notificationconsumer.h"
#include "swss/warm_restart.h"

#include "link_manager/LinkManagerStateMachineActiveStandby.h"
#include "mux_state/MuxState.h"

namespace test {
class MuxManagerTest;
}

namespace mux
{
#define MUX_CABLE_INFO_TABLE  "MUX_CABLE_INFO"
#define LINK_PROBE_STATS_TABLE_NAME "LINK_PROBE_STATS"

#define APP_FORWARDING_STATE_COMMAND_TABLE_NAME  "FORWARDING_STATE_COMMAND"
#define APP_FORWARDING_STATE_RESPONSE_TABLE_NAME "FORWARDING_STATE_RESPONSE"

#define APP_PEER_HW_FORWARDING_STATE_TABLE_NAME    "HW_FORWARDING_STATE_PEER"
#define STATE_PEER_HW_FORWARDING_STATE_TABLE_NAME   "HW_MUX_CABLE_TABLE_PEER"

#define STATE_MUX_SWITCH_CAUSE_TABLE_NAME "MUX_SWITCH_CAUSE"
#define LINK_PROBER_SESSION_STATE_CHANGE_NOTIFICATION_CHANNEL "LINK_PROBER_NOTIFICATIONS"
#define LINK_PROBER_SESSION_STATE_CHANGE_NOTIFICATION "link_prober_session_state_change"

class MuxManager;
using ServerIpPortMap = std::map<boost::asio::ip::address, std::string>;

/**
 *@class DbInterface
 *
 *@brief DbInterface interfaces with Redis DB, reads MUX config, and
 *       listens to updates posted to the subscriber tables.
 */
class DbInterface
{
public:
    /**
    *@method DbInterface
    *
    *@brief class default constructor
    */
    DbInterface() = delete;

    /**
    *@method DbInterface
    *
    *@brief class copy constructor
    *
    *@param DbInterface (in)  reference to DbInterface object to be copied
    */
    DbInterface(const DbInterface &) = delete;

    /**
    *@method DbInterface
    *
    *@brief class constructor
    *
    *@param muxManager (in)     pointer to MuxManager object
    *@param ioService (in)      pointer to Boost IO Service
    */
    DbInterface(mux::MuxManager *muxManager, boost::asio::io_service *ioService);

    /**
    *@method ~DbInterface
    *
    *@brief class destructor
    */
    virtual ~DbInterface() = default;

    /**
    *@method getBarrier
    *
    *@brief getter for Boost barrier object
    *
    *@return reference to Boost barrier object
    */
    inline boost::barrier& getBarrier() {return mBarrier;};

    /**
    *@method getStrand
    *
    *@brief getter for Boost strand object
    *
    *@return reference to Boost strand object
    */
    inline boost::asio::io_service::strand& getStrand() {return mStrand;};

    /**
    *@method getMuxState
    *
    *@brief retrieve the current MUX state
    *
    *@param portName (in)   MUX/port name
    *
    *@return none
    */
    virtual void getMuxState(const std::string &portName);

    /**
    *@method setMuxState
    *
    *@brief set MUX state in APP DB for orchagent processing
    *
    *@param portName (in)   MUX/port name
    *@param label (in)      label of target state
    *
    *@return none
    */
    void setMuxState(const std::string &portName, mux_state::MuxState::Label label);

    /**
    *@method handleSetMuxState
    *
    *@brief set MUX state in APP DB for orchagent processing
    *
    *@param portName (in)   MUX/port name
    *@param label (in)      label of target state
    *
    *@return none
    */
    virtual void handleSetMuxState(const std::string portName, mux_state::MuxState::Label label);

    /**
    *@method setPeerMuxState
    *
    *@brief set peer MUX state in APP DB for orchagent processing
    *
    *@param portName (in)   MUX/port name
    *@param label (in)      label of target state
    *
    *@return none
    */
    void setPeerMuxState(const std::string &portName, mux_state::MuxState::Label label);

    /**
    *@method setLinkProberSessionState
    *
    *@brief set link prober session state in ASIC DB to simulate offload
    *
    *@param portName (in)   MUX/port name
    *@param sessionId (in)  link prober session id
    *@param label (in)      label of target state
    *
    *@return none
    */
    void setLinkProberSessionState(const std::string portName, const std::string &sessionId, link_prober::LinkProberState::Label label);

    /**
    *@method probeMuxState
    *
    *@brief trigger xcvrd to read MUX state using i2c
    *
    *@param portName (in)   MUX/port name
    *
    *@return label of MUX state
    */
    virtual void probeMuxState(const std::string &portName);

    /**
     * @method probeForwardingState
     * 
     * @brief  trigger tranceiver daemon to read Fowarding state using gRPC
     * 
     * @param portName (in) MUX/port name 
     * 
     * @return none
     */
    void probeForwardingState(const std::string &portName);

    /**
    *@method setMuxLinkmgrState
    *
    *@brief set MUX LinkMgr state in State DB for cli processing
    *
    *@param portName (in)   MUX/port name
    *@param label (in)      label of target state
    *
    *@return none
    */
    virtual void setMuxLinkmgrState(const std::string &portName, link_manager::ActiveStandbyStateMachine::Label label);

    /**
    *@method postMetricsEvent
    *
    *@brief post MUX metrics event
    *
    *@param portName (in)   MUX/port name
    *@param metrics (in)    metrics data
    *@param label (in)      label of target state
    *
    *@return none
    */
    void postMetricsEvent(
        const std::string &portName,
        link_manager::ActiveStandbyStateMachine::Metrics metrics,
        mux_state::MuxState::Label label
    );

    /**
    *@method handlePostMuxMetrics
    *
    *@brief set MUX metrics to state db
    *
    *@param portName (in)   MUX/port name
    *@param metrics (in)    metrics data
    *@param label (in)      label of target state
    *@param time (in)       current time
    *
    *@return none
    */
    virtual void handlePostMuxMetrics(
        const std::string portName,
        link_manager::ActiveStandbyStateMachine::Metrics metrics,
        mux_state::MuxState::Label label,
        boost::posix_time::ptime time
    );

    /**
     * @method postSwitchCause
     * 
     * @param portName (in) port name
     * @param cause (in) switch cause 
     * 
     * @return none
     */
    virtual void postSwitchCause(
        const std::string &portName,
        link_manager::ActiveStandbyStateMachine::SwitchCause cause
    );

    /**
     * @method handlePostSwitchCause
     * 
     * @brief post switch cause to state db 
     * 
     * @param portName (in) port Name
     * @param cause (in) switch cause to post
     * @param time (in) current time
     * 
     * @return none
     */
    void handlePostSwitchCause(
        const std::string &portName,
        link_manager::ActiveStandbyStateMachine::SwitchCause cause,
        boost::posix_time::ptime time
    );

    /**
     * @method postLinkProberMetricsEvent
     * 
     * @brief post link prober event
     * 
     * @param portName (in) port name 
     * @param metrics (in) link prober event name 
     * 
     * @return none
     * 
    */
    virtual void postLinkProberMetricsEvent(
        const std::string &portName, 
        link_manager::ActiveStandbyStateMachine::LinkProberMetrics metrics
    );

    /**
     * @method postPckLossRatio
     * 
     * @brief post pck loss ratio update to state db
     * 
     * @param portName (in) port name 
     * @param unknownEventCount (in) count of missing icmp packets
     * @param expectedPacketCount (in) count of expected icmp packets 
     * 
     * @return none
    */
    virtual void postPckLossRatio(
        const std::string &portName,
        const uint64_t unknownEventCount, 
        const uint64_t expectedPacketCount
    );

    /**
    *@method initialize
    *
    *@brief initialize DB and start SWSS listening thread
    *
    *@return none
    */
    void initialize();

    /**
    *@method deinitialize
    *
    *@brief deinitialize DB interface and join SWSS listening thread
    *
    *@return none
    */
    void deinitialize();

    /**
    *@method updateServerMacAddress
    *
    *@brief Update Server MAC address behind a MUX port
    *
    *@param serverIp (in)   Server IP address
    *@param serverMac (in)  Server MAC address
    *
    *@return none
    */
    void updateServerMacAddress(boost::asio::ip::address serverIp, const uint8_t *serverMac);

    /**
    *@method stopSwssNotificationPoll
    *
    *@brief stop SWSS listening thread
    *
    *@return none
    */
    void stopSwssNotificationPoll() {mPollSwssNotifcation = false;};

    /**
     * @method setMuxMode 
     * 
     * @brief set config db mux mode 
     * 
     * @param portName (in) MUX port name 
     * @param state (in) MUX mode state 
     *  
     * @return none
     */
    void setMuxMode(const std::string &portName, const std::string state);

    /**
     * @method warmRestartReconciliation
     * 
     * @brief port warm restart reconciliation procedure
     * 
     * @param portName(in) Mux port name
     * 
     * @return none
     */
    void warmRestartReconciliation(const std::string &portName);

    /**
     * @method isWarmStart
     * 
     * @brief is warm start or not
     * 
     * @return system flag for warm start context 
     */
    virtual bool isWarmStart(){return swss::WarmStart::isWarmStart();};

    /**
     * @method getWarmStartTimer
     * 
     * @brief get warm start time out in sec
     * 
     * @return timeout in sec 
     */
    virtual uint32_t getWarmStartTimer(){return swss::WarmStart::getWarmStartTimer("linkmgrd", "mux");};

    /**
     * @method setWarmStartStateReconciled
     * 
     * @brief set warm start state reconciled
     * 
     * @return none
     */
    virtual void setWarmStartStateReconciled(){swss::WarmStart::setWarmStartState("linkmgrd", swss::WarmStart::RECONCILED);};

    /**
    *@method getMuxModeConfig
    *
    *@brief retrieve mux mode config
    *
    *@return port to mux mode map
    */
    virtual std::map<std::string, std::string> getMuxModeConfig();

private:
    friend class test::MuxManagerTest;

    /**
    *@method handleGetMuxState
    *
    *@brief get state db MUX state
    *
    *@param portName (in)   MUX/port name
    *
    *@return none
    */
    void handleGetMuxState(const std::string portName);

    /**
    *@method handleSetPeerMuxState
    *
    *@brief set peer MUX state in APP DB for orchagent processing
    *
    *@param portName (in)   MUX/port name
    *@param label (in)      label of target state
    *
    *@return none
    */
    virtual void handleSetPeerMuxState(const std::string portName, mux_state::MuxState::Label label);

    /**
    *@method handleSetLinkProberSessionState
    *
    *@brief set link prober session state in ASIC DB to simulate offload
    *
    *@param sessionId (in)  link prober session id
    *@param label (in)      label of target state
    *
    *@return none
    */
    void handleSetLinkProberSessionState(const std::string &sessionId, link_prober::LinkProberState::Label label);

    /**
    *@method handleProbeMuxState
    *
    *@brief trigger xcvrd to read MUX state using i2c
    *
    *@param portName (in)   MUX/port name
    *
    *@return label of MUX state
    */
    void handleProbeMuxState(const std::string portName);

    /**
     * @method handleProbeForwardingState
     * 
     * @brief trigger xcvrd to read forwarding state using gRPC
     * 
     * @param portName (in) MUX/port name
     * 
     * @return none
     */
    virtual void handleProbeForwardingState(const std::string portName);

    /**
    *@method handleSetMuxLinkmgrState
    *
    *@brief set MUX LinkMgr state in State DB for cli processing
    *
    *@param portName (in)   MUX/port name
    *@param label (in)      label of target state
    *
    *@return none
    */
    void handleSetMuxLinkmgrState(const std::string portName, link_manager::ActiveStandbyStateMachine::Label label);

    /**
     * @method handlePostLinkProberMetrics
     * 
     * @brief post link prober event to state db 
     * 
     * @param portName (in) port name
     * @param metrics (in) metrics data
     * @param time (in) event time stamp
     * 
     * @return none 
    */
    void handlePostLinkProberMetrics(
        const std::string portName,
        link_manager::ActiveStandbyStateMachine::LinkProberMetrics metrics,
        boost::posix_time::ptime time
    );

    /**
     * @method handlePostPckLossRatio
     * 
     * @brief handle post pck loss ratio update 
     * 
     * @param portName (in) port name 
     * @param unknownEventCount (in) count of missing icmp packets
     * @param expectedPacketCount (in) count of expected icmp packets 
     * 
     * @return none
    */
    void handlePostPckLossRatio(
        const std::string portName,
        const uint64_t unknownEventCount, 
        const uint64_t expectedPacketCount
    );

    /**
     * @method handleSetMuxMode
     * 
     * @brief handle set mux mode 
     * 
     * @param portName (in) MUX port name
     * @param state (in) MUX mode state
     * 
     * @return none
     */
    virtual void handleSetMuxMode(const std::string &portName, const std::string state);

    /**
    *@method processTorMacAddress
    *
    *@brief retrieve ToR MAC address information
    *
    *@param mac (in)   ToR mac address
    *
    *@return none
    */
    inline void processTorMacAddress(std::string& mac);

    /**
    *@method getTorMacAddress
    *
    *@brief retrieve ToR MAC address information
    *
    *@param configDbConnector   config db connector
    *
    *@return none
    */
    void getTorMacAddress(std::shared_ptr<swss::DBConnector> configDbConnector);

    /**
     * @method getVlanNames
     * 
     * @brief  get vlan names
     * 
     * @param configDbConnector   config db connector
     * 
     * @return none
     */
    void getVlanNames(std::shared_ptr<swss::DBConnector> configDbConnector);

    /**
     * @method getVlanMacAddress
     * 
     * @brief retrieve Vlan MAC address information
     * 
     * @param vlanNames (in) vlan names 
     * 
     * @return none
     */
    void getVlanMacAddress(std::vector<std::string> &vlanNames);

    /**
     * @method processVlanMacAddress
     * 
     * @brief process Vlan Mac Address
     * 
     * @return none 
     */
    void processVlanMacAddress(std::string& mac);

    /**
    *@method processLoopback2InterfaceInfo
    *
    *@brief process Loopback2 interface information
    *
    *@param loopbackIntfs   config_db Loopback2 entries
    *
    *@return none
    */
    inline void processLoopback2InterfaceInfo(std::vector<std::string> &loopbackIntfs);

    /**
    *@method getLoopback2InterfaceInfo
    *
    *@brief retrieve Loopback2 interface information
    *
    *@param configDbConnector   config db connector
    *
    *@return none
    */
    void getLoopback2InterfaceInfo(std::shared_ptr<swss::DBConnector> configDbConnector);

    /**
    *@method processServerIpAddress
    *
    *@brief process server/blades IP address and builds a map of IP to port name
    *
    *@param entries   config_db MUX_CABLE entries
    *
    *@return none
    */
    inline void processServerIpAddress(std::vector<swss::KeyOpFieldsValuesTuple> &entries);

    /**
    *@method getServerIpAddress
    *
    *@brief retrieve server/blades IP address and builds a map of IP to port name
    *
    *@param configDbConnector   config db connector
    *
    *@return none
    */
    void getServerIpAddress(std::shared_ptr<swss::DBConnector> configDbConnector);

    /**
    *@method processPortCableType
    *
    *@brief process port cable type and build a map of port name to cable type
    *
    *@param entries   config_db MUX_CABLE entries
    *
    *@return none
    */
    inline void processPortCableType(std::vector<swss::KeyOpFieldsValuesTuple> &entries);

   /**
    *@method getPortCableType
    *
    *@brief retrieve per port cable type
    *
    *@param configDbConnector   config db connector
    *
    *@return none
    */
    void getPortCableType(std::shared_ptr<swss::DBConnector> configDbConnector);

    /**
    *@method processSoCIpAddress
    *
    *@brief process SoC IP address and builds a map of IP to port name
    *
    *@param entries   config_db MUX_CABLE entries
    *
    *@return none
    */
    inline void processSoCIpAddress(std::vector<swss::KeyOpFieldsValuesTuple> &entries);

    /**
    *@method getSoCIpAddress
    *
    *@brief retrieve SoC IP address and builds a map of IP to port name
    *
    *@param configDbConnector   config db connector
    *
    *@return none
    */
    void getSoCIpAddress(std::shared_ptr<swss::DBConnector> configDbConnector);

    /**
    *@method processMuxPortConfigNotifiction
    *
    *@brief process MUX port configuration change notification
    *
    *@param entries (in) reference to changed entries of MUX config table
    *
    *@return none
    */
    inline void processMuxPortConfigNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);

    /**
    *@method handleMuxPortConfigNotifiction
    *
    *@brief handles MUX port configuration change notification
    *
    *@param configMuxTable (in) reference to MUX config table
    *
    *@return none
    */
    void handleMuxPortConfigNotifiction(swss::SubscriberStateTable &configMuxTable);

    /**
    *@method processMuxLinkmgrConfigNotifiction
    *
    *@brief process MUX Linkmgr configuration change notification
    *
    *@param entries (in) reference to MUX linkmgr config entries
    *
    *@return none
    */
    inline void processMuxLinkmgrConfigNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);

    /**
    *@method handleMuxLinkmgrConfigNotifiction
    *
    *@brief handles MUX Linkmgr configuration change notification
    *
    *@param configLocalhostTable (in) reference to MUX linkmgr config table
    *
    *@return none
    */
    void handleMuxLinkmgrConfigNotifiction(swss::SubscriberStateTable &configLocalhostTable);

    /**
    *@method processLinkStateNotifiction
    *
    *@brief process link state change notification
    *
    *@param entries (in) reference to app db port table
    *
    *@return none
    */
    inline void processLinkStateNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);

    /**
    *@method handleLinkStateNotifiction
    *
    *@brief handles link state change notification
    *
    *@param appdbPortTable (in) reference to app db port table
    *
    *@return none
    */
    void handleLinkStateNotifiction(swss::SubscriberStateTable &appdbPortTable);

    /**
     * @method processPeerLinkStateNotification
     * 
     * @brief process peer link status change notification
     * 
     * @param entries reference to state db mux cable info table 
     * 
     * @return none
    */
    inline void processPeerLinkStateNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries);

    /**
     * @method handlePeerLinkStateNotification
     * 
     * @brief handle peer's link status change notification 
     * 
     * @param stateDbMuxInfoTable reference to state db mux info table 
     * 
     * @return none
    */
    void handlePeerLinkStateNotification(swss::SubscriberStateTable &stateDbMuxInfoTable);

    /**
    *@method processMuxResponseNotifiction
    *
    *@brief process MUX response (from xcvrd) notification
    *
    *@param entries (in) reference to app db port entries
    *
    *@return none
    */
    inline void processMuxResponseNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);

    /**
     * @method processForwardingResponseNotification
     * 
     * @brief process forwarding response (from xcvrd probing) notification
     * 
     * @param entries (in) reference to app db entries
     * 
     * @return none
     */
    inline void processForwardingResponseNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries);

    /**
    *@method handleMuxResponseNotifiction
    *
    *@brief handles MUX response (from xcvrd) notification
    *
    *@param appdbPortTable (in) reference to app db port table
    *
    *@return none
    */
    void handleMuxResponseNotifiction(swss::SubscriberStateTable &appdbPortTable);

    /**
     * @method handleForwardingResponseNotification
     * 
     * @brief handle forwarding state response (from xcvrd probing) notification
     */
    void handleForwardingResponseNotification(swss::SubscriberStateTable &appdbForwardingResponseTable);

    /**
    *@method processPeerMuxNotification
    *
    *@brief process peer MUX state (from xcvrd) notification
    *
    *@param entries (in) reference to state db peer mux table entries
    *
    *@return none
    */
    inline void processPeerMuxNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries);

    /**
    *@method handlePeerMuxStateNotification
    *
    *@brief handles peer MUX notification (from xcvrd) 
    *
    *@param stateDbPeerMuxTable (in) reference to state db peer hw forwarding table 
    *
    *@return none
    */
    void handlePeerMuxStateNotification(swss::SubscriberStateTable &stateDbPeerMuxTable);

    /**
    *@method processMuxStateNotifiction
    *
    *@brief processes MUX state (from orchagent) notification
    *
    *@param entries (in) reference to state db port entries
    *
    *@return none
    */
    inline void processMuxStateNotifiction(std::deque<swss::KeyOpFieldsValuesTuple> &entries);

    /**
    *@method handleMuxStateNotifiction
    *
    *@brief handles MUX state (from orchagent) notification
    *
    *@param statedbPortTable (in) reference to state db port table
    *
    *@return none
    */
    void handleMuxStateNotifiction(swss::SubscriberStateTable &statedbPortTable);

    /**
    *@method processLinkProberSessionStateNotification
    *
    *@brief process link prober session state notification
    *
    *@param entries (in) reference to link prober session state notification
    *
    *@return none
    */
    inline void processLinkProberSessionStateNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries);

    /**
    *@method handleLinkProberSessionStateNotification
    *
    *@brief handle link prober session state notification
    *
    *@return none
    */
    void handleLinkProberSessionStateNotification(swss::NotificationConsumer &linkProberNotificationChannel);

    /**
    *@method handleSwssNotification
    *
    *@brief main thread method for handling SWSS notification
    *
    *@return none
    */
    void handleSwssNotification();

    /**
     * @method processDefaultRouteStateNotification
     * 
     * @brief process default route state notification from orchagent
     * 
     * @param entries reference to state db default route state entries
     * 
     * @return none
    */
    void processDefaultRouteStateNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries);

    /**
     * @method handleDefaultRouteStateNotification
     * 
     * @brief handle Default Route State notification from orchagent
     * 
     * @param statedbRouteTable reference to state db route table 
     * 
     * @return none
    */
    void handleDefaultRouteStateNotification(swss::SubscriberStateTable &statedbRouteTable);

    /**
     * @method handleBgpDeviceGlobal
     * 
     * @brief handle tsa_enable notification from BGP Device Global table 
     * 
     * @return none
     */
    void handleBgpDeviceGlobal(swss::SubscriberStateTable &configDbBgpDeviceGlobalTable);

    /**
     * @method processTsaEnableNotification
     * 
     * @brief process Tsa Enable Notification
     * 
     * @return none
     */
    void processTsaEnableNotification(std::deque<swss::KeyOpFieldsValuesTuple> &entries);

private:
    static std::vector<std::string> mMuxState;
    static std::vector<std::string> mLinkProberState;
    static std::vector<std::string> mMuxLinkmgrState;
    static std::vector<std::string> mMuxMetrics;
    static std::vector<std::string> mLinkProbeMetrics;
    static std::vector<std::string> mActiveStandbySwitchCause;

private:
    mux::MuxManager *mMuxManagerPtr;
    bool mPollSwssNotifcation = true;

    std::shared_ptr<swss::DBConnector> mAppDbPtr;
    std::shared_ptr<swss::DBConnector> mStateDbPtr;
    std::shared_ptr<swss::DBConnector> mAsicDbPtr;
    std::shared_ptr<swss::Table> mMuxStateTablePtr;

    // for communicating with orchagent
    std::shared_ptr<swss::ProducerStateTable> mAppDbMuxTablePtr;
    // for communication with driver (setting peer's forwarding state)
    std::shared_ptr<swss::Table> mAppDbPeerMuxTablePtr;
    // for communicating with the driver (probing the mux)
    std::shared_ptr<swss::Table> mAppDbMuxCommandTablePtr;
    // for communication with the driver (probing forwarding state)
    std::shared_ptr<swss::Table> mAppDbForwardingCommandTablePtr;
    // for writing the current mux linkmgr health
    std::shared_ptr<swss::Table> mStateDbMuxLinkmgrTablePtr;
    // for writing mux metrics
    std::shared_ptr<swss::Table> mStateDbMuxMetricsTablePtr;
    // for writing link probe statistics data
    std::shared_ptr<swss::Table> mStateDbLinkProbeStatsTablePtr;
    // for writing mux switch reason to state db 
    std::shared_ptr<swss::Table> mStateDbSwitchCauseTablePtr;
    // for writing link prober session state change notification
    std::shared_ptr<swss::NotificationProducer> mAsicDbNotificationChannelPtr;

    std::shared_ptr<boost::thread> mSwssThreadPtr;

    boost::barrier mBarrier;

    boost::asio::io_service::strand mStrand;

    ServerIpPortMap mServerIpPortMap;
};

} /* namespace common */

#endif /* DBINTERFACE_H_ */
