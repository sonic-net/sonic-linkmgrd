#include <memory>
#include <stdint.h>
#include <linux/filter.h>
#include "LinkProberBase.h"
#include <boost/bind/bind.hpp>

#include "common/MuxLogger.h"
#include "common/MuxException.h"
#include "LinkProberStateMachineActiveActive.h"
#include "LinkProberStateMachineActiveStandby.h"

namespace link_prober
{
//
// Berkeley Packet Filter program that captures incoming ICMP traffic
//
SockFilter LinkProberBase::mIcmpFilter[] = {
    [0]  = {.code = 0x28, .jt = 0, .jf = 0,  .k = 0x0000000c},
    [1]  = {.code = 0x15, .jt = 0, .jf = 10, .k = 0x00000800},
    [2]  = {.code = 0x20, .jt = 0, .jf = 0,  .k = 0x0000001a},
    [3]  = {.code = 0x15, .jt = 0, .jf = 8,  .k = 0x00000000},
    [4]  = {.code = 0x30, .jt = 0, .jf = 0,  .k = 0x00000017},
    [5]  = {.code = 0x15, .jt = 0, .jf = 6,  .k = 0x00000001},
    [6]  = {.code = 0x28, .jt = 0, .jf = 0,  .k = 0x00000014},
    [7]  = {.code = 0x45, .jt = 4, .jf = 0,  .k = 0x00001fff},
    [8]  = {.code = 0xb1, .jt = 0, .jf = 0,  .k = 0x0000000e},
    [9]  = {.code = 0x50, .jt = 0, .jf = 0,  .k = 0x0000000e},
    [10] = {.code = 0x15, .jt = 0, .jf = 1,  .k = 0x00000000},
    [11] = {.code = 0x6,  .jt = 0, .jf = 0,  .k = 0x00040000},
    [12] = {.code = 0x6,  .jt = 0, .jf = 0,  .k = 0x00000000},
};

// Set to hold all the session id's accross all ports of system for both Normal/Rx
std::unordered_set<std::string> LinkProberBase::mGuidSet;

LinkProberBase::LinkProberBase(common::MuxPortConfig &muxPortConfig, boost::asio::io_service &ioService,
            LinkProberStateMachineBase *linkProberStateMachinePtr) :
    mMuxPortConfig(muxPortConfig),
    mIoService(ioService),
    mLinkProberStateMachinePtr(linkProberStateMachinePtr),
    mStrand(mIoService),
    mStream(mIoService),
    mDeadlineTimer(mIoService),
    mSuspendTimer(mIoService),
    mSwitchoverTimer(mIoService)
{
    try {
        mSockFilterPtr = std::shared_ptr<SockFilter> (
            new SockFilter[sizeof(mIcmpFilter) / sizeof(*mIcmpFilter)],
            std::default_delete<SockFilter[]>()
        );
        memcpy(mSockFilterPtr.get(), mIcmpFilter, sizeof(mIcmpFilter));

        mSockFilterProg.len = sizeof(mIcmpFilter) / sizeof(*mIcmpFilter);
        mSockFilterProg.filter = mSockFilterPtr.get();
    }
    catch (const std::bad_alloc& ex) {
        std::ostringstream errMsg;
        errMsg << "Failed allocate memory. Exception details: " << ex.what();

        throw MUX_ERROR(BadAlloc, errMsg.str());
    }

    setSelfGuidData(generateGuid());
}

//
// ---> startInitRecv();
//
// start ICMP ECHOREPLY reception
//
void LinkProberBase::startInitRecv()
{
    MUXLOGTRACE(mMuxPortConfig.getPortName());

    mStream.async_read_some(
        boost::asio::buffer(mRxBuffer, MUX_MAX_ICMP_BUFFER_SIZE),
        mStrand.wrap(boost::bind(
            &LinkProberBase::handleInitRecv,
            this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred
        ))
    );
}

//
// ---> handleInitRecv(const boost::system::error_code& errorCode, size_t bytesTransferred);
//
// handle packet reception intialization
//
void LinkProberBase::handleInitRecv(
    const boost::system::error_code& errorCode,
    size_t bytesTransferred
)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    if (errorCode != boost::asio::error::operation_aborted) {
        ether_header *ethHeader = reinterpret_cast<ether_header *> (mRxBuffer.data());
        std::array<uint8_t, ETHER_ADDR_LEN> macAddress;

        memcpy(macAddress.data(), ethHeader->ether_shost, macAddress.size());

        boost::asio::io_service::strand &strand = mLinkProberStateMachinePtr->getStrand();
        boost::asio::io_service &ioService = strand.context();
        ioService.post(strand.wrap(boost::bind(
            &LinkProberStateMachineBase::handleMackAddressUpdate,
            mLinkProberStateMachinePtr,
            macAddress
        )));
    }
}

