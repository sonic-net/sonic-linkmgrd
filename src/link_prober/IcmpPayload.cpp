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
 * IcmpPayload.cpp
 *
 *  Created on: Oct 9, 2020
 *      Author: tamer
 */

#include <string.h>

#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>

#include "IcmpPayload.h"
#include "common/MuxLogger.h"

namespace link_prober
{
//
// static members
//
boost::uuids::uuid IcmpPayload::mGuid;
uint32_t IcmpPayload::mCookie = 0x47656d69;
uint32_t IcmpPayload::mVersion = 0;

//
// ---> IcmpPayload();
//
// class constructor
//
IcmpPayload::IcmpPayload() :
    cookie(htonl(mCookie)),
    version(htonl(mVersion)),
    command(htonl(static_cast<uint32_t> (Command::COMMAND_NONE)))
{
    memcpy(un.uuid.data, mGuid.data, sizeof(un.uuid.data));
}

//
// ---> generateGuid()
//
// generate GUID for the current instance of linkmgrd
//
void IcmpPayload::generateGuid()
{
    boost::uuids::random_generator gen;
    mGuid = gen();

    MUXLOGWARNING(boost::format("Link Prober generated GUID: {%s}") % boost::uuids::to_string(mGuid));
}

} /* namespace link_prober */
