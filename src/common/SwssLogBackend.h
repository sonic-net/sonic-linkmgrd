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

#ifndef SWSSLOGBACKEND_H_
#define SWSSLOGBACKEND_H_

#include <boost/log/sinks/basic_sink_backend.hpp>
#include <boost/log/sinks/syslog_backend.hpp>
#include <boost/log/trivial.hpp>

#include "swss/logger.h"

namespace common
{

class SwssSyslogBackend : public boost::log::sinks::basic_formatted_sink_backend<char>
{
public:
    typedef boost::log::sinks::syslog_backend::char_type char_type;
    typedef boost::log::sinks::syslog_backend::string_type string_type;
    typedef boost::log::sinks::syslog_backend::severity_mapper_type severity_mapper_type;
    typedef boost::log::sinks::syslog::level boost_syslog_level;
    typedef boost::log::trivial::severity_level boost_log_level;

public:
    /**
     *@method ~SwssSyslogBackend
     *
     *@brief class constructor
     *
     */
    SwssSyslogBackend();

    /**
     *@method ~SwssSyslogBackend
     *
     *@brief class destructor
     *
     */
    ~SwssSyslogBackend() = default;

    /**
     *@method set_severity_mapper
     *
     *@brief set the mapping from boost log severity to syslog level
     *
     *@param mapper (in)                severity mapping
     *
     *@return none
     */
    void set_severity_mapper(const severity_mapper_type& mapper);

    /**
     *@method consume
     *
     *@brief write the message to the sink
     *
     *@param record (in)                boost log record entry
     *
     *@param formatted_messgaes (in)    formatted log string of the record
     *
     *@return none
     */
    void consume(const boost::log::record_view& record, const string_type& formatted_message);

private:
    /**
     *@method send
     *
     *@brief write the message swss logger
     *
     *@param level (in)                 syslog level
     *
     *@param formatted_messgaes (in)    formatted log string of the record
     *
     *@return none
     */
    inline void send(boost_syslog_level level, const string_type& formatted_message);

    severity_mapper_type mLevelMapper;
    swss::Logger& mSwssLogger;
};

} /* namespace common */

#endif /* SWSSLOGBACKEND_H_ */
