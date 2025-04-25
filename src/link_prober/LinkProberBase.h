#ifndef LINK_PROBER_LINKPROBERBASE_H_
#define LINK_PROBER_LINKPROBERBASE_H_

#include <memory>
#include <stdint.h>
#include <vector>
#include <linux/filter.h>
#include <unordered_set>
#include <iomanip>

#include <netpacket/packet.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <malloc.h>

#include <common/BoostAsioBehavior.h>
#include <boost/asio.hpp>
#include <boost/function.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/lexical_cast.hpp>
#include "LinkProberStateMachineBase.h"

#include "IcmpPayload.h"
#include "common/MuxPortConfig.h"
#include "common/MuxLogger.h"

namespace std {
    template <>
    struct hash<boost::uuids::uuid> {
        std::size_t operator()(const boost::uuids::uuid& uuid) const {
            // Use boost's built-in hash function for UUIDs
            return boost::uuids::hash_value(uuid);
        }
    };
}

namespace test {
class LinkProberTest;
class LinkProberMockTest;
}

namespace link_prober
{

/**
 *@enum HeartbeatType
 *
 *@brief Received heartbeat type
 */
enum class HeartbeatType: uint8_t {
    HEARTBEAT_SELF,
    HEARTBEAT_PEER,

    Count
};

using SockFilter = struct sock_filter;
using SockFilterProg = struct sock_fprog;
using SockAddrLinkLayer = struct sockaddr_ll;


/**
 *@class LinkProberBase
 *
 *@brief probes the server sing ICMP ECHPREQUEST packet. The packet payload
 *       holds GUID that identifies this ToR. Reception of this ToR's GUID
 *       indicate that the link is in Active state. Reception of unknown
 *       GUID will indicate standby state. Lack of ICMP packets will signal
 *       that the link state is unknown and it might be newly learned peers UID
 */
class LinkProberBase
{
public:
    /**
    *@method LinkProberBase
    *
    *@brief class default constructor
    */
    LinkProberBase() = delete;

    /**
    *@method LinkProberBase
    *
    *@brief class copy constructor
    *
    *@param LinkProberBase (in)  reference to LinkProberBase object to be copied
    */
    LinkProberBase(const LinkProberBase &) = delete;


    /**
    *@method ~LinkProberBase
    *
    *@brief class destructor
    */
    ~LinkProberBase() = default;

    /**
    *@method LinkProberBase
    *@brief class constructor
    *
    *@param muxPortConfig (in)          reference to MuxPortConfig object
    *@param ioService (in)              reference to boost io_service object
    *@param linkProberStateMachinePtr (in) reference to LinkProberStateMachineBase object
    */
    LinkProberBase(common::MuxPortConfig &muxPortConfig, boost::asio::io_service &ioService,
            LinkProberStateMachineBase *linkProberStateMachinePtr);

    
    /**
    *@method initialize
    *
    *@brief initialize link prober sockets and builds ICMP packet
    *
    *@return none
    */
    virtual void initialize() {
        MUXLOGWARNING(boost::format("Link Prober Base initialize not implemented"));
    };

    /**
    *@method startProbing
    *
    *@brief start probing server/blade using ICMP ECHOREQUEST
    *
    *@return none
    */
    virtual void startProbing() {
        MUXLOGWARNING(boost::format("Link Prober startProbing not implemented"));
    }

    /**
    *@method suspendTxProbes
    *
    *@brief suspend sending ICMP ECHOREQUEST packets
    *
    *@param suspendTime_msec suspension time in msec
    *
    *@return none
    */
    virtual void suspendTxProbes(uint32_t suspendTime_msec) {
        MUXLOGWARNING(boost::format("Link Prober suspendTxProbes not implemented"));
    }

    /**
    *@method resumeTxProbes
    *
    *@brief resume sending ICMP ECHOREQUEST packets
    *
    *@return none
    */
    virtual void resumeTxProbes() {
        MUXLOGWARNING(boost::format("Link Prober resumeTxProbes not implemented"));
    }

    /**
    *@method detectLink
    *
    *@brief detect link status
    *
    *@return none
    */
    virtual void detectLink() {
        MUXLOGWARNING(boost::format("Link Prober detectLink not implemented"));
    }

