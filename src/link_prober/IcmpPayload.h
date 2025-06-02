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
#include <unordered_set>
#include <boost/uuid/uuid.hpp>

#include <sys/types.h>

#define MUX_MAX_ICMP_BUFFER_SIZE    9100

__BEGIN_DECLS

#include <endian.h>
inline uint64_t ntohll(uint64_t x)
{
    return be64toh(x);
}

namespace link_prober
{
/**
 *@enum TlvType
 *
 *@brief Supported TLV types
 */
enum TlvType: uint8_t {
    TLV_COMMAND = 0x5,
    TLV_DUMMY = 0xfe,
    TLV_SENTINEL = 0xff,
};

struct TlvHead {
    uint8_t type;
    uint16_t length;
} __attribute__((packed));

struct Tlv {
    TlvHead tlvhead;
    union {
        uint8_t command;
        uint8_t data[1];
    };
} __attribute__((packed));

/**
 *@enum Command
 *
 *@brief Command to be sent to peer device
 */
enum class Command: uint8_t {
    COMMAND_NONE,
    COMMAND_SWITCH_ACTIVE,
    COMMAND_MUX_PROBE,

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
    uint8_t uuid[8];
    uint64_t seq;

    /**
    *@method IcmpPayload
    *
    *@brief class default constructor
    */
    IcmpPayload();

    /**
    *@method getCookie
    *
    *@brief getter for ICMP cookie by software probing
    *
    *@return ICMP cookie
    */
    static uint32_t getSoftwareCookie() {return mSoftwareCookie;};

    /**
    *@method getCookie
    *
    *@brief getter for ICMP cookie by hardware probing
    *
    *@return ICMP cookie
    */
    static uint32_t getHardwareCookie() {return mHardwareCookie;};

    /**
    *@method getVersion
    *
    *@brief getter for current version
    *
    *@return current version
    */
    static uint32_t getVersion() {return mVersion;};

private:
    static uint32_t mHardwareCookie;
    static uint32_t mSoftwareCookie;
    static uint32_t mVersion;
    
} __attribute__((packed));

static_assert(sizeof(IcmpPayload) % 2 == 0,
              "ICMP Payload size should be even sized, please add zero padding");

static_assert(sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr) + sizeof(IcmpPayload) < MUX_MAX_ICMP_BUFFER_SIZE,
              "Buffer Size doesn't fit Link Prober ICMP packet with its payload");

} /* namespace link_prober */

__END_DECLS

#pragma pack(pop)

#endif /* ICMPPAYLOAD_H_ */