//
// ---> handleRecv(const boost::system::error_code& errorCode, size_t bytesTransferred);
//
// handle packet reception
//
void LinkProberBase::handleRecv(
    const boost::system::error_code& errorCode,
    size_t bytesTransferred
)
{
    MUXLOGDEBUG(mMuxPortConfig.getPortName());

    if (!errorCode) {
        iphdr *ipHeader = reinterpret_cast<iphdr *> (mRxBuffer.data() + sizeof(ether_header));
        icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (
            mRxBuffer.data() + sizeof(ether_header) + sizeof(iphdr)
        );

        MUXLOGTRACE(boost::format("%s: Got data from: %s, size: %d") %
            mMuxPortConfig.getPortName() %
            boost::asio::ip::address_v4(ntohl(ipHeader->saddr)).to_string() %
            (bytesTransferred - sizeof(iphdr) - sizeof(ether_header))
        );

        IcmpPayload *icmpPayload = reinterpret_cast<IcmpPayload *> (
            mRxBuffer.data() + mPacketHeaderSize
        );

        if (ntohl(icmpPayload->cookie) == IcmpPayload::getCookie() &&
            ntohl(icmpPayload->version) <= IcmpPayload::getVersion() &&
            (ntohs(icmpHeader->un.echo.id) == mMuxPortConfig.getServerId() ||
            mMuxPortConfig.getLinkFailureDetectionTypeHw() )) { 
            // echo.id in hw prober will not be set
            MUXLOGTRACE(boost::format("%s: Valid ICMP Packet from %s") %
                mMuxPortConfig.getPortName() %
                mMuxPortConfig.getBladeIpv4Address().to_string()
            );

            uint64_t network_guid = *reinterpret_cast<uint64_t*>(icmpPayload->uuid);
            uint64_t host_guid = ntohll(network_guid);
            host_guid = static_cast<uint32_t>(host_guid);
            auto guidDataStr = std::to_string(host_guid);
            bool isMatch = getSelfGuidData() == guidDataStr;
            HeartbeatType heartbeatType;
            if (isMatch) {
                MUXLOGTRACE(boost::format("%s: Matching Guid") % mMuxPortConfig.getPortName());
                // echo reply for an echo request generated by this/active ToR
                mRxSelfSeqNo = mTxSeqNo;
                heartbeatType = HeartbeatType::HEARTBEAT_SELF;
            } else {
                mRxPeerSeqNo = mTxSeqNo;
                heartbeatType = HeartbeatType::HEARTBEAT_PEER;
                setPeerGuidData(guidDataStr);
            }
            mReportHeartbeatReplyReceivedFuncPtr(heartbeatType);

            size_t nextTlvOffset = mTlvStartOffset;
            size_t nextTlvSize = 0;
            bool stopProcessTlv = false;
            while ((nextTlvSize = findNextTlv(nextTlvOffset, bytesTransferred)) > 0 && !stopProcessTlv) {
                Tlv *nextTlvPtr = reinterpret_cast<Tlv *> (mRxBuffer.data() + nextTlvOffset);
                switch (nextTlvPtr->tlvhead.type) {
                    case TlvType::TLV_COMMAND: {
                        handleTlvCommandRecv(nextTlvPtr, !isMatch);
                        break;
                    }
                    case TlvType::TLV_SENTINEL: {
                        // sentinel TLV, stop processing
                        stopProcessTlv = true;
                        break;
                    }
                    default: {
                        // try to skip unknown TLV with valid length(>0)
                        stopProcessTlv = (nextTlvSize == sizeof(Tlv));
                        break;
                    }
                }
                nextTlvOffset += nextTlvSize;
            }

            if (nextTlvOffset < bytesTransferred) {
                size_t BytesNotProcessed = bytesTransferred - nextTlvOffset;
                MUXLOGTRACE(boost::format("%s: %d bytes in RxBuffer not processed") %
                    mMuxPortConfig.getPortName() %
                    BytesNotProcessed
                );
            }
        } else {
            // Unknown ICMP packet, ignore.
            MUXLOGTRACE(mMuxPortConfig.getPortName() + ": Failed to receive heartbeat! Error code: " + errorCode.message());
        }
        // start another receive to consume as much as possible of backlog packets if any
        startRecv();
    }
}

