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

#ifndef LINK_MANAGER_LINKMANAGERSTATEMACHINEBASE_H_
#define LINK_MANAGER_LINKMANAGERSTATEMACHINEBASE_H_

#include <bitset>
#include <boost/function.hpp>
#include <functional>
#include <memory>
#include <string>
#include <tuple>
#include <vector>

#include "link_prober/LinkProber.h"
#include "link_prober/LinkProberState.h"
#include "link_state/LinkState.h"
#include "link_state/LinkStateMachine.h"
#include "mux_state/MuxState.h"
#include "mux_state/MuxStateMachine.h"

namespace mux {
#define ps(compositeState) std::get<0>(compositeState)
#define ms(compositeState) std::get<1>(compositeState)
#define ls(compositeState) std::get<2>(compositeState)

class MuxPort;
}; // namespace mux

namespace link_manager {
class ActiveStandbyStateMachine;

/**
 *@class LinkProberEvent
 *
 *@brief signals a LinkeProber event to the composite state machine
 */
class LinkProberEvent {
public:
    LinkProberEvent() = default;
    ~LinkProberEvent() = default;
};

/**
 *@class MuxStateEvent
 *
 *@brief signals a MuxState event to the composite state machine
 */
class MuxStateEvent {
public:
    MuxStateEvent() = default;
    ~MuxStateEvent() = default;
};

/**
 *@class LinkStateEvent
 *
 *@brief signals a LinkState event to the composite state machine
 */
class LinkStateEvent {
public:
    LinkStateEvent() = default;
    ~LinkStateEvent() = default;
};

/**
 *@class LinkManagerStateMachineBase
 *
 *@brief Abstrct composite state machine of LinkPorberState, MuxState, and LinkState
 */
class LinkManagerStateMachineBase : public common::StateMachine {
public:
    /**
     *@enum Label
     *
     *@brief Label corresponding to each LINKMGR Health State
     */
    enum class Label {
        Uninitialized,
        Unhealthy,
        Healthy,

        Count
    };

    using CompositeState = std::tuple<link_prober::LinkProberState::Label,
                                      mux_state::MuxState::Label,
                                      link_state::LinkState::Label>;
    using TransitionFunction = std::function<void(CompositeState&)>;

public:
    /**
     *@method LinkManagerStateMachineBase
     *
     *@brief class default constructor
     */
    LinkManagerStateMachineBase() = delete;

    /**
     * @method LinkManagerStateMachineBase
     *
     * @brief class copy constructor
     *
     * @param LinkManagerStateMachineBase   reference to LinkManagerStateMachineBase object to be copied
     */
    LinkManagerStateMachineBase(const LinkManagerStateMachineBase&) = delete;

    /**
     * @method LinkManagerStateMachineBase
     *
     * @brief Construct a new LinkManagerStateMachineBase object
     *
     * @param strand                        boost serialization object
     * @param muxPortConfig                 reference to MuxPortConfig object
     * @param initialCompositeState         initial Composite states
     */
    LinkManagerStateMachineBase(
        mux::MuxPort *muxPortPtr,
        boost::asio::io_service::strand& strand,
        common::MuxPortConfig& muxPortConfig,
        CompositeState initialCompositeState
    );

    /**
     * @method ~LinkManagerStateMachineBase
     *
     * @brief Destroy the Link Manager State Machine Base object
     */
    virtual ~LinkManagerStateMachineBase() = default;

    /**
     * @method initializeTransitionFunctionTable
     *
     * @brief Initialize the transition function table
     */
    virtual void initializeTransitionFunctionTable() = 0;

    /**
     * @method handleStateChange
     *
     * @brief handle LinkProberEvent
     *
     * @param event                         reference to LinkProberEvent object
     * @param state                         new LinkProberState label
     */
    virtual void handleStateChange(LinkProberEvent& event, link_prober::LinkProberState::Label state) = 0;

    /**
     * @method handleStateChange
     *
     * @brief handle MuxStateEvent
     *
     * @param event                         reference to MuxStateEvent object
     * @param state                         new MuxStateEvent label
     */
    virtual void handleStateChange(MuxStateEvent& event, mux_state::MuxState::Label state) = 0;

