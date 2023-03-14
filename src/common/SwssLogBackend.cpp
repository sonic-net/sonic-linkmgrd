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

#include "SwssLogBackend.h"

namespace common
{

//
// ---> SwssSyslogBackend();
//
// class Constructor
//
SwssSyslogBackend::SwssSyslogBackend()
    : mSwssLogger(swss::Logger::getInstance())
{
}

//
// ---> set_severity_mapper(const severity_mapper_type& mapper)
//
// set the mapping from boost log severity to syslog level
//
void SwssSyslogBackend::set_severity_mapper(const severity_mapper_type& mapper)
{
    mLevelMapper = mapper;
}

//
// ---> consume(const boost::log::record_view& record,
//              const string_type& formatted_message);
//
// write the message to the sink
//
void SwssSyslogBackend::consume(const boost::log::record_view& record, const string_type& formatted_message)
{
    boost_syslog_level level = mLevelMapper(record);
    send(level, formatted_message);
}

//
// ---> send(boost_syslog_level level,
//           const string_type& formatted_message);
//
// write the message swss logger
//
void SwssSyslogBackend::send(boost_syslog_level level, const string_type& formatted_message)
{
    switch (level) {
        case boost_syslog_level::debug:
            mSwssLogger.write(swss::Logger::SWSS_DEBUG, formatted_message.c_str());
            break;
        case boost_syslog_level::info:
            mSwssLogger.write(swss::Logger::SWSS_INFO, formatted_message.c_str());
            break;
        case boost_syslog_level::notice:
            mSwssLogger.write(swss::Logger::SWSS_NOTICE, formatted_message.c_str());
            break;
        case boost_syslog_level::warning:
            mSwssLogger.write(swss::Logger::SWSS_WARN, formatted_message.c_str());
            break;
        case boost_syslog_level::error:
            mSwssLogger.write(swss::Logger::SWSS_ERROR, formatted_message.c_str());
            break;
        case boost_syslog_level::critical:
            mSwssLogger.write(swss::Logger::SWSS_CRIT, formatted_message.c_str());
            break;
        case boost_syslog_level::alert:
            mSwssLogger.write(swss::Logger::SWSS_ALERT, formatted_message.c_str());
            break;
        case boost_syslog_level::emergency:
            mSwssLogger.write(swss::Logger::SWSS_EMERG, formatted_message.c_str());
            break;
        default:
            break;
    }
}

} /* namespace common */
