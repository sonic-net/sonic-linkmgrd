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
 * MuxException.h
 *
 *  Created on: Oct 4, 2020
 *      Author: Tamer Ahmed
 */

#ifndef MUXEXCEPTION_H_
#define MUXEXCEPTION_H_

#include <exception>
#include <string>

#include "MuxLogger.h"

/**
 * A macro for generating an MuxException with file path and line number.
 */
#define MUX_ERROR(name, msg)    common::name##Exception(msg, __FILE__, __LINE__)

namespace common
{

class MuxException : public std::exception {
public:
    MuxException(
        std::string excStr,
        const char *excFile,
        unsigned int excLine
    ) noexcept {
        mExcStr = std::string(excFile) + ":" + std::to_string(excLine) + ": " + excStr;
        MUXLOGERROR(mExcStr);
    }

    virtual const char* what() const throw() {
        return mExcStr.c_str();
    }

    virtual ~MuxException () noexcept {};

private:
    std::string mExcStr;
};

/**
* Exception for Runtime Expression
*
* This is thrown when a runtime exception is detected
*/
class RunTimeErrorException : public MuxException {
public:
    using MuxException::MuxException;

    virtual ~RunTimeErrorException () noexcept {};
};

/**
* Exception for missing configuration.
*
* This is thrown when required configuration is missing.
*/
class ConfigNotFoundException : public RunTimeErrorException {
public:
    using RunTimeErrorException::RunTimeErrorException;

    virtual ~ConfigNotFoundException () noexcept {};
};

/**
* Exception for MUX Logger
*
* This is thrown when boost logger report exceptions
*/
class MuxLoggerException : public RunTimeErrorException {
public:
    using RunTimeErrorException::RunTimeErrorException;

    virtual ~MuxLoggerException () noexcept {};
};

/**
* Exception for Bad Memory Alloc
*
* This is thrown when failing to allocated memory
*/
class BadAllocException : public RunTimeErrorException {
public:
    using RunTimeErrorException::RunTimeErrorException;

    virtual ~BadAllocException () noexcept {};
};

/**
* Exception for Socket Errors
*
* This is thrown when socket operation fails
*/
class SocketErrorException : public RunTimeErrorException {
public:
    using RunTimeErrorException::RunTimeErrorException;

    virtual ~SocketErrorException () noexcept {};
};

} /* namespace common */

#endif /* MUXEXCEPTION_H_ */
