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
#ifndef LINK_PROBER_LINKPROBERSESSIONSTATEMACHINE_H_
#define LINK_PROBER_LINKPROBERSESSIONSTATEMACHINE_H_

#include "link_prober/LinkProberStateMachineBase.h"

namespace link_manager
{
class LinkManagerStateMachineBase;
} /* namespace link_manager */

namespace mux
{
class MuxPort;
} /* namespace mux */

namespace link_prober
{

/**
 *@class LinkProberSessionStateMachine
 *
 *@brief maintains LinkProber session state machine
 */
class LinkProberSessionStateMachine : public LinkProberStateMachineBase
{
public:
    using LinkProberStateMachineBase::postLinkProberStateEvent;

public:
    /**
    *@method LinkProberSessionStateMachine
    *
    *@brief class default constructor
    */
    LinkProberSessionStateMachine() = delete;

    /**
    *@method LinkProberSessionStateMachine
    *
    *@brief class copy constructor
    *
    *@param LinkProberSessionStateMachine (in)  reference to LinkProberSessionStateMachine object to be copied
    */
    LinkProberSessionStateMachine(const LinkProberSessionStateMachine &) = delete;

    /**
    *@method LinkProberSessionStateMachine
    *
    *@brief class constructor
    *
    *@param linkManagerStateMachinePtr (in) pointer to LinkManagerStateMachineBase
    *@param muxPort (in)                    reference to MuxPort object
    *@param strand (in)                     reference to boost serialization object
    *@param muxPortConfig (in)              reference to MuxPortConfig object
    *@param selfLabel (in)                  self session initial state
    *@param peerLabel (in)                  peer session initial state
    */
    LinkProberSessionStateMachine(
        link_manager::LinkManagerStateMachineBase *linkManagerStateMachinePtr,
        mux::MuxPort *muxPort,
        boost::asio::io_service::strand &strand,
        common::MuxPortConfig &muxPortConfig,
        LinkProberState::Label selfLabel,
        LinkProberState::Label peerLabel
    );

    /**
    *@method ~LinkProberSessionStateMachine
    *
    *@brief class destructor
    */
    virtual ~LinkProberSessionStateMachine() = default;

    /**
     *@method enterState
     *
     *@brief force the state machine to enter a given state
     *
     *@param label (in)  label of target state
     *
     *@return none
     */
    void enterState(LinkProberState::Label label) override;

    /**
     *@method enterPeerState
     *
     *@brief force the state machine to enter a given peer state
     *
     *@param label (in)  label of target peer state
     *
     *@return none
     */
    void enterPeerState(LinkProberState::Label label) override;

    /**
     *@method getCurrentPeerState
     *
     *@brief getter for current peer state
     *
     *@return current peer state of the state machine
     */
    LinkProberState *getCurrentPeerState() override;

    /**
     *@method postLinkProberStateEvent
     *
     *@brief post LinkProberState event to the state machine
     *
     *@param e (in)  reference to the LinkProberState event
     *
     *@return none
     */
    template <class E>
    void postLinkProberStateEvent(E &e);

    /**
     *@method processEvent
     *
     *@brief process IcmpSelfEvent
     *
     *@param icmpSelfEvent (in)  reference to the IcmpSelfEvent event
     *
     *@return none
     */
    void processEvent(IcmpSelfEvent &icmpSelfEvent);

    /**
     *@method processEvent
     *
     *@brief process IcmpUnknownEvent
     *
     *@param icmpUnknownEvent (in)  reference to the IcmpUnknownEvent event
     *
     *@return none
     */
    void processEvent(IcmpUnknownEvent &icmpUnknownEvent);

    /**
     *@method processEvent
     *
     *@brief process IcmpPeerActiveEvent
     *
     *@param icmpPeerActiveEvent (in)  reference to the IcmpPeerActiveEvent event
     *
     *@return none
     */
    void processEvent(IcmpPeerActiveEvent &icmpPeerActiveEvent) override;

    /**
     *@method processEvent
     *
     *@brief process IcmpPeerUnknownEvent
     *
     *@param icmpPeerUnknownEvent (in)  reference to the IcmpPeerUnknownEvent event
     *
     *@return none
     */
    void processEvent(IcmpPeerUnknownEvent &icmpPeerUnknownEvent) override;

public:
    /**
     *@method getSelfSessionId
     *
     *@brief getter for self session id
     *
     *@return self session id
     */
    const std::string &getSelfSessionId() {return mSelfSessionId;};

    /**
     *@method getPeerSessionId
     *
     *@brief getter for peer session id
     *
     *@return peer session id
     */
    const std::string &getPeerSessionId() {return mPeerSessionId;};

private:
    /**
     *@method setCurrentPeerState
     *
     *@brief setter for current peer state
     *
     *@param state (in)  current peer state of the state machine
     *
     *@return none
     */
    void setCurrentPeerState(LinkProberState *state);

private:
    /**
     *@method postLinkProberSessionNotificationToDb
     *
     *@brief post self link prober session change event to DB
     *
     *@param state (in)  pointer to current self session state
     *
     *@return none
     */
    inline void postLinkProberSessionNotificationToDb(LinkProberState *state);

    /**
     *@method postLinkProberPeerSessionNotificationToDb
     *
     *@brief post peer link prober session change event to DB
     *
     *@param state (in)  pointer to current peer session state
     *
     *@return none
     */
    inline void postLinkProberPeerSessionNotificationToDb(LinkProberState *state);

private:
    std::string mSelfSessionId;
    std::string mPeerSessionId;

    LinkProberState *mCurrentPeerState = nullptr;
    mux::MuxPort *mMuxPort = nullptr;
};

} /* namespace link_prober */

#endif /* LINK_PROBER_LINKPROBERSESSIONSTATEMACHINE_H_ */
