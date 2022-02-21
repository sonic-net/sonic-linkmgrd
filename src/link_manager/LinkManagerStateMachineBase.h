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
};  // namespace mux

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
        boost::asio::io_service::strand& strand,
        common::MuxPortConfig& muxPortConfig,
        CompositeState initialCompositeState);

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

    Label mLabel = Label::Uninitialized;
};

} /* namespace link_manager */

#endif /* LINK_MANAGER_LINKMANAGERSTATEMACHINEBASE_H_ */
