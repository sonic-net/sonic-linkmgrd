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

#include "gtest/gtest.h"

#include "common/MuxLogger.h"
#include "MuxLoggerTest.h"

namespace test
{

void MuxLoggerTest::initializeLogger(
    std::string &prog, std::string &path, boost::log::trivial::severity_level level, bool extraLogFile, bool linkToSwsslogger
)
{
    initialize(
        prog,
        path,
        level,
        extraLogFile,
        linkToSwsslogger
    );
}

void MuxLoggerTest::startSwssLogger(const std::string &swssPrio)
{
    mSwssLoggerStarted = true;
    mInitSwssPrioStr = swssPrio;
}

TEST_F(MuxLoggerTest, LinkToSwssLogger)
{
    std::string prog = "linkmgrd";
    std::string path = "";
    boost::log::trivial::severity_level muxLoggerLevel;
    swss::Logger::Priority swssLoggerLevel;

    initializeLogger(prog, path, boost::log::trivial::fatal, false, true);

    EXPECT_TRUE(isSwssLoggerStarted());
    EXPECT_EQ(getInitialSwssLogPrioStr(), "NOTICE");

    swssPrioNotify(prog, getInitialSwssLogPrioStr());
    swssLoggerLevel = swss::Logger::getMinPrio();
    muxLoggerLevel = getLevel();
    EXPECT_EQ(swssLoggerLevel, swss::Logger::SWSS_NOTICE);
    EXPECT_EQ(muxLoggerLevel, boost::log::trivial::warning);

    swssPrioNotify(prog, "EMERG");
    swssLoggerLevel = swss::Logger::getMinPrio();
    muxLoggerLevel = getLevel();
    EXPECT_EQ(swssLoggerLevel, swss::Logger::SWSS_EMERG);
    EXPECT_EQ(muxLoggerLevel, boost::log::trivial::fatal);

    swssPrioNotify(prog, "ALERT");
    swssLoggerLevel = swss::Logger::getMinPrio();
    muxLoggerLevel = getLevel();
    EXPECT_EQ(swssLoggerLevel, swss::Logger::SWSS_ALERT);
    EXPECT_EQ(muxLoggerLevel, boost::log::trivial::fatal);

    swssPrioNotify(prog, "CRIT");
    swssLoggerLevel = swss::Logger::getMinPrio();
    muxLoggerLevel = getLevel();
    EXPECT_EQ(swssLoggerLevel, swss::Logger::SWSS_CRIT);
    EXPECT_EQ(muxLoggerLevel, boost::log::trivial::error);

    swssPrioNotify(prog, "ERROR");
    swssLoggerLevel = swss::Logger::getMinPrio();
    muxLoggerLevel = getLevel();
    EXPECT_EQ(swssLoggerLevel, swss::Logger::SWSS_ERROR);
    EXPECT_EQ(muxLoggerLevel, boost::log::trivial::error);

    swssPrioNotify(prog, "WARN");
    swssLoggerLevel = swss::Logger::getMinPrio();
    muxLoggerLevel = getLevel();
    EXPECT_EQ(swssLoggerLevel, swss::Logger::SWSS_WARN);
    EXPECT_EQ(muxLoggerLevel, boost::log::trivial::warning);

    swssPrioNotify(prog, "NOTICE");
    swssLoggerLevel = swss::Logger::getMinPrio();
    muxLoggerLevel = getLevel();
    EXPECT_EQ(swssLoggerLevel, swss::Logger::SWSS_NOTICE);
    EXPECT_EQ(muxLoggerLevel, boost::log::trivial::warning);

    swssPrioNotify(prog, "INFO");
    swssLoggerLevel = swss::Logger::getMinPrio();
    muxLoggerLevel = getLevel();
    EXPECT_EQ(swssLoggerLevel, swss::Logger::SWSS_INFO);
    EXPECT_EQ(muxLoggerLevel, boost::log::trivial::info);

    swssPrioNotify(prog, "DEBUG");
    swssLoggerLevel = swss::Logger::getMinPrio();
    muxLoggerLevel = getLevel();
    EXPECT_EQ(swssLoggerLevel, swss::Logger::SWSS_DEBUG);
    EXPECT_EQ(muxLoggerLevel, boost::log::trivial::trace);
}

} /* namespace test */
