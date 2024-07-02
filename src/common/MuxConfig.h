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
 * MuxConfig.h
 *
 *  Created on: Oct 9, 2020
 *      Author: Tamer Ahmed
 */

#ifndef MUXCONFIG_H_
#define MUXCONFIG_H_

#include <string>
#include <net/ethernet.h>

#include <common/BoostAsioBehavior.h>
#include <boost/asio.hpp>

namespace common
{

/**
 *@class MuxConfig
 *
 *@brief Holds MUX configuration
 */
class MuxConfig
{
public:
    /**
    *@method MuxConfig
    *
    *@brief class default constructor
    */
    MuxConfig() = default;

    /**
    *@method MuxConfig
    *
    *@brief class copy constructor
    *
    *@param MuxConfig (in)  reference to MuxConfig object to be copied
    */
    MuxConfig(const MuxConfig &) = delete;

    /**
    *@method ~MuxConfig
    *
    *@brief class destructor
    */
    virtual ~MuxConfig() = default;

    /**
    *@method setNumberOfThreads
    *
    *@brief setter for number of threads
    *
    *@param numberOfThreads (in)  number of threads of linkmgrd
    *
    *@return none
    */
    inline void setNumberOfThreads(uint8_t numberOfThreads) {mNumberOfThreads = numberOfThreads;};

    /**
    *@method setTimeoutIpv4_msec
    *
    *@brief setter for IPv4 LinkProber timeout in msec
    *
    *@param timeout_msec (in)  timeout in msec
    *
    *@return none
    */
    inline void setTimeoutIpv4_msec(uint32_t timeout_msec) {mTimeoutIpv4_msec = timeout_msec;};

    /**
    *@method setTimeoutIpv6_msec
    *
    *@brief setter for IPv6 LinkProber timeout in msec
    *
    *@param timeout_msec (in)  timeout in msec
    *
    *@return none
    */
    inline void setTimeoutIpv6_msec(uint32_t timeout_msec) {mTimeoutIpv6_msec = timeout_msec;};

    /**
    *@method setOscillationEnabled
    *
    *@brief setter for enabling/disabling mux state oscillation
    *
    *@param enable (in)  enable/disable mux state oscillation
    *
    *@return none
    */
    inline void setOscillationEnabled(bool enable) {mEnableTimedOscillationWhenNoHeartbeat = enable;};

    /**
    *@method setLinkProberStatUpdateIntervalCount
    *
    *@brief setter for link prober stats posting interval
    *
    *@param interval_count (in)  interval in heartbeat count
    *
    *@return none
    */
    inline void setLinkProberStatUpdateIntervalCount(uint32_t interval_count) {mLinkProberStatUpdateIntervalCount = interval_count > 50? interval_count:50;};

    /**
    *@method setPositiveStateChangeRetryCount
    *
    *@brief setter for LinkProber positive state change retry count
    *
    *@param stateChangeRetryCount (in)  state change retry count
    *
    *@return none
    */
    inline void setPositiveStateChangeRetryCount(uint32_t stateChangeRetryCount) {
        mPositiveStateChangeRetryCount = stateChangeRetryCount;
    };

    /**
    *@method setNegativeStateChangeRetryCount
    *
    *@brief setter for LinkProber negative state change retry count
    *
    *@param stateChangeRetryCount (in)  state change retry count
    *
    *@return none
    */
    inline void setNegativeStateChangeRetryCount(uint32_t stateChangeRetryCount) {
        mNegativeStateChangeRetryCount = stateChangeRetryCount;
    };

    /**
    *@method setSuspendTimeout_msec
    *
    *@brief setter for LinkProber suspend timer timeout
    *
    *@param suspendTimeout_msec (in)  suspend timer timeout
    *
    *@return none
    */
    inline void setSuspendTimeout_msec(uint32_t suspendTimeout_msec) {mSuspendTimeout_msec = suspendTimeout_msec;};

    /**
    *@method setMuxStateChangeRetryCount
    *
    *@brief setter for MuxState state change retry count
    *
    *@param muxStateChangeRetryCount (in)  state change retry count
    *
    *@return none
    */
    inline void setMuxStateChangeRetryCount(uint32_t muxStateChangeRetryCount) {mMuxStateChangeRetryCount = muxStateChangeRetryCount;};

    /**
    *@method setLinkStateChangeRetryCount
    *
    *@brief setter for LinkeState change retry count
    *
    *@param linkStateChangeRetryCount (in)  state change retry count
    *
    *@return none
    */
    inline void setLinkStateChangeRetryCount(uint32_t linkStateChangeRetryCount) {mLinkStateChangeRetryCount = linkStateChangeRetryCount;};

    /**
    *@method setTorMacAddress
    *
    *@brief setter for ToR MAC address
    *
    *@param address (in) ToR MAC address
    *
    *@return none
    */
    inline void setTorMacAddress(const std::array<uint8_t, ETHER_ADDR_LEN> &address) {mTorMacAddress = address;};

