/*
 * FakeLinkProber.h
 *
 *  Created on: Oct 23, 2020
 *      Author: tamer
 */

#ifndef FAKELINKPROBER_H_
#define FAKELINKPROBER_H_

#include "link_prober/LinkProberStateMachine.h"
namespace test
{

class FakeLinkProber
{
public:
    FakeLinkProber(link_prober::LinkProberStateMachine *linkProberStateMachine);
    virtual ~FakeLinkProber() = default;

    template<class E>
    void postLinkProberEvent(E &e);

    void postSuspendTimerExpiredEvent();

    void initialize();
    void startProbing();
    void updateEthernetFrame();
    void probePeerTor();
    void suspendTxProbes(uint32_t suspendTime_msec);
    void resumeTxProbes();
    void sendPeerSwitchCommand();

public:
    uint32_t mInitializeCallCount = 0;
    uint32_t mStartProbingCallCount = 0;
    uint32_t mUpdateEthernetFrameCallCount = 0;
    uint32_t mProbePeerTorCallCount = 0;
    uint32_t mSuspendTxProbeCallCount = 0;
    uint32_t mResumeTxProbeCallCount = 0;
    uint32_t mSendPeerSwitchCommand = 0;

private:
    link_prober::LinkProberStateMachine *mLinkProberStateMachine;
};

} /* namespace test */

#endif /* FAKELINKPROBER_H_ */
