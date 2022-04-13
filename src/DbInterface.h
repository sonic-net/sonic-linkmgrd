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

#include "link_manager/LinkManagerStateMachineActiveStandby.h"
#include "mux_state/MuxState.h"

namespace test {
class MuxManagerTest;
}

namespace mux
{
#define MUX_CABLE_INFO_TABLE  "MUX_CABLE_INFO"
#define LINK_PROBE_STATS_TABLE_NAME "LINK_PROBE_STATS" 

class MuxManager;
using ServerIpPortMap = std::map<boost::asio::ip::address, std::string>;
using SoCIpPortMap = std::map<boost::asio::ip::address, std::string>;

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
    virtual void setMuxState(const std::string &portName, mux_state::MuxState::Label label);

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
    virtual void postMetricsEvent(
        const std::string &portName,
        link_manager::ActiveStandbyStateMachine::Metrics metrics,
        mux_state::MuxState::Label label
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
    *@method handleSetMuxState
    *
    *@brief set MUX state in APP DB for orchagent processing
    *
    *@param portName (in)   MUX/port name
    *@param label (in)      label of target state
    *
    *@return none
    */
    void handleSetMuxState(const std::string portName, mux_state::MuxState::Label label);

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
    void handlePostMuxMetrics(
        const std::string portName,
        link_manager::ActiveStandbyStateMachine::Metrics metrics,
        mux_state::MuxState::Label label,
        boost::posix_time::ptime time
    );

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

private:
    static std::vector<std::string> mMuxState;
    static std::vector<std::string> mMuxLinkmgrState;
    static std::vector<std::string> mMuxMetrics;
    static std::vector<std::string> mLinkProbeMetrics;

private:
    mux::MuxManager *mMuxManagerPtr;
    bool mPollSwssNotifcation = true;

    std::shared_ptr<swss::DBConnector> mAppDbPtr;
    std::shared_ptr<swss::DBConnector> mStateDbPtr;
    std::shared_ptr<swss::Table> mMuxStateTablePtr;

    // for communicating with orchagent
    std::shared_ptr<swss::ProducerStateTable> mAppDbMuxTablePtr;
    // for communicating with the driver (probing the mux)
    std::shared_ptr<swss::Table> mAppDbMuxCommandTablePtr;
    // for writing the current mux linkmgr health
    std::shared_ptr<swss::Table> mStateDbMuxLinkmgrTablePtr;
    // for writing mux metrics
    std::shared_ptr<swss::Table> mStateDbMuxMetricsTablePtr;
    // for writing link probe statistics data
    std::shared_ptr<swss::Table> mStateDbLinkProbeStatsTablePtr;

    std::shared_ptr<boost::thread> mSwssThreadPtr;

    boost::barrier mBarrier;

    boost::asio::io_service::strand mStrand;

    ServerIpPortMap mServerIpPortMap;
    SoCIpPortMap mSoCIpPortMap;
};

} /* namespace common */

#endif /* DBINTERFACE_H_ */
