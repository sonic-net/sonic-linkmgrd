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
 * linkMgrdMain.cpp
 *
 *  Created on: Oct 23, 2020
 *      Author: Tamer Ahmed Ahmed
 */

#include <iostream>

#include <boost/lexical_cast.hpp>
#include <boost/program_options.hpp>

#include "swss/warm_restart.h"

#include "MuxManager.h"
#include "MuxPort.h"
#include "common/MuxConfig.h"
#include "common/MuxLogger.h"
#include "link_manager/LinkManagerStateMachineActiveStandby.h"
#include "link_prober/LinkProberStateMachineActiveActive.h"
#include "link_prober/LinkProberStateMachineActiveStandby.h"
#include "link_prober/LinkProber.h"
#include "link_prober/IcmpPayload.h"

// Private namespace for this module
namespace {
    // Some namespace and type aliases...
    namespace program_options = boost::program_options;

    static auto DEFAULT_LOGGING_FILTER_LEVEL = boost::log::trivial::debug;

    void InitializeLogger(std::string execName, boost::log::trivial::severity_level level, bool extraLogFile, bool linkToSwssLogger)
    {
        std::string progName = execName.substr(execName.find_last_of('/') + 1);
        std::string logFile = "/var/log/mux/" + progName + ".log";

        common::MuxLogger::getInstance()->initialize(progName, logFile, level, extraLogFile, linkToSwssLogger);
    }

} // end namespace


//
// Main program entry point for Gemini MUX Manager (aka linkmgrd).
//
int main(int argc, const char* argv[])
{
    int retValue = EXIT_SUCCESS;

    //
    // Constants for command line argument strings:
    //
    boost::log::trivial::severity_level level;
    bool extraLogFile = false;
    bool measureSwitchover = false;
    bool defaultRoute = false;
    bool linkToSwssLogger = false;
    bool simulateLfdOffload = false;

    program_options::options_description description("linkmgrd options");
    description.add_options()
        ("help,h",
         "Print usage information.")
        ("verbosity,v",
         program_options::value<boost::log::trivial::severity_level>(&level)->value_name("<severity_level>")->
         default_value(DEFAULT_LOGGING_FILTER_LEVEL),
         "Boost logging verbosity level.")
        ("extra_log_file,e",
         program_options::bool_switch(&extraLogFile)->default_value(false),
         "Store logs in an extra log file")
         ("measure_switchover_overhead,m",
         program_options::bool_switch(&measureSwitchover)->default_value(false),
         "Decrease link prober interval after switchover to better measure switchover overhead")
         ("default_route,d",
         program_options::bool_switch(&defaultRoute)->default_value(false),
         "Disable heartbeat sending and avoid switching to active when default route is missing"
         )
        ("link_to_swss_logger,l",
         program_options::bool_switch(&linkToSwssLogger)->default_value(false),
         "Link to swss logger instead of using native boost syslog support, this will"
         "set the boost logging level to TRACE and option verbosity is ignored"
         )
        ("simulate_lfd_offload,s",
         program_options::bool_switch(&simulateLfdOffload)->default_value(false),
         "Simulate LFD offload by posting link prober state change notification to Redis"
         )
    ;

    //
    // Actually parse options, print verbose usage when it fails
    //
    program_options::variables_map variableMap;
    try {
        store(parse_command_line(argc, argv, description), variableMap);
        program_options::notify(variableMap);
    }
    catch (program_options::error_with_option_name& e) {
        std::cerr << "Command Line Syntax Error: " << e.what() << std::endl;
        std::cout << description << "\n";
        retValue = EXIT_FAILURE;
    }
    catch (program_options::error& e) {
        std::cerr << "Command Line Error: " << e.what() << std::endl;
        std::cout << description << "\n";
        retValue = EXIT_FAILURE;
    }

    if (retValue == EXIT_SUCCESS && variableMap.count("help")) {
        std::cout << description << "\n";

        retValue = EXIT_FAILURE;
    }

    if (retValue == EXIT_SUCCESS) {
        InitializeLogger(argv[0], level, extraLogFile, linkToSwssLogger);
        std::stringstream ss;
        ss << "level: " << level;
        MUXLOGINFO(ss.str());

        // initialize static data
        link_prober::IcmpPayload::generateGuid();

        // warm restart static
        swss::WarmStart::initialize("linkmgrd", "mux");
        swss::WarmStart::checkWarmStart("linkmgrd", "mux");
        if (swss::WarmStart::isWarmStart()) {
            swss::WarmStart::setWarmStartState("linkmgrd", swss::WarmStart::INITIALIZED);
        }

        std::shared_ptr<mux::MuxManager> muxManagerPtr = std::make_shared<mux::MuxManager> ();
        muxManagerPtr->initialize(measureSwitchover, defaultRoute, simulateLfdOffload);
        muxManagerPtr->run();
        muxManagerPtr->deinitialize();
    }

    return retValue;
}