//
// ---> handleTlvCommandRecv(Tlv *tlvPtr, bool isPeer);
//
// process icmp Tlv 
//
void LinkProberBase::handleTlvCommandRecv(
    Tlv *tlvPtr,
    bool isPeer
)
{
    if (isPeer) {
        boost::asio::io_service::strand &strand = mLinkProberStateMachinePtr->getStrand();

        switch (static_cast<Command>(tlvPtr->command)) {
            case Command::COMMAND_SWITCH_ACTIVE: {
                boost::asio::post(strand, boost::bind(
                    static_cast<void (LinkProberStateMachineBase::*) (SwitchActiveRequestEvent&)>(&LinkProberStateMachineBase::processEvent),
                    mLinkProberStateMachinePtr,
                    LinkProberStateMachineBase::getSwitchActiveRequestEvent()
                ));
                break;
            }
            case Command::COMMAND_MUX_PROBE: {
                boost::asio::post(strand, boost::bind(
                    static_cast<void (LinkProberStateMachineBase::*) (MuxProbeRequestEvent&)>(&LinkProberStateMachineBase::processEvent),
                    mLinkProberStateMachinePtr,
                    LinkProberStateMachineBase::getMuxProbeRequestEvent()
                ));
                break;
            }
            default: {
                break;
            }
        }
    }
}

//
// ---> findNextTlv
//
// Find next TLV to process in rxBuffer
//
size_t LinkProberBase::findNextTlv(size_t readOffset, size_t bytesTransferred)
{
    size_t tlvSize = 0;
    if (readOffset + sizeof(TlvHead) <= bytesTransferred) {
        Tlv *tlvPtr = reinterpret_cast<Tlv *> (mRxBuffer.data() + readOffset);
        tlvSize = (sizeof(TlvHead) + ntohs(tlvPtr->tlvhead.length));
        if (readOffset + tlvSize > bytesTransferred) {
            tlvSize = 0;
        }
    }
    return tlvSize;
}

//
// ---> startRecv();
//
// start ICMP ECHOREPLY reception
//
void LinkProberBase::startRecv()
{
    MUXLOGTRACE(mMuxPortConfig.getPortName());


    mStream.async_read_some(
        boost::asio::buffer(mRxBuffer, MUX_MAX_ICMP_BUFFER_SIZE),
        mStrand.wrap(boost::bind(
            &LinkProberBase::handleRecv,
            this,
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred
        ))
    );
}

//
// ---> uuidToHexString(const boost::uuids::uuid& uuid);
//
// convert uuid to string in hes form
//
std::string LinkProberBase::uuidToHexString(const boost::uuids::uuid& uuid)
{
    std::ostringstream oss;
    oss << std::hex << std::setfill('0'); // Hex formatting with leading zeros

    for (const auto& byte : uuid) {
        oss << std::setw(2) << static_cast<unsigned int>(byte); // Convert each byte to hex
    }

    return oss.str();
}

//
// ---> generateGuid();
//
// generate GUID for link_prober
//
std::string LinkProberBase::generateGuid()
{
    boost::uuids::uuid generatedGuid;
    boost::uuids::random_generator gen;
    generatedGuid = gen();
    std::fill(generatedGuid.begin(), generatedGuid.end() - 4, 0);
    auto generatedGuidStr = uuidToHexString(generatedGuid);

    if(mGuidSet.find(generatedGuidStr) == mGuidSet.end()) {
        mGuidSet.insert(generatedGuidStr);
        MUXLOGWARNING(boost::format("Link Prober generated GUID: {%s}") % generatedGuidStr);
    }
    else {
        MUXLOGWARNING(boost::format("Guid collision happened for guid : {%s}") % generatedGuidStr);
        generatedGuidStr = generateGuid();
    }
    mSelfUUID = generatedGuid;
    return generatedGuidStr;
}

}
