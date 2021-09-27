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
 * IcmpPayload.h
 *
 *  Created on: Oct 9, 2020
 *      Author: tamer
 */

#ifndef ICMPPAYLOAD_H_
#define ICMPPAYLOAD_H_

#pragma pack(push, 1)

#include <assert.h>
#include <stdint.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <net/ethernet.h>

#include <boost/uuid/uuid.hpp>

#include <sys/types.h>

#define MUX_MAX_ICMP_BUFFER_SIZE    128

__BEGIN_DECLS

namespace link_prober
{

/**
 *@enum Command
 *
 *@brief Command to be sent to peer device
 */
enum class Command: uint32_t {
    COMMAND_NONE,
    COMMAND_SWITCH_ACTIVE,

    Count
};

/**
 *@struct IcmpPayload
 *
 *@brief Build ICMP packet to sent to server
 */
struct IcmpPayload {
    uint32_t cookie;
    uint32_t version;
    union {
        uint64_t guid[2];
        boost::uuids::uuid uuid;
    } un;
    uint32_t command;

    /**
    *@method IcmpPayload
    *
    *@brief class default constructor
    */
    IcmpPayload();

    /**
    *@method generateGuid
    *
    *@brief generate GUID for the current instance of linkmgrd
    *
    *@return none
    */
    static void generateGuid();

    /**
    *@method getGuidData
    *
    *@brief getter for GUID data
    *
    *@return pointer to current GUID data
    */
    static uint8_t* getGuidData() {return reinterpret_cast<uint8_t *> (mGuid.data);};

    /**
    *@method getGuid
    *
    *@brief getter for GUID object
    *
    *@return reference to current GUID object
    */
    static boost::uuids::uuid& getGuid() {return mGuid;};

    /**
    *@method getCookie
    *
    *@brief getter for ICMP cookie
    *
    *@return ICMP coolie
    */
    static uint32_t getCookie() {return mCookie;};

    /**
    *@method getVersion
    *
    *@brief getter for current version
    *
    *@return current version
    */
    static uint32_t getVersion() {return mVersion;};

private:
    static uint32_t mCookie;
    static uint32_t mVersion;
    static boost::uuids::uuid mGuid;
} __attribute__((packed));

static_assert(sizeof(IcmpPayload) % 2 == 0,
              "ICMP Payload size should be even sized, please add zero padding");

static_assert(sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr) + sizeof(IcmpPayload) < MUX_MAX_ICMP_BUFFER_SIZE,
              "Buffer Size doesn't fit Link Prober ICMP packet with its payload");

} /* namespace link_prober */

__END_DECLS

#pragma pack(pop)

#endif /* ICMPPAYLOAD_H_ */
