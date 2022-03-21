#include "gmock/gmock.h"
#include "gtest/gtest.h"

#include "common/MuxLogger.h"

void initLogger()
{
    std::string prog_name = "linkmgrd-test";
    std::string log_filename = "/tmp/" + prog_name + ".log";
    bool extraLogFile = true;
    common::MuxLogger::getInstance()->initialize(prog_name, log_filename, boost::log::trivial::debug, extraLogFile);
    common::MuxLogger::getInstance()->setLevel(boost::log::trivial::trace);
}

int main(int argc, char** argv)
{
    initLogger();
    // NOTE: keep this flags as comments for futher debug
    // ::testing::GTEST_FLAG(catch_exceptions) = false;
    // ::testing::GTEST_FLAG(throw_on_failure) = false;
    ::testing::InitGoogleMock(&argc, argv);
    return RUN_ALL_TESTS();
}