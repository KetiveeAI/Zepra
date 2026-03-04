/**
 * @file WebRTCAPI.h
 * @brief WebRTC API stubs for ZepraScript
 * 
 * Basic type definitions for future WebRTC implementation.
 * These are stubs that define the interface but not full functionality.
 */

#pragma once

#include "../config.hpp"
#include "runtime/objects/value.hpp"
#include "runtime/async/promise.hpp"
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>

namespace Zepra::Browser {

using Runtime::Value;
using Runtime::Object;

// =============================================================================
// RTCIceCandidate
// =============================================================================

/**
 * @brief ICE candidate for connection establishment
 */
class RTCIceCandidate {
public:
    RTCIceCandidate() = default;
    
    std::string candidate;           // SDP candidate string
    std::string sdpMid;              // Media stream ID
    uint32_t sdpMLineIndex = 0;      // Index in SDP
    std::string usernameFragment;    // ICE username fragment
    
    // ICE candidate component
    enum class Component { RTP = 1, RTCP = 2 };
    std::optional<Component> component;
    
    // Protocol
    enum class Protocol { UDP, TCP };
    std::optional<Protocol> protocol_;
    
    std::string toJSON() const;
    static RTCIceCandidate fromJSON(const std::string& json);
};

// =============================================================================
// RTCSessionDescription
// =============================================================================

/**
 * @brief SDP session description
 */
class RTCSessionDescription {
public:
    enum class Type { Offer, Answer, Provisional, Rollback };
    
    RTCSessionDescription() = default;
    RTCSessionDescription(Type type, const std::string& sdp)
        : type(type), sdp(sdp) {}
    
    Type type = Type::Offer;
    std::string sdp;
    
    std::string toJSON() const;
    static RTCSessionDescription fromJSON(const std::string& json);
};

// =============================================================================
// RTCConfiguration
// =============================================================================

/**
 * @brief Configuration for RTCPeerConnection
 */
struct RTCConfiguration {
    struct IceServer {
        std::vector<std::string> urls;
        std::string username;
        std::string credential;
    };
    
    std::vector<IceServer> iceServers;
    
    enum class IceTransportPolicy { Relay, All };
    IceTransportPolicy iceTransportPolicy = IceTransportPolicy::All;
    
    enum class BundlePolicy { Balanced, MaxBundle, MaxCompat };
    BundlePolicy bundlePolicy = BundlePolicy::Balanced;
    
    enum class RtcpMuxPolicy { Require };
    RtcpMuxPolicy rtcpMuxPolicy = RtcpMuxPolicy::Require;
};

// =============================================================================
// RTCDataChannelInit
// =============================================================================

/**
 * @brief Options for creating a data channel
 */
struct RTCDataChannelInit {
    bool ordered = true;
    std::optional<uint16_t> maxPacketLifeTime;
    std::optional<uint16_t> maxRetransmits;
    std::string protocol;
    bool negotiated = false;
    std::optional<uint16_t> id;
};

// =============================================================================
// RTCDataChannel
// =============================================================================

/**
 * @brief Bidirectional data channel
 */
class RTCDataChannel : public Object {
public:
    RTCDataChannel(const std::string& label, const RTCDataChannelInit& options);
    ~RTCDataChannel();
    
    // Properties
    std::string label() const { return label_; }
    bool ordered() const { return ordered_; }
    std::optional<uint16_t> maxPacketLifeTime() const { return maxPacketLifeTime_; }
    std::optional<uint16_t> maxRetransmits() const { return maxRetransmits_; }
    std::string protocol() const { return protocol_; }
    bool negotiated() const { return negotiated_; }
    std::optional<uint16_t> id() const { return id_; }
    
    enum class ReadyState { Connecting, Open, Closing, Closed };
    ReadyState readyState() const { return readyState_; }
    
    size_t bufferedAmount() const { return bufferedAmount_; }
    size_t bufferedAmountLowThreshold() const { return bufferedAmountLowThreshold_; }
    void setBufferedAmountLowThreshold(size_t value) { bufferedAmountLowThreshold_ = value; }
    
    // Binary type
    enum class BinaryType { Blob, ArrayBuffer };
    BinaryType binaryType() const { return binaryType_; }
    void setBinaryType(BinaryType type) { binaryType_ = type; }
    
    // Methods
    void send(const std::string& data);
    void send(const std::vector<uint8_t>& data);
    void close();
    
    // Event handlers
    using Handler = std::function<void()>;
    using MessageHandler = std::function<void(const Value&)>;
    using ErrorHandler = std::function<void(const std::string&)>;
    
    void setOnOpen(Handler h) { onOpen_ = std::move(h); }
    void setOnMessage(MessageHandler h) { onMessage_ = std::move(h); }
    void setOnError(ErrorHandler h) { onError_ = std::move(h); }
    void setOnClose(Handler h) { onClose_ = std::move(h); }
    void setOnBufferedAmountLow(Handler h) { onBufferedAmountLow_ = std::move(h); }
    
private:
    std::string label_;
    bool ordered_ = true;
    std::optional<uint16_t> maxPacketLifeTime_;
    std::optional<uint16_t> maxRetransmits_;
    std::string protocol_;
    bool negotiated_ = false;
    std::optional<uint16_t> id_;
    ReadyState readyState_ = ReadyState::Connecting;
    size_t bufferedAmount_ = 0;
    size_t bufferedAmountLowThreshold_ = 0;
    BinaryType binaryType_ = BinaryType::ArrayBuffer;
    
