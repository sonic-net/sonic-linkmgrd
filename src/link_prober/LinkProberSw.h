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
 * LinkProberSw.h
 *
 *  Created on: Oct 4, 2020
 *      Author: tamer
 */

#ifndef LINK_PROBER_LINKPROBERSW_H_
#define LINK_PROBER_LINKPROBERSW_H_

#include "LinkProberBase.h"

namespace test {
class LinkProberTest;
class LinkProberMockTest;
}

namespace link_prober
{
/**
 *@class LinkProberSw
 *
 *@brief probes the server sing ICMP ECHPREQUEST packet. The packet payload
 *       holds GUID that identifies this ToR. Reception of this ToR's GUID
 *       indicate that the link is in Active state. Reception of unknown
 *       GUID will indicate standby state. Lack of ICMP packets will signal
 *       that the link state is unknown.
 */
class LinkProberSw : public LinkProberBase
{
public:    
    /**
    *@method LinkProberSw
    *
    *@brief class default constructor
    */
    LinkProberSw() = delete;

    /**
    *@method LinkProberSw
    *
    *@brief class copy constructor
    *
    *@param LinkProberSw (in)  reference to LinkProberSw object to be copied
    */
    LinkProberSw(const LinkProberSw &) = delete;

    /**
    *@method LinkProberSw
    *
    *@brief class constructor
    *
    *@param muxPortConfig (in)          reference to MuxPortConfig object
    *@param ioService (in)              reference to boost io_service object
    *@param linkProberStateMachinePtr (in) reference to LinkProberStateMachineBase object
    */
    LinkProberSw(
        common::MuxPortConfig &muxPortConfig,
        boost::asio::io_service &ioService,
        LinkProberStateMachineBase *linkProberStateMachinePtr
    );

    /**
    *@method ~LinkProberSw
    *
    *@brief class destructor
    */
    virtual ~LinkProberSw() = default;


private:

    /**
    *@method handleTimeout
    *
    *@brief handle ICMP packet reception timeout
    *
    *@param errorCode (in)          socket error code
    *
    *@return none
    */
    void handleTimeout(boost::system::error_code errorCode);

    /**
    *@method handleSuspendTimeout
    *
    *@brief handle suspend timer timeout
    *
    *@param errorCode (in)          socket error code
    *
    *@return none
    */
    void handleSuspendTimeout(boost::system::error_code errorCode);

    /**
    *@method startTimer
    *
    *@brief start ICMP ECHOREPLY timeout timer
    *
    *@return none
    */
    void startTimer();

    /**
     * @method handleSwitchoverTimeout
     * 
     * @brief handle switchover time out 
     * 
     * @param errorCode (in) socket error code 
     * 
     * @return none
     */
    void handleSwitchoverTimeout(boost::system::error_code errorCode);

    friend class test::LinkProberTest;
    friend class test::LinkProberMockTest;

public:

    /**
    *@method startProbing
    *
    *@brief start probing server/blade using ICMP ECHOREQUEST
    *
    *@return none
    */
    virtual void startProbing() override;

    /**
    *@method initialize
    *
    *@brief initialize link prober sockets and builds ICMP packet
    *
    *@return none
    */
    virtual void initialize() override;

    /**
    *@method suspendTxProbes
    *
    *@brief suspend sending ICMP ECHOREQUEST packets
    *
    *@param suspendTime_msec suspension time in msec
    *
    *@return none
    */
    virtual void suspendTxProbes(uint32_t suspendTime_msec) override;

    /**
    *@method resumeTxProbes
    *
    *@brief resume sending ICMP ECHOREQUEST packets
    *
    *@return none
    */
    virtual void resumeTxProbes() override;

    /**
    *@method updateEthernetFrame
    *
    *@brief update Ethernet frame of Tx Buffer
    *
    *@return none
    */
    virtual void updateEthernetFrame() override;

    /**
    *@method detectLink
    *
    *@brief detect link status
    *
    *@return none
    */
    virtual void detectLink() override;

    /**
     * @method resetIcmpPacketCounts()
     * 
     * @brief reset Icmp packet counts, post a pck loss ratio update immediately 
     * 
     * @return none
    */
    virtual void resetIcmpPacketCounts() override;

    /**
    *@method shutdownTxProbes
    *
    *@brief stop sending ICMP ECHOREQUEST packets
    *
    *@return none
    */
    virtual void shutdownTxProbes() override;

    /**
    * @method restartTxProbes
    * 
    * @brief restart sending ICMP ECHOREQUEST packets
    * 
    * @return none
    */
    virtual void restartTxProbes() override;

    /**
     * @method decreaseProbeIntervalAfterSwitch
     *  
     * @brief adjust link prober interval to 10 ms after switchover to better measure the switchover overhead.
     * 
     * @param switchTime_msec (in) switchover is expected to complete  within this time window
     * @param expectingLinkProberEvent (in) depends on which state LinkManager is switching to, link prober expects self or peer events
     * 
     * @return none
     */
    virtual void decreaseProbeIntervalAfterSwitch(uint32_t switchTime_msec) override;

    /**
     * @method revertProbeIntervalAfterSwitchComplete 
     * 
     * @brief revert probe interval change after switchover is completed
     * 
     * @return none
     */
    virtual void revertProbeIntervalAfterSwitchComplete() override;

    /**
     * @method reportHeartbeatReplyReceivedActiveStandby
     * 
     * @brief report heartbeat reply received to active-standby mode link prober state machine
     * 
     * @return none
     */
    void reportHeartbeatReplyReceivedActiveStandby(HeartbeatType heartbeatType);

    /**
     * @method reportHeartbeatReplyNotReceivedActiveStandby
     * 
     * @brief report heartbeat reply not received to active-standby mode link prober state machine
     * 
     * @param heartbeatType (in) received heartbeat type
     *
     * @return none
     */
    void reportHeartbeatReplyNotReceivedActiveStandby(HeartbeatType heartbeatType);

    /**
     * @method reportHeartbeatReplyReceivedActiveActive
     * 
     * @brief report heartbeat reply received to active-active mode link prober state machine
     * 
     * @param heartbeatType (in) received heartbeat type
     *
     * @return none
     */
    void reportHeartbeatReplyReceivedActiveActive(HeartbeatType heartbeatType);

    /**
     * @method reportHeartbeatReplyNotReceivedActiveActive
     * 
     * @brief report heartbeat reply not received to active-active mode link prober state machine
     * 
     * @return none
     */
    void reportHeartbeatReplyNotReceivedActiveActive(HeartbeatType heartbeatType);

    /**
    *@method handleIcmpPayload
    *
    *@brief handle Icmp packet recieved on socket
    *
    *@return none
    */
    virtual void handleIcmpPayload(size_t bytesTransferred, icmphdr *icmpHeader, IcmpPayload *icmpPayload) override;
    virtual void handleIcmpPayload_pc(size_t bytesTransferred, icmphdr *icmpHeader, IcmpPayload *icmpPayload) override;

private:
};

} /* namespace link_prober */

#endif /* LINKPROBER_H_ */