    /**
    * @method resetIcmpPacketCounts()
    * 
    * @brief reset Icmp packet counts, post a pck loss ratio update immediately 
    * 
    * @return none
    */
    virtual void resetIcmpPacketCounts() {
        MUXLOGWARNING(boost::format("Link Prober resetIcmpPacketCounts not implemented"));
    }

    /**
    * @method handleStateDbStateUpdate
    * 
    * @brief handle state change of ICMP_ECHO_SESSION_TABLE in STATE_DB
    * 
    * @return none
    */
    virtual void handleStateDbStateUpdate(const std::string &linkFailureDetectionState, const std::string session_type) {
        MUXLOGWARNING(boost::format("Link Prober handleStateDbStateUpdate not implemented"));
    }

    /**
    *@method shutdownTxProbes
    *
    *@brief stop sending ICMP ECHOREQUEST packets
    *
    *@return none
    */
    virtual void shutdownTxProbes() {
        MUXLOGWARNING(boost::format("Link Prober shutdownTxProbes not implemented"));
    }

    /**
     * @method restartTxProbes
     * 
     * @brief restart sending ICMP ECHOREQUEST packets
     * 
     * @return none
     */
    virtual void restartTxProbes() {
        MUXLOGWARNING(boost::format("Link Prober restartTxProbes not implemented"));
    }

    /**
     * @method decreaseProbeIntervalAfterSwitch
     *  
     * @brief adjust link prober interval to 10 ms after switchover to better measure the switchover overhead.
     * 
     * @param switchTime_msec (in) switchover is expected to complete  within this time window
     * @param expectingLinkProberEvent (in) depends on which state LinkManager is switching to, link prober expects self or peer events
     * 
     * @return none
     */
    virtual void decreaseProbeIntervalAfterSwitch(uint32_t switchTime_msec) {
        MUXLOGWARNING(boost::format("Link Prober decreaseProbeIntervalAfterSwitch not implemented"));
    }

    /**
     * @method revertProbeIntervalAfterSwitchComplete 
     * 
     * @brief revert probe interval change after switchover is completed
     * 
     * @return none
     */
    virtual void revertProbeIntervalAfterSwitchComplete(){
        MUXLOGWARNING(boost::format("Link Prober revertProbeIntervalAfterSwitchComplete not implemented"));
    }

    /**
    *@method getSelfGuidData
    *
    *@brief getter for self GUID data
    *
    *@return pointer to current self GUID data
    */
    inline std::string getSelfGuidData() {
        return mSelfGuid;
    }

    /**
    *@method getPeerGuidData
    *
    *@brief getter for PEER GUID data
    *
    *@return pointer to current PEER GUID data
    */
    inline std::string getPeerGuidData() {
        return mPeerGuid;
    }

    /**
    *@method setSelfGuidData
    *
    *@brief setter for self GUID data
    *
    *@return none
    */
   inline void setSelfGuidData(std::string l_guid) {
       mSelfGuid = l_guid;
   }

   /**
    *@method setPeerGuidData
    *
    *@brief setter for PEER GUID data
    *
    *@return none
    */
    inline void setPeerGuidData(std::string l_guid) {
        mPeerGuid = l_guid;
    }

    /**
    * @method getProbingInterval
    * 
    * @brief get link prober interval
    * 
    * @return link prober interval
    */
    inline uint32_t getProbingInterval() {
        MUXLOGDEBUG(mMuxPortConfig.getPortName());
        return mDecreaseProbingInterval? mMuxPortConfig.getDecreasedTimeoutIpv4_msec():mMuxPortConfig.getTimeoutIpv4_msec();
    }

    /**
    *@method updateEthernetFrame
    *
    *@brief update Ethernet frame of Tx Buffer
    *
    *@return none
    */
   virtual void updateEthernetFrame() {
    MUXLOGWARNING(boost::format("Link Prober updateEthernetFrame not implemented"));
    }

    /**
    *@method probePeerTor
    *
    *@brief send an early HB to peer ToR
    *
    *@return none
    */
    virtual void probePeerTor();

    /**
    *@method sendPeerProbeCommand
    *
    *@brief send probe command to peer ToR
    *
    *@return none
    */
   virtual void sendPeerProbeCommand();

    /**
    *@method sendPeerSwitchCommand
    *
    *@brief send switch command to peer ToR
    *
    *@return none
    */
    virtual void sendPeerSwitchCommand();

