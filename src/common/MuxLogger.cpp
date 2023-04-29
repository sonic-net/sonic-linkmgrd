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
 * MuxLogger.cpp
 *
 *  Created on: Oct 4, 2020
 *      Author: Tamer Ahmed
 */

#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include "boost/log/utility/setup/from_settings.hpp"
#include <boost/log/utility/exception_handler.hpp>
#include <boost/filesystem.hpp>

#include "MuxException.h"
#include "MuxLogger.h"
#include "SwssLogBackend.h"

namespace common
{

//
// ---> operator()(const boost::log::runtime_error &ex);
//
// handles runtime error exceptions
//
void MuxLoggerExceptionHandler::operator()(const boost::log::runtime_error &ex) const
{
    std::ostringstream errMsg;
    errMsg << "MUX Logger exception!!" << ". Exception details: " << ex.what();

    throw MUX_ERROR(MuxLogger, errMsg.str());
}

//
// ---> operator()(const std::exception &ex);
//
// handles std exceptions
//
void MuxLoggerExceptionHandler::operator()(const std::exception &ex) const
{
    std::ostringstream errMsg;
    errMsg << "MUX Logger exception!!" << ". Exception details: " << ex.what();

    throw MUX_ERROR(MuxLogger, errMsg.str());
}

//
// ---> getInstance();
//
// constructs MuxLogger singleton instance
//
MuxLoggerPtr MuxLogger::getInstance()
{
    static std::shared_ptr<MuxLogger> MuxLoggerPtr = nullptr;

    if (MuxLoggerPtr == nullptr) {
        MuxLoggerPtr = std::shared_ptr<MuxLogger> (new MuxLogger);
    }

    return MuxLoggerPtr;
}

const MuxLogger::BoostLogLevelMap MuxLogger::mBoostLogLevelMapper = {
    { boost::log::sinks::syslog::emergency, boost::log::trivial::fatal },
    { boost::log::sinks::syslog::alert, boost::log::trivial::fatal },
    { boost::log::sinks::syslog::critical, boost::log::trivial::error },
    { boost::log::sinks::syslog::error, boost::log::trivial::error },
    { boost::log::sinks::syslog::warning, boost::log::trivial::warning },
    { boost::log::sinks::syslog::notice, boost::log::trivial::warning },
    { boost::log::sinks::syslog::info, boost::log::trivial::info },
    { boost::log::sinks::syslog::debug, boost::log::trivial::trace }
};

const MuxLogger::SyslogLevelMap MuxLogger::mSyslogLevelMapper = {
    { boost::log::trivial::fatal, boost::log::sinks::syslog::alert },
    { boost::log::trivial::error, boost::log::sinks::syslog::error },
    { boost::log::trivial::warning, boost::log::sinks::syslog::notice },
    { boost::log::trivial::info, boost::log::sinks::syslog::info },
    { boost::log::trivial::debug, boost::log::sinks::syslog::debug },
    { boost::log::trivial::trace, boost::log::sinks::syslog::debug }
};

//
// ---> initialize(std::string &prog,
//                 std::string &path,
//                 boost::log::trivial::severity_level level);
//
// initialize MUX logging class
//
void MuxLogger::initialize(
    std::string &prog,
    std::string &path,
    boost::log::trivial::severity_level level,
    bool extraLogFile,
    bool linkToSwssLogger
)
{
    namespace trivial = boost::log::trivial;

    mLevel = level;

    boost::log::register_simple_formatter_factory<trivial::severity_level, char> ("Severity");

    boost::log::settings settings;
    boost::log::init_from_settings(settings);

    boost::log::add_common_attributes();
    boost::log::core::get()->set_exception_handler(
        boost::log::make_exception_handler<boost::log::runtime_error, std::exception> (MuxLoggerExceptionHandler())
    );

    mLinkToSwssLogger = linkToSwssLogger;

    if (linkToSwssLogger) {
        addSwssSyslogSink(prog);
        // default to "NOTICE" when linking to swss log level
        startSwssLogger("NOTICE");
        mLevel = boost::log::trivial::warning;
    } else {
        addSyslogSink(prog);
    }

    if (extraLogFile) {
        addExtraLogFileSink(prog, path);
    }
}

//
// ---> setLevel(const boost::log::trivial::severity_level level);
//
// set logger frontend and backend filter level
//
void MuxLogger::setLevel(const boost::log::trivial::severity_level level)
{
    namespace trivial = boost::log::trivial;

    mLevel = level;
    boost::log::core::get()->set_filter(trivial::severity >= level);
}

//
// ---> addExtraLogFileSink(std::string &prog, const std::string &logFile);
//
// Add an extra log file sink
//
void MuxLogger::addExtraLogFileSink(std::string &prog, const std::string &logFile)
{
    namespace keywords = boost::log::keywords;

    try {
        boost::filesystem::remove(logFile);
        boost::log::add_file_log(
            keywords::file_name = logFile,
            keywords::format = "[%TimeStamp%] [%Severity%] %Message%"
        );
    }
    catch (std::exception& ex) {
        std::ostringstream errMsg;
        errMsg << "MUX Logger exception!!" << ". Exception details: " << ex.what();

        throw MUX_ERROR(MuxLogger, errMsg.str());
   }
}

//
// ---> addSyslogSink(std::string &prog);
//
// Add syslog sink
//
void MuxLogger::addSyslogSink(std::string &prog)
{
    namespace sinks = boost::log::sinks;
    try {
        // Create a syslog sink
        boost::shared_ptr<sinks::syslog_backend> sink(new sinks::syslog_backend(
            boost::log::keywords::facility = sinks::syslog::user,
            boost::log::keywords::use_impl = sinks::syslog::native
        ));

        // Create and fill in another level translator for "Severity" attribute of type string
        sinks::syslog::custom_severity_mapping<boost::log::trivial::severity_level> mapping("Severity");
        for (const auto &p : mSyslogLevelMapper) {
            mapping[p.first] = p.second;
        }
        sink->set_severity_mapper(mapping);

        // Add the sink to the core
        boost::log::core::get()->add_sink(boost::make_shared<sinks::synchronous_sink<sinks::syslog_backend>> (sink));
    }
    catch (std::exception& ex) {
        std::ostringstream errMsg;
        errMsg << "MUX Logger exception!!" << ". Exception details: " << ex.what();

        throw MUX_ERROR(MuxLogger, errMsg.str());
   }
}

//
// ---> addSwssSyslogSink(std::string &prog);
//
// Add swss syslog sink
//
void MuxLogger::addSwssSyslogSink(std::string &prog)
{
    namespace sinks = boost::log::sinks;
    try {
        boost::shared_ptr<SwssSyslogBackend> sink(new SwssSyslogBackend());

        // Create and fill in another level translator for "Severity" attribute of type string
        sinks::syslog::custom_severity_mapping<boost::log::trivial::severity_level> mapping("Severity");
        for (const auto &p : mSyslogLevelMapper) {
            mapping[p.first] = p.second;
        }
        sink->set_severity_mapper(mapping);

        boost::log::core::get()->add_sink(boost::make_shared<sinks::synchronous_sink<SwssSyslogBackend>> (sink));
    }
    catch (std::exception& ex) {
        std::ostringstream errMsg;
        errMsg << "MUX Logger exception!!" << ". Exception details: " << ex.what();

        throw MUX_ERROR(MuxLogger, errMsg.str());
   }
}

//
// ---> startSwssLogger(const std::string &swssPrio);
//
// start swss logger
//
void MuxLogger::startSwssLogger(const std::string &swssPrio)
{
    swss::Logger::linkToDbWithOutput(
        "linkmgrd",
        [=](std::string component, std::string prioStr) { this->swssPrioNotify(component, prioStr); },
        swssPrio,
        [=](std::string component, std::string outputStr) { this->swssOutputNotify(component, outputStr); },
        "SYSLOG"
    );
    swss::Logger::restartLogger();
}

//
// ---> swssPrioNotify(const std::string& component, const std::string& prioStr);
//
// process syslog priority setting from swssloglevel
//
void MuxLogger::swssPrioNotify(std::string component, std::string prioStr)
{
    namespace sinks = boost::log::sinks;

    if (swss::Logger::priorityStringMap.find(prioStr) == swss::Logger::priorityStringMap.end()) {
        MUXLOGFATAL(boost::format("Invalid loglevel %s, ignored.") % prioStr);
    } else {
        swss::Logger::Priority swssLogLevel = swss::Logger::priorityStringMap.at(prioStr);
        swss::Logger::getInstance().setMinPrio(swssLogLevel);
        MUXLOGFATAL(boost::format("Updated linkmgrd swss log level to: %s") % prioStr);

        sinks::syslog::level syslogLevel = static_cast<sinks::syslog::level>(swssLogLevel);
        if (mBoostLogLevelMapper.find(syslogLevel) != mBoostLogLevelMapper.end()) {
            boost::log::trivial::severity_level boostLogLevel = mBoostLogLevelMapper.at(syslogLevel);
            setLevel(boostLogLevel);
            MUXLOGFATAL(boost::format("Updated mux log level to: %s") % boostLogLevel);
        }
    }
}

//
// ---> swssOutputNotify(const std::string& component, const std::string& outputStr);
//
// process syslog output setting from swssloglevel, only support syslog
//
void MuxLogger::swssOutputNotify(std::string component, std::string outputStr)
{
    if (outputStr != "SYSLOG") {
        MUXLOGFATAL(boost::format("Invalid logoutput %s, ignored.") % outputStr);
    }
}


} /* namespace common */