    Handler onOpen_;
    MessageHandler onMessage_;
    ErrorHandler onError_;
    Handler onClose_;
    Handler onBufferedAmountLow_;
};

// =============================================================================
// RTCPeerConnection
// =============================================================================

/**
 * @brief WebRTC peer connection
 */
class RTCPeerConnection : public Object {
public:
    explicit RTCPeerConnection(const RTCConfiguration& config = {});
    ~RTCPeerConnection();
    
    // Connection state
    enum class ConnectionState {
        New, Connecting, Connected, Disconnected, Failed, Closed
    };
    
    enum class IceConnectionState {
        New, Checking, Connected, Completed, Disconnected, Failed, Closed
    };
    
    enum class IceGatheringState {
        New, Gathering, Complete
    };
    
    enum class SignalingState {
        Stable, HaveLocalOffer, HaveRemoteOffer,
        HaveLocalPranswer, HaveRemotePranswer, Closed
    };
    
    ConnectionState connectionState() const { return connectionState_; }
    IceConnectionState iceConnectionState() const { return iceConnectionState_; }
    IceGatheringState iceGatheringState() const { return iceGatheringState_; }
    SignalingState signalingState() const { return signalingState_; }
    
    // Local/Remote descriptions
    RTCSessionDescription* localDescription() const { return localDescription_.get(); }
    RTCSessionDescription* remoteDescription() const { return remoteDescription_.get(); }
    RTCSessionDescription* currentLocalDescription() const { return currentLocalDescription_.get(); }
    RTCSessionDescription* currentRemoteDescription() const { return currentRemoteDescription_.get(); }
    RTCSessionDescription* pendingLocalDescription() const { return pendingLocalDescription_.get(); }
    RTCSessionDescription* pendingRemoteDescription() const { return pendingRemoteDescription_.get(); }
    
    // ==========================================================================
    // Methods (Stub implementations)
    // ==========================================================================
    
    /**
     * @brief Create an offer SDP
     */
    void createOffer(std::function<void(RTCSessionDescription*)> success,
                     std::function<void(const std::string&)> failure);
    
    /**
     * @brief Create an answer SDP
     */
    void createAnswer(std::function<void(RTCSessionDescription*)> success,
                      std::function<void(const std::string&)> failure);
    
    /**
     * @brief Set local description
     */
    void setLocalDescription(RTCSessionDescription* desc,
                             std::function<void()> success,
                             std::function<void(const std::string&)> failure);
    
    /**
     * @brief Set remote description
     */
    void setRemoteDescription(RTCSessionDescription* desc,
                              std::function<void()> success,
                              std::function<void(const std::string&)> failure);
    
    /**
     * @brief Add ICE candidate
     */
    void addIceCandidate(const RTCIceCandidate& candidate,
                         std::function<void()> success,
                         std::function<void(const std::string&)> failure);
    
    /**
     * @brief Create a data channel
     */
    RTCDataChannel* createDataChannel(const std::string& label,
                                      const RTCDataChannelInit& options = {});
    
    /**
     * @brief Close the connection
     */
    void close();
    
    // Event handlers
    using StateHandler = std::function<void()>;
    using IceCandidateHandler = std::function<void(const RTCIceCandidate&)>;
    using DataChannelHandler = std::function<void(RTCDataChannel*)>;
    
    void setOnConnectionStateChange(StateHandler h) { onConnectionStateChange_ = std::move(h); }
    void setOnIceConnectionStateChange(StateHandler h) { onIceConnectionStateChange_ = std::move(h); }
    void setOnIceGatheringStateChange(StateHandler h) { onIceGatheringStateChange_ = std::move(h); }
    void setOnSignalingStateChange(StateHandler h) { onSignalingStateChange_ = std::move(h); }
    void setOnIceCandidate(IceCandidateHandler h) { onIceCandidate_ = std::move(h); }
    void setOnDataChannel(DataChannelHandler h) { onDataChannel_ = std::move(h); }
    
private:
    RTCConfiguration config_;
    
    ConnectionState connectionState_ = ConnectionState::New;
    IceConnectionState iceConnectionState_ = IceConnectionState::New;
    IceGatheringState iceGatheringState_ = IceGatheringState::New;
    SignalingState signalingState_ = SignalingState::Stable;
    
    std::unique_ptr<RTCSessionDescription> localDescription_;
    std::unique_ptr<RTCSessionDescription> remoteDescription_;
    std::unique_ptr<RTCSessionDescription> currentLocalDescription_;
    std::unique_ptr<RTCSessionDescription> currentRemoteDescription_;
    std::unique_ptr<RTCSessionDescription> pendingLocalDescription_;
    std::unique_ptr<RTCSessionDescription> pendingRemoteDescription_;
    
    std::vector<std::unique_ptr<RTCDataChannel>> dataChannels_;
    
    StateHandler onConnectionStateChange_;
    StateHandler onIceConnectionStateChange_;
    StateHandler onIceGatheringStateChange_;
    StateHandler onSignalingStateChange_;
    IceCandidateHandler onIceCandidate_;
    DataChannelHandler onDataChannel_;
};

// =============================================================================
// Builtin Functions
// =============================================================================

void initWebRTC();

} // namespace Zepra::Browser