    /**
     * @method setVlanMacAddress
     * 
     * @brief setter for Vlan MAC address
     * 
     * @param address (in) Vlan MAC address
     * 
     * @return none
     */
    inline void setVlanMacAddress(const std::array<uint8_t, ETHER_ADDR_LEN> &address) {mVlanMacAddress = address;};

    /**
     * @method setIfUseTorMacAsSrcMac
     * 
     * @brief setter for flag whether use ToR MAC address as link prober src MAC 
     * 
     * @param enable (in) bool 
     * 
     * @return none
     */
    inline void setIfUseTorMacAsSrcMac(bool enable) {mEnableUseTorMac = enable;};

    /**
    *@method setLoopbackIpv4Address
    *
    *@brief setter for Loopback IPv4 address
    *
    *@param address (in)  IPv4 address
    *
    *@return none
    */
    inline void setLoopbackIpv4Address(boost::asio::ip::address& address) {mLoopbackIpv4Address = address;};

    /**
    *@method getNumberOfThreads
    *
    *@brief getter for logging severity level
    *
    *@return number of linkmgrd/application threads
    */
    inline uint8_t getNumberOfThreads() const {return mNumberOfThreads;};

    /**
    *@method getTimeoutIpv4_msec
    *
    *@brief getter for IPv4 LinkProber timeout in msec
    *
    *@return timeout in msec
    */
    inline uint32_t getTimeoutIpv4_msec() const {return mTimeoutIpv4_msec;};

    /**
    *@method getTimeoutIpv6_msec
    *
    *@brief getter for IPv6 LinkProber timeout in msec
    *
    *@return timeout in msec
    */
    inline uint32_t getTimeoutIpv6_msec() const {return mTimeoutIpv6_msec;};

    /**
    *@method getPositiveStateChangeRetryCount
    *
    *@brief getter for LinkProber positive state change retry count
    *
    *@return state change retry count
    */
    inline uint32_t getPositiveStateChangeRetryCount() const {return mPositiveStateChangeRetryCount;};

    /**
    *@method getNegativeStateChangeRetryCount
    *
    *@brief getter for LinkProber negative state change retry count
    *
    *@return state change retry count
    */
    inline uint32_t getNegativeStateChangeRetryCount() const {return mNegativeStateChangeRetryCount;};

    /**
    *@method getLinkProberStatUpdateIntervalCount
    *
    *@brief getter for LinkProber negative state change retry count
    *
    *@return state change retry count
    */
    inline uint32_t getLinkProberStatUpdateIntervalCount() const {return mLinkProberStatUpdateIntervalCount;};

    /**
    *@method getSuspendTimeout_msec
    *
    *@brief getter for LinkProber suspend timer timeout
    *
    *@return suspend timer timeout
    */
    inline uint32_t getSuspendTimeout_msec() const {return (mNegativeStateChangeRetryCount + 1) * mTimeoutIpv4_msec;};

    /**
    *@method getIfOscillationEnabled
    *
    *@brief getter for oscillation flag
    *
    *@return oscillation flag
    */
    inline bool getIfOscillationEnabled() const {return mEnableTimedOscillationWhenNoHeartbeat;};

    /**
    *@method getOscillationInterval_sec
    *
    *@brief getter for mux state oscillation interval
    *
    *@return oscillation interval
    */
    inline uint32_t getOscillationInterval_sec() const {return mOscillationTimeout_sec;};

    /**
    *@method setOscillationInterval_sec
    *
    *@brief setter for mux state oscillation interval
    *
    *@param oscillationInterval_sec (in) oscillation interval
    *@param force (in) force set the oscillation interval
    *
    *@return none
    */
    inline void setOscillationInterval_sec(uint32_t oscillationInterval_sec, bool force=false) {
        if (force || oscillationInterval_sec > 300) {
            mOscillationTimeout_sec = oscillationInterval_sec;
        } else {
            mOscillationTimeout_sec = 300;
        }
    };

    /**
    *@method getMuxStateChangeRetryCount
    *
    *@brief getter for MuxState state change retry count
    *
    *@return state change retry count
    */
    inline uint32_t getMuxStateChangeRetryCount() const {return mMuxStateChangeRetryCount;};

    /**
    *@method getLinkStateChangeRetryCount
    *
    *@brief getter for LinkeState change retry count
    *
    *@return state change retry count
    */
    inline uint32_t getLinkStateChangeRetryCount() const {return mLinkStateChangeRetryCount;};

    /**
    *@method getTorMacAddress
    *
    *@brief getter for ToR MAC address
    *
    *@return ToR MAC address
    */
    inline const std::array<uint8_t, ETHER_ADDR_LEN>& getTorMacAddress() const {return mTorMacAddress;};

    /**
     * @method getVlanMacAddress
     * 
     * @brief getter for Vlan MAC address
     * 
     * @return Vlan MAC address
     */
    inline const std::array<uint8_t, ETHER_ADDR_LEN>& getVlanMacAddress() const {return mVlanMacAddress;};

    /**
    *@method getLoopbackIpv4Address
    *
    *@brief getter for Loopback IPv4 address
    *
    *@return IPv4 address
    */
    inline boost::asio::ip::address getLoopbackIpv4Address() {return mLoopbackIpv4Address;};
    
