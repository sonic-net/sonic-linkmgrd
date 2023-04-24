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
 * AsyncEvent.h
 *
 *  Created on: April 24, 2023
 *      Author: Longxiang Lyu
 */

#ifndef ASYNCEVENT_H
#define ASYNCEVENT_H

#include <common/BoostAsioBehavior.h>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <boost/function.hpp>

namespace common
{

/**
 * @class AsyncEvent
 *
 * @brief an asynchronous event implemented with boost::asio::deadline_timer
 */
class AsyncEvent
{
public:
    /**
     *@method AsyncEvent
     *
     *@brief class default constructor
     */
    AsyncEvent() = delete;

    /**
     *@method AsyncEvent
     *
     *@brief Construct a new AsyncEvent object
     *
     *@param strand                 a boost strand object
     */
    AsyncEvent(boost::asio::io_service::strand& strand)
        : mStrand(strand), mWaitEventTimer(strand.context(), boost::posix_time::ptime(boost::posix_time::pos_infin))
    {}

    /**
     *@method AsyncEvent
     *
     *@brief class copy constructor
     */
    AsyncEvent(const AsyncEvent&) = delete;

    /**
     *@method ~AsyncEvent
     *
     *@brief Destroy the Async Event object
     */
    virtual ~AsyncEvent() = default;

    /**
     *@method registerWaitHandler
     *
     *@brief register a wait handlers to be called when event is notified
     *
     *@param handler                a wait handler for the event
     */
    void registerWaitHandler(boost::function<void()>&& handler)
    {
        mWaitEventTimer.async_wait(
            boost::asio::bind_executor(
                mStrand,
                [handler = std::move(handler)](const boost::system::error_code& errorCode) {
                    if (errorCode == boost::asio::error::operation_aborted) {
                        handler();
                    }
                }));
    }

    /**
     *@method notify
     *
     *@brief notify one registered wait handlers to execute
     */
    void notify() { mWaitEventTimer.cancel_one(); }

    /**
     *@method notify_all
     *
     *@brief notify all registered wait handlers to execute
     */
    void notify_all() { mWaitEventTimer.cancel(); }

private:
    boost::asio::io_service::strand& mStrand;
    boost::asio::deadline_timer mWaitEventTimer;
};

} /* namespace common */

#endif /* ASYNCEVENT_H */
