#ifndef __DO_IP_PACKET_H__
#define __DO_IP_PACKET_H__
#include <stdint.h>

#include <array>

#include "Payload.h"
#include "UdsMessage.h"

enum DoIpProtocolVersions : uint8_t {
  kDoIpIsoDis13400_2_2010 = 0x01,
  kDoIpIsoDis13400_2_2012 = 0x02,
  kDoIpIsoDis13400_2_2019 = 0x03,
  kVIDRequest = 0xFF
};

static const ByteVector kRoutingActivationRequestData = {0x00, 0x00, 0x00, 0x00, 0x00, 0x12, 0x34, 0x56, 0x78};
/**
 * \brief DoIP协议版本
 */
static const uint8_t kSupportedDoIpVersion(
    DoIpProtocolVersions::kDoIpIsoDis13400_2_2012);

/* DoIp Header format information */

/**
 * \brief 协议版本在DoIp头部的位置
 */
static const uint8_t kDoIp_ProtocolVersion_offset{0};

/**
 * \brief 协议版本长度大小
 */
static const uint8_t kDoIp_ProtocolVersion_length{1};

/**
 * \brief 协议版本字节取反后字节在DoIp头部的位置
 */
static const uint8_t kDoIp_InvProtocolVersion_offset{1};

/**
 * \brief 协议版本字节取反后长度大小
 */
static const uint8_t kDoIp_InvProtocolVersion_length{1};

/**
 * \brief 负载类型字节在DoIp头部的位置
 */
static const uint8_t kDoIp_PayloadType_offset{2};

/**
 * \brief 负载类型长度大小
 */
static const uint8_t kDoIp_PayloadType_length{2};

/**
 * \brief 负载长度字节在DoIp头部的位置
 */
static const uint8_t kDoIp_PayloadLength_offset{4};

/**
 * \brief 负载长度大小
 */
static const uint8_t kDoIp_PayloadLength_length{4};

/**
 * \brief DoIp头部的字节数
 */
static const uint8_t kDoIp_HeaderTotal_length{8};

/**
 * \brief Number of fields in the DoIp header
 */
static const uint8_t kDoIp_HeaderTotal_fields{5};

/**
 * \brief Definition of maximum protocol id
 */
static const uint8_t kDoIp_ProtocolVersion_max{0xFFU};

enum GenericDoIpHeader : char {
  kProtocolVersionIdx,
  kInvProtocolVersionIdx,
  kPayloadTypeIdx,
  kPayloadLengthIdx,
  kPayloadIdx
};

enum DoIpAckCodes : char {
  Ack = 0x00,
};

enum DoIpNackCodes : char {
  kIncorrectPatternFormat = 0x00,
  kUnknownPayloadType = 0x01,
  kMessageTooLarge = 0x02,
  kOutOfMemory = 0x30,
  kInvalidPayloadLength = 0x04
};

/**
 * @brief 负载类型的值
 *
 */
enum DoIpPayload : uint16_t {
  kGenericDoIpNack = 0,
  kVehicleIdentificationRequest = 0x0001,
  kVehicleIdentificationRequestWithEid = 0x0002,
  kVehicleIdentificationRequestWithVin = 0x0003,
  kVehicleAnnouncement = 0x0004,
  kRoutingActivationRequest = 0x0005,
  kRoutingActivationResponse = 0x0006,
  kAliveCheckRequest = 0x0007,
  kAliveCheckResponse = 0x0008,

  kDoIpEntityStatusRequest = 0x4001,
  kDoIpEntityStatusResponse = 0x4002,
  kDiagnosticPowerModeInformationRequest = 0x4003,
  kDiagnosticPowerModeInformationResponse = 0x4004,

  kDiagnosticMessage = 0x8001,
  kDiagnosticAck = 0x8002,
  kDiagnosticNack = 0x8003
};

/**
 * \brief Enumerator for DoIp routing activation responses codes.
 */
enum DoIpRoutingActivationResponseCodes : uint8_t {
  /* ISO 13400-2:2012 Table-25*/
  kRoutingActivationDeniedUnknownSa = 0x00,
  kRoutingActivationDeniedAllSocketsRegistered = 0x01,
  kRoutingActivationDeniedSaDifferent = 0x02,
  kRoutingActivationDeniedSaAlreadyRegsiteredAndActive = 0x03,
  kRoutingActivationDeniedMissingAuthentication = 0x04,
  kRoutingActivationDeniedRejectedConfirmation = 0x05,
  kRoutingActivationDeniedUnsupportedRoutingActivationType = 0x06,
  kRoutingActivationSuccessfullyActivated = 0x10,
  kRoutingActivationWillActivatedConfirmationRequired = 0x11
};

class DoIpPacket : public PayloadOwner<uint32_t> {
 public:
  // typedef std::array<struct iovec, kDoIp_HeaderTotal_fields> ScatterArray;
  typedef std::array<struct iovec, kDoIp_HeaderTotal_fields> ScatterArray;
  enum ByteOrder {
    kHost,
    kNetWork,
  };

  ByteOrder byte_order_;

  typedef uint16_t PayloadType;
  typedef uint8_t ProtocolVersion;
  typedef uint16_t AddressType;

 protected:
  ProtocolVersion protocol_version_;
  ProtocolVersion inv_protocol_version_;

 public:
  PayloadType payload_type_;
  PayloadLength payload_length_;

  explicit DoIpPacket(const ByteOrder byte_order_);
  virtual ~DoIpPacket();

  void SetPayloadLength(PayloadLength payload_length,
                        bool force = false) override;

  inline void SetProtocolVersion(ProtocolVersion prot_version) {
    protocol_version_ = prot_version;
    inv_protocol_version_ = ~prot_version;
  }
  inline ProtocolVersion GetProtocolVersion(void) {
    return (protocol_version_);
  }
  inline ProtocolVersion GetInverseProtocolVersion(void) {
    return (protocol_version_);
  }

  void SetPayloadType(PayloadType type) { payload_type_ = type; }
  void SetPayloadType(uint8_t u_1, uint8_t u_2) ;
  /**
   * @brief 本地字节序(小端)转为网络字节序(大端)
   *
   */
  void Hton();

  /**
   * @brief 网络字节序(大端)转为本地字节序(小端)
   *
   */
  void Ntoh();

  uint8_t VerifyPayloadType();
  std::string GetVIN();
  ByteVector GetLogicalAddress();
  ByteVector GetEID();
  ByteVector GetGID();
  uint8_t GetFurtherActionRequied();
  void ConstructVehicleIdentificationRequest();
  void ConstructRoutingActivationRequest(uint16_t source_address);
  void ConstructDiagnosticMessage(uint16_t source_address, uint16_t target_address, ByteVector user_data);
  ScatterArray GetScatterArray();
  void PrintPacketByte();
};

#endif