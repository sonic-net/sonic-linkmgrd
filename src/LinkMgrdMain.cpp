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

#include "MuxManager.h"
#include "MuxPort.h"
#include "common/MuxConfig.h"
#include "common/MuxLogger.h"
#include "link_manager/LinkManagerStateMachine.h"
#include "link_prober/LinkProberStateMachine.h"
#include "link_prober/LinkProber.h"
#include "link_prober/IcmpPayload.h"

// Private namespace for this module
namespace {
    // Some namespace and type aliases...
    namespace program_options = boost::program_options;

    static auto DEFAULT_LOGGING_FILTER_LEVEL = boost::log::trivial::debug;

    void InitializeLogger(std::string execName, boost::log::trivial::severity_level level)
    {
        std::string progName = execName.substr(execName.find_last_of('/') + 1);
        std::string logFile = "/var/log/mux/" + progName + ".log";

        common::MuxLogger::getInstance()->initialize(progName, logFile, level);
    }

} // end namespace


//
// Main program entry point for Gemini MUX Manager (aka linkmgrd).
//
int main(int argc, const char* argv[])
{
    int retValue = EXIT_SUCCESS;
    int dummyVal = -1;
    //
    // Constants for command line argument strings:
    //
    boost::log::trivial::severity_level level;

    program_options::options_description description("linkmgrd options");
    description.add_options()
        ("help,h",
         "Print usage information.")
        ("verbosity,v",
         program_options::value<boost::log::trivial::severity_level>(&level)->value_name("<severity_level>")->
         default_value(DEFAULT_LOGGING_FILTER_LEVEL),
         "Logging verbosity level.")
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
        InitializeLogger(argv[0], level);
        std::stringstream ss;
        ss << "level: " << level;
        MUXLOGINFO(ss.str());

        // initialize static data
        link_prober::IcmpPayload::generateGuid();

        std::shared_ptr<mux::MuxManager> muxManagerPtr = std::make_shared<mux::MuxManager> ();
        muxManagerPtr->initialize();
        muxManagerPtr->run();
        muxManagerPtr->deinitialize();
    }

    return retValue;
}