    /**
     * @method initTxBufferTlvSendProbe
     *
     * @brief initialize TX buffer TLVs to send probe command to peer
     *
     * @return none
     */
    void initTxBufferTlvSendProbe();

    /**
      * @method initTxBufferTlvSentinel
      * 
      * @brief initialize TX buffer to have only TLV sentinel
      * 
      * @return none
      */
    void initTxBufferTlvSentinel();

    /**
     * @method initTxBufferTlvSendSwitch
     * 
     * @brief initialize TX buffer TLVs to send switch command to peer
     * 
     * @return none
     */
    void initTxBufferTlvSendSwitch();

    void resetTxBufferTlv() {mTxPacketSize = mTlvStartOffset;};

    static std::unordered_set<std::string> mGuidSet;
    boost::uuids::uuid mSelfUUID;
    
protected:

    

    /**
    *@method addChecksumCarryover
    *
    *@brief add checksum carryover
    *
    *@param checksum (out)  pointer to checksum field
    *@param sum (in)        current sum of the buffer payload
    *
    *@return CRC checksum
    */
   void addChecksumCarryover(uint16_t *checksum, uint32_t sum);

    /**
    * @method startInitRecv
    * 
    * @brief start ICMP ECHOREPLY reception
    * 
    * @return none
    */
   void startInitRecv();

   /**
   * @method startRecv
   * 
   * @brief start ICMP ECHOREPLY reception
   * 
   * @return none
   */
   void startRecv();

   /**
   * @method setupSocket
   * 
   * @brief setup socket filter and 
   * 
   * @return none
   */
   void setupSocket();

   /**
   *@method handleTlvCommandRecv
   *
   *@brief handle TLV command
   *
   *@param tlvPtr (in)     Tlv ptr points to the start of TlvCommand in mRxBuffer
   *@param isPeer (in)     True if the reply received is from the peer ToR
   *
   *@return none
   */
   void handleTlvCommandRecv(
       Tlv *tlvPtr,
       bool isPeer
   );

       /**
   *@method handleRecv
   *
   *@brief handle packet reception
   *
   *@param errorCode (in)          socket error code
   *@param bytesTransferred (in)   number of bytes received
   *
   *@return none
   */
   void handleRecv(
       const boost::system::error_code &errorCode,
       size_t bytesTransferred
   );

   /**
   *@method handleInitRecv
   *
   *@brief handle packet reception
   *
   *@param errorCode (in)          socket error code
   *@param bytesTransferred (in)   number of bytes received
   *
   *@return none
   */
   void handleInitRecv(
       const boost::system::error_code &errorCode,
       size_t bytesTransferred
   );

   /**
   *@method handleSendProbeCommand
   *
   *@brief send probe command to peer ToR
   *
   *@return none
   */
   void handleSendProbeCommand();

   /**
   *@method handleSendSwitchCommand
   *
   *@brief send switch command to peer ToR
   *
   *@return none
   */
   void handleSendSwitchCommand();
    
    /**
    *@method computeChecksum
    *
    *@brief compute ICMP checksum
    *
    *@param icmpHeader (in, out)    pointer ICMP header
    *@param size (in)               size of ICMP payload
    *
    *@return CRC checksum
    */
    void computeChecksum(icmphdr *icmpHeader, size_t size);

    /**
    *@method computeChecksum
    *
    *@brief compute IPv4 checksum
    *
    *@param ipHeader (in, out)  pointer IPv4 header
    *@param size (in)           size of IPv4 header
    *
    *@return CRC checksum
    */
    void computeChecksum(iphdr *ipHeader, size_t size);

    /**
    *@method updateIcmpSequenceNo
    *
    *@brief update ICMP packet checksum, used before sending new heartbeat
    *
    *@return CRC checksum
    */
    void updateIcmpSequenceNo();

    /**
    *@method getTxBuffer
    *
    *@brief getter for TxBuffer used for testing
    *
    *@return tx buffer
    */
    std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> getTxBuffer() {return mTxBuffer;};

    /**
    *@method findNextTlv
    *
    *@brief Find next TLV in rxBuffer starting at readOffset
    *
    *@param readOffset (in)         starting offset to read
    *@param bytesTransferred (in)   total bytes received in rxBuffer
    *
    *@return the next TLV size
    */
    size_t findNextTlv(size_t readOffset, size_t bytesTransferred);