    /**
     * @method handleStateChange
     *
     * @brief handle LinkStateEvent
     *
     * @param event                         reference to LinkStateEvent object
     * @param state                         new LinkStateEvent label
     */
    virtual void handleStateChange(LinkStateEvent& event, link_state::LinkState::Label state) = 0;

    /**
     * @method handlePeerStateChange
     *
     * @brief handle peer LinkProberEvent
     *
     * @param event                         reference to a peer LinkProberEvent object
     * @param state                         new LinkProberState label
     */
    virtual void handlePeerStateChange(LinkProberEvent& event, link_prober::LinkProberState::Label state);

    /**
     * @method getLinkProberEvent
     *
     * @brief Get the Link Prober Event object
     *
     * @return LinkProberEvent&
     */
    static LinkProberEvent& getLinkProberEvent() { return mLinkProberEvent; };

    /**
     * @method getMuxStateEvent
     *
     * @brief Get the Mux State Event object
     *
     * @return MuxStateEvent&
     */
    static MuxStateEvent& getMuxStateEvent() { return mMuxStateEvent; };

    /**
     * @method getLinkStateEvent
     *
     * @brief Get the Link State Event object
     *
     * @return LinkStateEvent&
     */
    static LinkStateEvent& getLinkStateEvent() { return mLinkStateEvent; };

    /**
     * @method getCompositeState
     *
     * @brief Get the Composite State object
     *
     * @return const CompositeState&
     */
    const CompositeState& getCompositeState() { return mCompositeState; };

public:
    /**
     *@method handleSwssBladeIpv4AddressUpdate
     *
     *@brief initialize LinkProber component. Note if this is the last component to be initialized,
     *       state machine will be activated
     *
     *@return none
     */
    virtual void handleSwssBladeIpv4AddressUpdate(boost::asio::ip::address address);

    /**
     *@method handleGetServerMacNotification
     *
     *@brief handle get Server MAC address
     *
     *@param address (in)    Server MAC address
     *
     *@return none
     */
    virtual void handleGetServerMacAddressNotification(std::array<uint8_t, ETHER_ADDR_LEN> address);

    /**
     *@method handleGetMuxStateNotification
     *
     *@brief handle get MUX state notification
     *
     *@param label (in)              new MuxState label
     *
     *@return none
     */
    virtual void handleGetMuxStateNotification(mux_state::MuxState::Label label);

    /**
     *@method handleProbeMuxStateNotification
     *
     *@brief handle probe MUX state notification
     *
     *@param label (in)              new MuxState label
     *
     *@return none
     */
    virtual void handleProbeMuxStateNotification(mux_state::MuxState::Label label);

    /**
     *@method handleMuxStateNotification
     *
     *@brief handle MUX state notification
     *
     *@param label (in)              new MuxState label
     *
     *@return none
     */
    virtual void handleMuxStateNotification(mux_state::MuxState::Label label);

    /**
     *@method handleSwssLinkStateNotification
     *
     *@brief handle link state change notification
     *
     *@param label (in)  new LinkState label
     *
     *@return none
     */
    virtual void handleSwssLinkStateNotification(const link_state::LinkState::Label label);

    /**
     * @method handlePeerLinkStateNotification
     *
     * @brief handle peer link state notification
     *
     * @param label (in) new peer link state label
     *
     * @return none
     */
    virtual void handlePeerLinkStateNotification(const link_state::LinkState::Label label);

    /**
     *@method handleMuxConfigNotification
     *
     *@brief handle MUX configuration change notification
     *
     *@param mode (in)  new MUX config mode
     *
     *@return none
     */
    virtual void handleMuxConfigNotification(const common::MuxPortConfig::Mode mode);

    /**
     *@method handleSuspendTimerExpiry
     *
     *@brief handle suspend timer expiry notification from LinkProber
     *
     *@return none
     */
    virtual void handleSuspendTimerExpiry();

