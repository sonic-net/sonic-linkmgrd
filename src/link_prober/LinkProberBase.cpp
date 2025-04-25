#include <memory>
#include <stdint.h>
#include <linux/filter.h>
#include "LinkProberBase.h"
#include <boost/bind/bind.hpp>
#include <sstream>
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

void LinkProberBase::setupSocket() {
	SockAddrLinkLayer addr = {0};
    addr.sll_ifindex = if_nametoindex(mMuxPortConfig.getPortName().c_str());
    addr.sll_family = AF_PACKET;
    addr.sll_protocol = htons(ETH_P_ALL);

    mSocket = socket(AF_PACKET, SOCK_RAW | SOCK_NONBLOCK, IPPROTO_ICMP);
    if (mSocket < 0) {
        std::ostringstream errMsg;
        errMsg << "Failed to open socket with '" << strerror(errno) << "'"
               << std::endl;
        throw MUX_ERROR(SocketError, errMsg.str());
    }

    if (bind(mSocket, (struct sockaddr *) &addr, sizeof(addr))) {
        std::ostringstream errMsg;
        errMsg << "Failed to bind to interface '" << mMuxPortConfig.getPortName() << "' with '"
               << strerror(errno) << "'" << std::endl;
        throw MUX_ERROR(SocketError, errMsg.str());
    }

    mSockFilterPtr.get()[3].k = mMuxPortConfig.getBladeIpv4Address().to_v4().to_uint();
    if (setsockopt(mSocket, SOL_SOCKET, SO_ATTACH_FILTER, &mSockFilterProg, sizeof(mSockFilterProg)) != 0) {
        std::ostringstream errMsg;
        errMsg << "Failed to attach filter with '" << strerror(errno) << "'"
               << std::endl;
        throw MUX_ERROR(SocketError, errMsg.str());
    }

    mStream.assign(mSocket);

    initializeSendBuffer();
    startInitRecv();
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
        //ether_header *ethHeader = reinterpret_cast<ether_header *> (mRxBuffer.data());
        std::array<uint8_t, ETHER_ADDR_LEN> macAddress;
        
        if (mMuxPortConfig.ifEnableUseTorMac()) {
            memcpy(macAddress.data(), mMuxPortConfig.getTorMacAddress().data(),macAddress.size());
        } else {
            memcpy(macAddress.data(), mMuxPortConfig.getVlanMacAddress().data(),macAddress.size());
        }

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
    if( mMuxPortConfig.getLinkFailureDetectionTypeHw()){
        MUXLOGWARNING(boost::format("Raw GUID PORT: {%s}") % mMuxPortConfig.getPortName());
    }

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

        bool isHardware = ntohl(icmpPayload->cookie) == IcmpPayload::getHardwareCookie();

        if ((ntohl(icmpPayload->cookie) == IcmpPayload::getSoftwareCookie() || isHardware) &&
            ntohl(icmpPayload->version) <= IcmpPayload::getVersion() &&
            (ntohs(icmpHeader->un.echo.id) == mMuxPortConfig.getServerId() ||
                mMuxPortConfig.getLinkFailureDetectionTypeHw() )) { 
            // echo.id in hw prober will not be set
            MUXLOGTRACE(boost::format("%s: Valid ICMP Packet from %s") %
                mMuxPortConfig.getPortName() %
                mMuxPortConfig.getBladeIpv4Address().to_string()
            );

            uint64_t network_guid = *reinterpret_cast<uint64_t*>(icmpPayload->uuid);
            if (network_guid == 0) {
                MUXLOGWARNING(boost::format("Received invalid Raw GUID: {%d}") % network_guid);
                startRecv();
                return;
            }
            //MUXLOGWARNING(boost::format("Raw GUID: {0x%016llx}") % network_guid);
            uint64_t host_guid = ntohll(network_guid);
            //MUXLOGWARNING(boost::format("ntohll GUID: {0x%016llx}") % host_guid);
            host_guid = static_cast<uint32_t>(host_guid);
            //MUXLOGWARNING(boost::format("uint32_t  GUID: {0x0x%08x}") % host_guid);
            std::stringstream os;
            os << std::hex << std::setw(8) << std::setfill('0') << host_guid;
            auto guidDataStr = os.str();
            if( mMuxPortConfig.getLinkFailureDetectionTypeHw()){
                MUXLOGWARNING(boost::format("Link Prober recieved GUID: {%s}") % guidDataStr);
            }
            guidDataStr = "0x" + guidDataStr;
            bool isMatch = getSelfGuidData() == guidDataStr;
            if(mMuxPortConfig.getLinkFailureDetectionTypeHw() &&
                (mGuidSet.find(guidDataStr) != mGuidSet.end()) ){
                if (isMatch) {
                    MUXLOGWARNING(boost::format("Received  Duplicate Self GUID: {%s}") % guidDataStr);
                } else {
                    MUXLOGWARNING(boost::format("Received  Duplicate Peer GUID: {%s}") % guidDataStr);
                }
                startRecv();
                return;
            }
            HeartbeatType heartbeatType;
            if (isMatch) {
               // MUXLOGWARNING(boost::format("%s: Self Guid Detected") % mMuxPortConfig.getPortName());
                // echo reply for an echo request generated by this/active ToR
                mRxSelfSeqNo = mTxSeqNo;
                heartbeatType = HeartbeatType::HEARTBEAT_SELF;
            } else {
                mRxPeerSeqNo = mTxSeqNo;
                heartbeatType = HeartbeatType::HEARTBEAT_PEER;
                MUXLOGWARNING(boost::format("Peer Guid Detected %s") % guidDataStr);
                setPeerGuidData(guidDataStr);
            }
            mReportHeartbeatReplyReceivedFuncPtr(heartbeatType);

            if(!isHardware){
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
                MUXLOGWARNING(boost::format("SwitchActiveRequestEvent"));
                boost::asio::post(strand, boost::bind(
                    static_cast<void (LinkProberStateMachineBase::*) (SwitchActiveRequestEvent&)>(&LinkProberStateMachineBase::processEvent),
                    mLinkProberStateMachinePtr,
                    LinkProberStateMachineBase::getSwitchActiveRequestEvent()
                ));
                break;
            }
            case Command::COMMAND_MUX_PROBE: {
                MUXLOGWARNING(boost::format("MuxProbeRequestEvent"));
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
// ---> initializeSendBuffer();
//
// initialize ICMP packet once
//
void LinkProberBase::initializeSendBuffer()
{
    ether_header *ethHeader = reinterpret_cast<ether_header *> (mTxBuffer.data());
    memcpy(ethHeader->ether_dhost, mMuxPortConfig.getBladeMacAddress().data(), sizeof(ethHeader->ether_dhost));
    if (mMuxPortConfig.ifEnableUseTorMac()) {
        memcpy(ethHeader->ether_shost, mMuxPortConfig.getTorMacAddress().data(), sizeof(ethHeader->ether_shost));
    } else {
        memcpy(ethHeader->ether_shost, mMuxPortConfig.getVlanMacAddress().data(), sizeof(ethHeader->ether_shost));
    }
    ethHeader->ether_type = htons(ETHERTYPE_IP);

    iphdr *ipHeader = reinterpret_cast<iphdr *> (mTxBuffer.data() + sizeof(ether_header));
    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (mTxBuffer.data() + sizeof(ether_header) + sizeof(iphdr));

    IcmpPayload *payloadPtr  = new (mTxBuffer.data() + mPacketHeaderSize) IcmpPayload();
    memcpy(payloadPtr->uuid, (mSelfUUID.data + 8), sizeof(payloadPtr->uuid));
    resetTxBufferTlv();
    appendTlvSentinel();
    size_t totalPayloadSize = mTxPacketSize - mPacketHeaderSize;

    ipHeader->ihl = sizeof(iphdr) >> 2;
    ipHeader->version = IPVERSION;
    ipHeader->tos = 0xb8;
    ipHeader->tot_len = htons(sizeof(iphdr) + sizeof(icmphdr) + totalPayloadSize);
    ipHeader->id = static_cast<uint16_t> (rand());
    ipHeader->frag_off = 0;
    ipHeader->ttl = 64;
    ipHeader->protocol = IPPROTO_ICMP;
    ipHeader->check = 0;
    ipHeader->saddr = htonl(mMuxPortConfig.getLoopbackIpv4Address().to_v4().to_uint());
    ipHeader->daddr = htonl(mMuxPortConfig.getBladeIpv4Address().to_v4().to_uint());
    computeChecksum(ipHeader, ipHeader->ihl << 2);

    icmpHeader->type = ICMP_ECHO;
    icmpHeader->code = 0;
    icmpHeader->un.echo.id = htons(mMuxPortConfig.getServerId());
    icmpHeader->un.echo.sequence = htons(mTxSeqNo);

    computeChecksum(icmpHeader, sizeof(icmphdr) + totalPayloadSize);
}

//
// ---> appendTlvSentinel
//
// Append TlvSentinel to the end of txBuffer
//
size_t LinkProberBase::appendTlvSentinel()
{
    size_t tlvSize = sizeof(TlvHead);
    assert(mTxPacketSize + tlvSize <= MUX_MAX_ICMP_BUFFER_SIZE);
    Tlv *tlvPtr = reinterpret_cast<Tlv *> (mTxBuffer.data() + mTxPacketSize);
    tlvPtr->tlvhead.type = TlvType::TLV_SENTINEL;
    tlvPtr->tlvhead.length = 0;
    mTxPacketSize += tlvSize;
    return tlvSize;
}

//
// ---> sendPeerProbeCommand();
//
// send peer probe command
//
void LinkProberBase::sendPeerProbeCommand()
{
    boost::asio::post(mStrand, boost::bind(&LinkProberBase::handleSendProbeCommand, this));
}

//
// ---> probePeerTor();
//
// send an early HB to peer ToR
//
void LinkProberBase::probePeerTor()
{
    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(&LinkProberBase::sendHeartbeat, this, false)));
}

//
// ---> handleSendProbeCommand();
//
// send probe command to peer ToR
//
void LinkProberBase::handleSendProbeCommand()
{
    initTxBufferTlvSendProbe();

    sendHeartbeat();

    initTxBufferTlvSentinel();
}

//
// ---> handleSendSwitchCommand();
//
// send switch command to peer ToR
//
void LinkProberBase::handleSendSwitchCommand()
{
    initTxBufferTlvSendSwitch();

    sendHeartbeat();

    initTxBufferTlvSentinel();

    // inform the composite state machine about command send completion
    boost::asio::io_service::strand &strand = mLinkProberStateMachinePtr->getStrand();
    boost::asio::io_service &ioService = strand.context();
    ioService.post(strand.wrap(boost::bind(
        static_cast<void (LinkProberStateMachineBase::*) (SwitchActiveCommandCompleteEvent&)>
            (&LinkProberStateMachineBase::processEvent),
        mLinkProberStateMachinePtr,
        LinkProberStateMachineBase::getSwitchActiveCommandCompleteEvent()
    )));
}

//
// ---> sendPeerSwitchCommand();
//
// send send peer switch command
//
void LinkProberBase::sendPeerSwitchCommand()
{
    boost::asio::io_service &ioService = mStrand.context();
    ioService.post(mStrand.wrap(boost::bind(&LinkProberBase::handleSendSwitchCommand, this)));
}

//
// ---> sendHeartbeat(bool forceSend)
//
// send ICMP ECHOREQUEST packet
//
void LinkProberBase::sendHeartbeat(bool forceSend)
{
    MUXLOGTRACE(mMuxPortConfig.getPortName());

    updateIcmpSequenceNo();
    // check if suspend timer is running
    if (forceSend || ((!mSuspendTx) && (!mShutdownTx))) {
        boost::system::error_code errorCode;
        mStream.write_some(boost::asio::buffer(mTxBuffer.data(), mTxPacketSize), errorCode);

        if (errorCode) {
            MUXLOGTRACE(mMuxPortConfig.getPortName() + ": Failed to send heartbeat! Error code: " + errorCode.message());
        } else {
            MUXLOGTRACE(mMuxPortConfig.getPortName() + ": Done sending data");
        }
    }
}

//
// ---> calculateChecksum(uint16_t *data, size_t size);
//
// calculate ICMP payload checksum
//
uint32_t LinkProberBase::calculateChecksum(uint16_t *data, size_t size)
{
    uint32_t sum = 0;

    while (size > 1) {
        sum += ntohs(*data++);
        size -= sizeof(uint16_t);
    }

    if (size) {
        sum += ntohs(static_cast<uint16_t> ((*reinterpret_cast<uint8_t *> (data))));
    }

    return sum;
}

//
// ---> addChecksumCarryover(uint16_t *checksum, uint32_t sum);
//
// add checksum carryover
//
void LinkProberBase::addChecksumCarryover(uint16_t *checksum, uint32_t sum)
{
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    *checksum = htons(~sum);
}

//
// ---> computeChecksum(icmphdr *icmpHeader, size_t size);
//
// compute ICMP checksum
//
void LinkProberBase::computeChecksum(icmphdr *icmpHeader, size_t size)
{
    icmpHeader->checksum = 0;
    mIcmpChecksum = calculateChecksum(
        reinterpret_cast<uint16_t *> (icmpHeader), size
    );
    addChecksumCarryover(&icmpHeader->checksum, mIcmpChecksum);
}

//
// ---> computeChecksum(iphdr *ipHeader, size_t size);
//
// compute IPv4 checksum
//
void LinkProberBase::computeChecksum(iphdr *ipHeader, size_t size)
{
    ipHeader->check = 0;
    mIpChecksum = calculateChecksum(
        reinterpret_cast<uint16_t *> (ipHeader), size
    );
    addChecksumCarryover(&ipHeader->check, mIpChecksum);
}



//
// ---> updateIcmpSequenceNo();
//
// update ICMP packet checksum, used before sending new heartbeat
//
void LinkProberBase::updateIcmpSequenceNo()
{
    // update received sequence to avoid reporting invalid ICMP event when sequence number rolls over
    mRxPeerSeqNo = mTxSeqNo;
    mRxSelfSeqNo = mTxSeqNo;

    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (mTxBuffer.data() + sizeof(ether_header) + sizeof(iphdr));
    icmpHeader->un.echo.sequence = htons(++mTxSeqNo);
    mIcmpChecksum += mTxSeqNo ? 1 : 0;
    addChecksumCarryover(&icmpHeader->checksum, mIcmpChecksum);
}

//
// ---> appendTlvCommand
//
// Append TlvCommand to the end of txBuffer
//
size_t LinkProberBase::appendTlvCommand(Command commandType)
{
    size_t tlvSize = sizeof(TlvHead) + sizeof(Command);
    assert(mTxPacketSize + tlvSize <= MUX_MAX_ICMP_BUFFER_SIZE);
    Tlv *tlvPtr = reinterpret_cast<Tlv *> (mTxBuffer.data() + mTxPacketSize);
    tlvPtr->tlvhead.type = TlvType::TLV_COMMAND;
    tlvPtr->tlvhead.length = htons(sizeof(Command));
    tlvPtr->command = static_cast<uint8_t> (commandType);
    mTxPacketSize += tlvSize;
    return tlvSize;
}

//
// ---> initTxBufferTlvSendProbe
//
// Initialize TX buffer TLVs to send probe command to peer
//
void LinkProberBase::initTxBufferTlvSendProbe()
{
    resetTxBufferTlv();
    appendTlvCommand(Command::COMMAND_MUX_PROBE);
    appendTlvSentinel();

    calculateTxPacketChecksum();
}

//
// ---> initTxBufferTlvSentinel
//
// Initialize TX buffer to have only TLV sentinel
//
void LinkProberBase::initTxBufferTlvSentinel()
{
    resetTxBufferTlv();
    appendTlvSentinel();

    calculateTxPacketChecksum();
}

//
// ---> initTxBufferTlvSendSwitch
//
// Initialize TX buffer TLVs to send switch command to peer
//
void LinkProberBase::initTxBufferTlvSendSwitch()
{
    resetTxBufferTlv();
    appendTlvCommand(Command::COMMAND_SWITCH_ACTIVE);
    appendTlvSentinel();

    calculateTxPacketChecksum();
}

//
// ---> calculateTxPacketChecksum
//
// Calculate TX packet checksums in both IP header and ICMP header
//
void LinkProberBase::calculateTxPacketChecksum()
{
    size_t totalPayloadSize = mTxPacketSize - mPacketHeaderSize;
    iphdr *ipHeader = reinterpret_cast<iphdr *> (mTxBuffer.data() + sizeof(ether_header));
    icmphdr *icmpHeader = reinterpret_cast<icmphdr *> (mTxBuffer.data() + sizeof(ether_header) + sizeof(iphdr));
    computeChecksum(icmpHeader, sizeof(icmphdr) + totalPayloadSize);
    ipHeader->tot_len = htons(sizeof(iphdr) + sizeof(icmphdr) + totalPayloadSize);
    computeChecksum(ipHeader, ipHeader->ihl << 2);
}

//
// ---> appendTlvDummy
//
// Append a dummy TLV, test purpose only
//
size_t LinkProberBase::appendTlvDummy(size_t paddingSize, int seqNo)
{
    size_t tlvSize = sizeof(TlvHead) + paddingSize + sizeof(uint32_t);
    assert(mTxPacketSize + tlvSize <= MUX_MAX_ICMP_BUFFER_SIZE);
    Tlv *tlvPtr = reinterpret_cast<Tlv *> (mTxBuffer.data() + mTxPacketSize);
    tlvPtr->tlvhead.type = TlvType::TLV_DUMMY;
    tlvPtr->tlvhead.length = htons(paddingSize + sizeof(uint32_t));
    memset(tlvPtr->data, 0, paddingSize);
    *(reinterpret_cast<uint32_t *> (tlvPtr->data + paddingSize)) = htonl(seqNo);
    mTxPacketSize += tlvSize;
    return tlvSize;
}

//
// ---> handleUpdateEthernetFrame();
//
// update Ethernet frame of Tx Buffer
//
void LinkProberBase::handleUpdateEthernetFrame()
{
    initializeSendBuffer();
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
    generatedGuidStr = "0x" + generatedGuidStr.substr(generatedGuidStr.length() - 8);
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