    /**
    *@method handleUpdateEthernetFrame
    *
    *@brief update Ethernet frame of Tx Buffer
    *
    *@return none
    */
    void handleUpdateEthernetFrame();


    /**
    *@method sendHeartbeat
    *
    *@brief send ICMP ECHOREQUEST packet
    *
    *@param forceSend (in)  Force sending heartbeat, used in link detect only
    *
    *@return none
    */
    void sendHeartbeat(bool forceSend = false);

    /**
    *@method calculateChecksum
    *
    *@brief calculate ICMP payload checksum
    *
    *@param data (in)   pointer to data buffer
    *@param size (in)   size of data buffer
    *
    *@return CRC checksum
    */
    uint32_t calculateChecksum(uint16_t *data, size_t size);

    /**
    *@method initializeSendBuffer
    *
    *@brief initialize ICMP packet once
    *
    *@return CRC checksum
    */
    void initializeSendBuffer();

    /**
    *@method appendTlvCommand
    *
    *@brief append TlvCommand to txBuffer
    *
    *@param commandType (in)    command type
    *
    *@return the appended TLV size
    */
    size_t appendTlvCommand(Command commandType = Command::COMMAND_SWITCH_ACTIVE);

    /**
     *@method uuidToHexString
     *
     *@brief convert uuid to string in hex form
     *
     *@return uuid's string repersentation in hex form
     */
     std::string uuidToHexString(const boost::uuids::uuid& uuid);
 
    /**
     *@method generateGuid
     *
     *@brief generate GUID for each link_prober
     *
     *@return newly generated guid
     */
     std::string generateGuid();

    /**
    *@method appendTlvSentinel
    *
    *@brief append TlvSentinel to txBuffer
    *
    *@return the appended TLV size
    */
    size_t appendTlvSentinel();

    /**
    *@method appendTlvDummy
    *
    *@brief append dummy TLV, test purpose only
    *
    *@return the appended TLV size
    */
    size_t appendTlvDummy(size_t paddingSize, int seqNo);

    /**
    * @method calculateTxPacketChecksum
    * 
    * @brief calculate TX packet checksums in both IP header and ICMP header
    * 
    * @return none
    */
    void calculateTxPacketChecksum();
 
 
     static SockFilter mIcmpFilter[];    
 
     common::MuxPortConfig &mMuxPortConfig;
     boost::asio::io_service &mIoService;
     LinkProberStateMachineBase *mLinkProberStateMachinePtr;
 
     uint16_t mTxSeqNo = 0xffff;
     uint16_t mRxSelfSeqNo = 0;
     uint16_t mRxPeerSeqNo = 0;
 
     uint32_t mIcmpChecksum = 0;
     uint32_t mIpChecksum = 0;
 
     static const size_t mPacketHeaderSize = sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr);
     static const size_t mTlvStartOffset = sizeof(ether_header) + sizeof(iphdr) + sizeof(icmphdr) + sizeof(IcmpPayload);
 
     boost::asio::io_service::strand mStrand;
     boost::asio::posix::stream_descriptor mStream;
     boost::asio::deadline_timer mDeadlineTimer;
     boost::asio::deadline_timer mSuspendTimer;
     boost::asio::deadline_timer mSwitchoverTimer;
     std::shared_ptr<SockFilter> mSockFilterPtr;
     SockFilterProg mSockFilterProg;
 
     std::string mSelfGuid;
     std::string mPeerGuid;
     boost::function<void (HeartbeatType heartbeatType)> mReportHeartbeatReplyReceivedFuncPtr;
     boost::function<void (HeartbeatType heartbeatType)> mReportHeartbeatReplyNotReceivedFuncPtr;
 
 
     int mSocket = 0;
 
     std::size_t mTxPacketSize;
     std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> mTxBuffer;
     std::array<uint8_t, MUX_MAX_ICMP_BUFFER_SIZE> mRxBuffer;
 
     bool mCancelSuspend = false;
     bool mSuspendTx = false;
     bool mShutdownTx = false;
     bool mDecreaseProbingInterval = false;
 
     uint64_t mIcmpUnknownEventCount = 0;
     uint64_t mIcmpPacketCount = 0;
};

} /* namespace link_prober */

#endif /* LINKPROBER_H_ */
