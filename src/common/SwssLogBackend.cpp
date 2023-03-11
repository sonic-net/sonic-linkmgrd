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
    namespace sinks = boost::log::sinks;
    sinks::syslog::custom_severity_mapping<boost_log_level> mapping("Severity");
    mapping[boost::log::trivial::trace] = boost_syslog_level::debug;
    mapping[boost::log::trivial::debug] = boost_syslog_level::debug;
    mapping[boost::log::trivial::info] = boost_syslog_level::info;
    mapping[boost::log::trivial::warning] = boost_syslog_level::notice;
    mapping[boost::log::trivial::error] = boost_syslog_level::error;
    mapping[boost::log::trivial::fatal] = boost_syslog_level::alert;

    set_severity_mapper(mapping);
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
            swss::Logger::getInstance().write(swss::Logger::SWSS_DEBUG, formatted_message.c_str());
            break;
        case boost_syslog_level::info:
            swss::Logger::getInstance().write(swss::Logger::SWSS_INFO, formatted_message.c_str());
            break;
        case boost_syslog_level::notice:
            swss::Logger::getInstance().write(swss::Logger::SWSS_NOTICE, formatted_message.c_str());
            break;
        case boost_syslog_level::warning:
            swss::Logger::getInstance().write(swss::Logger::SWSS_WARN, formatted_message.c_str());
            break;
        case boost_syslog_level::error:
            swss::Logger::getInstance().write(swss::Logger::SWSS_ERROR, formatted_message.c_str());
            break;
        case boost_syslog_level::critical:
            swss::Logger::getInstance().write(swss::Logger::SWSS_CRIT, formatted_message.c_str());
            break;
        case boost_syslog_level::alert:
            swss::Logger::getInstance().write(swss::Logger::SWSS_ALERT, formatted_message.c_str());
            break;
        case boost_syslog_level::emergency:
            swss::Logger::getInstance().write(swss::Logger::SWSS_EMERG, formatted_message.c_str());
            break;
        default:
            break;
    }
}

} /* namespace common */
