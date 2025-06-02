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
 * FakeLinkProber.h
 *
 *  Created on: Oct 23, 2020
 *      Author: tamer
 */

#ifndef FAKELINKPROBER_H_
#define FAKELINKPROBER_H_

#include "link_prober/LinkProberStateMachineActiveStandby.h"
namespace test
{

class FakeLinkProber
{
public:
    FakeLinkProber(link_prober::LinkProberStateMachineBase *linkProberStateMachine);
    virtual ~FakeLinkProber() = default;

    template<class E>
    void postLinkProberEvent(E &e);

    void postSuspendTimerExpiredEvent();

    void initialize();
    void startProbing();
    void updateEthernetFrame();
    void probePeerTor();
    void detectLink();
    void suspendTxProbes(uint32_t suspendTime_msec);
    void resumeTxProbes();
    void sendPeerSwitchCommand();
    void sendPeerProbeCommand();
    void resetIcmpPacketCounts();
    void shutdownTxProbes();
    void restartTxProbes();
    void decreaseProbeIntervalAfterSwitch(uint32_t switchTime_msec);
    void revertProbeIntervalAfterSwitchComplete();
    void handleSendSwitchCommand();
    void handleSwitchCommandRecv();
    void handleMuxProbeCommandRecv();
    void handleStateDbStateUpdate();

public:
    uint32_t mInitializeCallCount = 0;
    uint32_t mStartProbingCallCount = 0;
    uint32_t mUpdateEthernetFrameCallCount = 0;
    uint32_t mProbePeerTorCallCount = 0;
    uint32_t mSuspendTxProbeCallCount = 0;
    uint32_t mDetectLinkCallCount = 0;
    uint32_t mResumeTxProbeCallCount = 0;
    uint32_t mSendPeerSwitchCommand = 0;
    uint32_t mSendPeerProbeCommand = 0;
    uint32_t mShutdownTxProbeCallCount = 0;
    uint32_t mRestartTxProbeCallCount = 0;

    uint64_t mIcmpUnknownEventCount = 0;
    uint64_t mIcmpPacketCount = 0;

    uint32_t mDecreaseIntervalCallCount = 0;
    uint32_t mRevertIntervalCallCount = 0;
    uint32_t mIcmpEchoSessionStateUpdateCallCount = 0;

private:
    link_prober::LinkProberStateMachineBase *mLinkProberStateMachine;
};

} /* namespace test */

#endif /* FAKELINKPROBER_H_ */
