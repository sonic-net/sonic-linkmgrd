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

#include <boost/log/sinks/syslog_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include "boost/log/utility/setup/from_settings.hpp"
#include <boost/log/utility/exception_handler.hpp>
#include <boost/filesystem.hpp>
#include <boost/bind/bind.hpp>

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

    boost::log::register_simple_formatter_factory<trivial::severity_level, char> ("Severity");

    boost::log::settings settings;
    boost::log::init_from_settings(settings);

    boost::log::add_common_attributes();
    boost::log::core::get()->set_exception_handler(
        boost::log::make_exception_handler<boost::log::runtime_error, std::exception> (MuxLoggerExceptionHandler())
    );

    mLinkToSwssLogger = linkToSwssLogger;

    if (linkToSwssLogger) {
        // Setting the mux logger to the most verbose level to
        // send all logs received by boost logger to swss logger
        mLevel = boost::log::trivial::severity_level::trace;
        mMuxLoggerEarlyStageFilterPtr = boost::bind(
            &MuxLogger::isLogAcceptedBySwssLogger,
            this,
            boost::placeholders::_1
        );
        swss::Logger::linkToDbNative("linkmgrd");
        addSwssSyslogSink(prog);
    } else {
        mLevel = level;
        mMuxLoggerEarlyStageFilterPtr = boost::bind(
            &MuxLogger::isLogAcceptedByBoostLogger,
            this,
            boost::placeholders::_1
        );
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
    if (mLinkToSwssLogger) {
        MUXLOGERROR("Setting boost logger level is not supported when link to swss logger.");
        return;
    }
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
        mapping[boost::log::trivial::trace] = sinks::syslog::debug;
        mapping[boost::log::trivial::debug] = sinks::syslog::debug;
        mapping[boost::log::trivial::info] = sinks::syslog::info;
        mapping[boost::log::trivial::warning] = sinks::syslog::notice;
        mapping[boost::log::trivial::error] = sinks::syslog::error;
        mapping[boost::log::trivial::fatal] = sinks::syslog::alert;
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
        boost::log::core::get()->add_sink(boost::make_shared<sinks::synchronous_sink<SwssSyslogBackend>> (sink));
    }
    catch (std::exception& ex) {
        std::ostringstream errMsg;
        errMsg << "MUX Logger exception!!" << ". Exception details: " << ex.what();

        throw MUX_ERROR(MuxLogger, errMsg.str());
   }
}

//
// ---> isLogAcceptedByBoostLogger(boost::log::trivial::severity_level level);
//
// early stage log level check in log macros to see if log is accepted by native boost logger
//
bool MuxLogger::isLogAcceptedByBoostLogger(boost::log::trivial::severity_level level)
{
    return level >= getLevel();
}

//
// ---> isLogAcceptedBySwssLogger(boost::log::trivial::severity_level level);
//
// Early stage log level check in log macros to see if log is accepted by swss logger
//
bool MuxLogger::isLogAcceptedBySwssLogger(boost::log::trivial::severity_level level)
{
    return level >= mBoostLogLevelMapper[swss::Logger::getMinPrio()];
}

} /* namespace common */