    /**
    *@method getDecreasedTimeoutIpv4_msec
    *
    *@brief getter for decreased IPv4 LinkProber timeout in msec
    *
    *@return timeout in msec
    */
    inline uint32_t getDecreasedTimeoutIpv4_msec() const {return mDecreasedTimeoutIpv4_msec;};

    /**
     * @method getIfEnableSwitchoverMeasurement
     * 
     * @brief check if the feature that decreases link prober interval to measure switch overhead is enabled or not 
     * 
     * @return if switch overhead measurement feature is enabled
     */
    inline bool getIfEnableSwitchoverMeasurement() {return mEnableSwitchoverMeasurement;};

    /**
     * enableSwitchoverMeasurement
     * 
     * @brief enable or disable the feature that decreases link prober interval to measure switch overhead
     * 
     * @param enable_feature (in) enable feature 
     * 
     * @return  none
     */
    inline void enableSwitchoverMeasurement(bool enable_feature) {mEnableSwitchoverMeasurement = enable_feature;};

    /**
     * @method enableDefaultRouteFeature
     * 
     * @brief enable or disable the feature that shutdowns link prober & avoid switching active when defaul route is missing
     * 
     * @param enable_feature (in) enable feature
     * 
     * @return none 
     */
    inline void enableDefaultRouteFeature(bool enable_feature) {mEnableDefaultRouteFeature = enable_feature;};

    /**
     * @method getIfEnableDefaultRouteFeature
     * 
     * @brief check if default route related feature is enabled or not 
     * 
     * @return if default route related feature is enabled or not
     */
    inline bool getIfEnableDefaultRouteFeature() {return mEnableDefaultRouteFeature;};

    /**
     * @method getIfUseWellKnownMacActiveActive
     * 
     * @brief check if use well known mac to probe for active-active ports
     * 
     * @return true to use well known mac to probe for active-active ports
     */
    inline bool getIfUseWellKnownMacActiveActive() { return mUseWellKnownMacActiveActive; };

    /**
     * @method setUseWellKnownMacActiveActive
     * 
     * @brief setter to set if use well known mac to probe for active-active ports
     * 
     * @return none
     */
    inline void setUseWellKnownMacActiveActive(bool useWellKnownMacActiveActive) { mUseWellKnownMacActiveActive = useWellKnownMacActiveActive; };

    /**
     * @method getIfEnableUseTorMac
     * 
     * @brief check if use ToR MAC address as src MAC for link prober 
     * 
     * @return if use ToR MAC 
     */
    inline bool getIfEnableUseTorMac() {return mEnableUseTorMac;};

    /**
     * @method getMuxReconciliationTimeout
     * 
     * @brief getter of mux reconciliation time out 
     * 
     * @return timeout in sec
     */
    inline uint32_t getMuxReconciliationTimeout_sec(){return mMuxReconciliationTimeout_sec;};

    /**
    *@method enableSimulateLfdOffload
    *
    *@brief setter for simulate lfd offload enable flag
    *
    *@return enable simulate lfd offload flag
    */
    inline void enableSimulateLfdOffload(bool enableSimulateLfdOffload) { mEnableSimulateLfdOffload = enableSimulateLfdOffload; };

    /**
    *@method getIfEnableSimulateLfdOffload
    *
    *@brief getter for simulate lfd offload enable flag
    *
    *@return enable simulate lfd offload flag
    */
    inline bool getIfEnableSimulateLfdOffload() { return mEnableSimulateLfdOffload; };

private:
    uint8_t mNumberOfThreads = 5;
    uint32_t mTimeoutIpv4_msec = 100;
    uint32_t mTimeoutIpv6_msec = 1000;
    uint32_t mPositiveStateChangeRetryCount = 1;
    uint32_t mNegativeStateChangeRetryCount = 3;
    uint32_t mLinkProberStatUpdateIntervalCount = 300; 
    uint32_t mSuspendTimeout_msec = 500;
    uint32_t mMuxStateChangeRetryCount = 1;
    uint32_t mLinkStateChangeRetryCount = 1;

    bool mEnableTimedOscillationWhenNoHeartbeat = true;
    uint32_t mOscillationTimeout_sec = 300;

    bool mEnableSwitchoverMeasurement = false;
    uint32_t mDecreasedTimeoutIpv4_msec = 10;

    uint32_t mMuxReconciliationTimeout_sec = 10;

    bool mEnableDefaultRouteFeature = false;
    bool mUseWellKnownMacActiveActive = true;
    bool mEnableSimulateLfdOffload = false;

    bool mEnableUseTorMac = false;

    std::array<uint8_t, ETHER_ADDR_LEN> mTorMacAddress;
    std::array<uint8_t, ETHER_ADDR_LEN> mVlanMacAddress;
    boost::asio::ip::address mLoopbackIpv4Address = boost::asio::ip::make_address("10.212.64.0");
};

} /* namespace common */

#endif /* MUXCONFIG_H_ */