    /**
     *@method handleSwitchActiveCommandCompletion
     *
     *@brief handle completion of sending switch command to peer ToR
     *
     *@return none
     */
    virtual void handleSwitchActiveCommandCompletion();

    /**
     *@method handleSwitchActiveRequestEvent
     *
     *@brief handle switch active request from peer ToR
     *
     *@return none
     */
    virtual void handleSwitchActiveRequestEvent();

    /**
     * @method handleDefaultRouteStateNotification(const std::string &routeState)
     *
     * @brief handle default route state notification from routeorch
     *
     * @param routeState
     *
     * @return none
     */
    virtual void handleDefaultRouteStateNotification(const std::string& routeState);

    /**
     * @method handlePostPckLossRatioNotification
     *
     * @brief handle get post pck loss ratio
     *
     * @param unknownEventCount (in) count of missing icmp packets
     * @param expectedPacketCount (in) count of expected icmp packets
     *
     * @return none
     */
    virtual void handlePostPckLossRatioNotification(const uint64_t unknownEventCount, const uint64_t expectedPacketCount);

    /**
     * @method handleResetLinkProberPckLossCount
     *
     * @brief reset link prober heartbeat packet loss count
     *
     * @return none
     */
    virtual void handleResetLinkProberPckLossCount();

    /**
     *@method setComponentInitState
     *
     *@brief set component inti state. This method is used for testing
     *
     *@param component (in)  component index
     *
     *@return none
     */
    virtual void setComponentInitState(uint8_t component);

public:
    /**
    *@method getLinkProberStateMachinePtr
    *
    *@brief getter for LinkProberStateMachineBase pointer
    *
    *@return reference to LinkProberStateMachine pointer
    */
    std::shared_ptr<link_prober::LinkProberStateMachineBase> getLinkProberStateMachinePtr() {return mLinkProberStateMachinePtr;};

    /**
    *@method getMuxStateMachine
    *
    *@brief getter for MuxStateMachine object
    *
    *@return reference to MuxStateMachine object
    */
    mux_state::MuxStateMachine& getMuxStateMachine() {return mMuxStateMachine;};

    /**
    *@method getLinkStateMachine
    *
    *@brief getter for LinkStateMachine object
    *
    *@return reference to LinkStateMachine object
    */
    link_state::LinkStateMachine& getLinkStateMachine() {return mLinkStateMachine;};

private:
    friend class ActiveStandbyStateMachine;

private:
    /**
     * @method setLabel
     *
     * @brief Set the Linkmgr STATE_DB state
     *
     * @param label
     */
    virtual inline void setLabel(Label label) = 0;

    /**
     * @method noopTransitionFunction
     *
     * @brief NO-OP transition function
     *
     * @param nextState                     reference to CompositeState object
     */
    void noopTransitionFunction(CompositeState& nextState);

private:
    static LinkProberEvent mLinkProberEvent;
    static MuxStateEvent mMuxStateEvent;
    static LinkStateEvent mLinkStateEvent;

    // To print human readable state name
    static std::vector<std::string> mLinkProberStateName;
    static std::vector<std::string> mMuxStateName;
    static std::vector<std::string> mLinkStateName;
    static std::vector<std::string> mLinkHealthName;

private:
    TransitionFunction mStateTransitionHandler[link_prober::LinkProberState::Label::Count]
                                              [mux_state::MuxState::Label::Count]
                                              [link_state::LinkState::Label::Count];
    LinkManagerStateMachineBase::CompositeState mCompositeState;

    mux::MuxPort *mMuxPortPtr;
    std::shared_ptr<link_prober::LinkProberStateMachineBase> mLinkProberStateMachinePtr;
    std::shared_ptr<link_prober::LinkProber> mLinkProberPtr = nullptr;
    mux_state::MuxStateMachine mMuxStateMachine;
    link_state::LinkStateMachine mLinkStateMachine;

    Label mLabel = Label::Uninitialized;
};

} /* namespace link_manager */

#endif /* LINK_MANAGER_LINKMANAGERSTATEMACHINEBASE_H_ */
