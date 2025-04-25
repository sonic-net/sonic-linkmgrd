#ifndef LINK_PROBER_LINKPROBERHW_H_
#define LINK_PROBER_LINKPROBERHW_H_

#include "LinkProberBase.h"

namespace test {
class LinkProberTest;
class LinkProberMockTest;
}

namespace mux {
    class MuxPort;
}

namespace link_prober
{


class LinkProberHw : public LinkProberBase
{
public:
    /**
    *@method LinkProberHw
    *
    *@brief class default constructor
    */
    LinkProberHw() = delete;

    /**
    *@method LinkProberHw
    *
    *@brief class copy constructor
    *
    *@param LinkProberHw (in)  reference to LinkProber object to be copied
    */
    LinkProberHw(const LinkProberHw &) = delete;

    /**
    *@method LinkProberHw
    *
    *@brief class constructor
    *
    *@param muxPortConfig (in)          reference to MuxPortConfig object
    *@param ioService (in)              reference to boost io_service object
    *@param linkProberStateMachinePtr (in) reference to LinkProberStateMachineBase object
    *@param muxPort                     pointer to MuxPortObject
    */
    LinkProberHw(
        common::MuxPortConfig &muxPortConfig,
        boost::asio::io_service &ioService,
        LinkProberStateMachineBase *linkProberStateMachinePtr,
        mux::MuxPort *muxPort
    );

    /**
    *@method initialize
    *
    *@brief handle state change notification from STATE_DB:ICMP_ECHO_SESSION_TABLE
    *
    *
    *@return none
    */
    virtual void initialize() final override;

    /**
    *@method startProbing
    *
    *@brief triggers creation of ICMP_ECHO_SESSION_TABLE in APP_DB table to start sending/recieving of ICMP session packets
    *
    *@return none
    */
    virtual void startProbing() final override;

    /**
    *@method suspendTxProbes
    *
    *@brief suspend sending ICMP ECHOREQUEST packets
    *
    *@param suspendTime_msec suspension time in msec
    *
    *@return none
    */
    virtual void suspendTxProbes(uint32_t suspendTime_msec) final override;

    /**
    *@method shutdownTxProbes
    *
    *@brief stop sending ICMP ECHOREQUEST packets
    *
    *@return none
    */
    virtual void shutdownTxProbes() final override;

    /**
    * @method restartTxProbes
    * 
    * @brief restart sending ICMP ECHOREQUEST packets
    * 
    * @return none
    */
    virtual void restartTxProbes() final override;

    /**
    *@method resumeTxProbes
    *
    *@brief resume sending ICMP ECHOREQUEST packets
    *
    *@return none
    */
    virtual void resumeTxProbes() final override;

    /**
    * @method handleStateDbStateUpdate
    * 
    * @brief handle state change of ICMP_ECHO_SESSION_TABLE in STATE_DB
    * 
    * @return none
    */
    virtual void handleStateDbStateUpdate(const std::string& state, const std::string torType) final override;

    /**
    *@method updateEthernetFrame
    *
    *@brief update Ethernet frame of Tx Buffer
    *
    *@return none
    */
    virtual void updateEthernetFrame() final override;
    
    /**
    *@method ~LinkProber
    *
    *@brief class destructor
    */
    virtual ~LinkProberHw() = default;

private:
    /**
     *@method startPositiveProbingTimer
     *
     *@brief triggers handling of expiration of positive probing timer 
     *
     *@param torType (in)  session tor type
     *
     *@return none
     */
    void startPositiveProbingTimer(std::string torType);

    /**
     *@method handlePositiveProbingTimeout
     *
     *@brief bring Link prober state machine to active when positive probing timer expires
     *
     *@param torType (in)  session tor type
     *
     *@return none
     */
    void handlePositiveProbingTimeout(std::string torType);

    /**
     *@method handleSuspendTimeout
     *
     *@brief handles suspendTimer expires start probing again 
     *
     *@param errorCode (in)  boost system error_code during suspend
     *
     *@return none
     */
    void handleSuspendTimeout(boost::system::error_code errorCode);

    /**
     *@method createIcmpEchoSession
     *
     *@brief intializing reciving thread and triggers creation of new icmp_echo session
     *
     *@param torType (in)  session tor type
     *@param guid (in)  global uuid to be used for session 
     *
     *@return none
     */
    void createIcmpEchoSession(std::string torType, std::string guid);

    /**
     *@method deleteIcmpEchoSession
     *
     *@brief intializing reciving thread and triggers creation of new icmp_echo session
     *
     *@param torType (in)  session tor type
     *@param guid (in)  global uuid to be used for session
     *
     *@return none
     */
    void deleteIcmpEchoSession(std::string torType, std::string guid);

    /**
     *@method reportHeartbeatReplyNotReceivedActiveActive
     *
     *@brief first time recieve a new type guid reply packet
     *
     *@param heartbeatType (in)  session tor type
     *
     *@return none
     */
    void reportHeartbeatReplyNotReceivedActiveActive(HeartbeatType heartbeatType);

    /**
     *@method reportHeartbeatReplyReceivedActiveActive
     *
     *@brief first time recieve a new type guid reply packet
     *
     *@param heartbeatType (in)  session tor type
     *
     *@return none
     */
    void reportHeartbeatReplyReceivedActiveActive(HeartbeatType heartbeatType);

    /**
     *@method etherMacArrayToString
     *
     *@brief process LinkProberState suspend timer expiry event
     *
     *@param macAddress (in)  reference to macAdress in ether array format
     *
     *@return helper to convert ether array structure of mac address to string format
     */
    std::string etherMacArrayToString(const std::array<uint8_t, 6>& macAddress);

    mux::MuxPort *mMuxPortPtr;
    boost::asio::deadline_timer mSuspendTimer;
    boost::asio::deadline_timer mPositiveProbingTimer;
    boost::asio::deadline_timer mPositiveProbingPeerTimer;

    static std::string mIcmpTableName;
    static std::string mSessionCookie;
    static std::string mDefaultVrfName;
    static std::string mSessionTypeSelf;
    static std::string mSessionTypePeer;
    static std::string mKeySeparator;
    static std::string mUpState;
    static std::string mDownState;
};
}

#endif
