#ifndef __DO_IP_PACKET_H__
#define __DO_IP_PACKET_H__
#include <stdint.h>

#include <array>

#include "PayloadOwner.h"
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
static const uint8_t kDoIpProtocolVersionOffset{0};

/**
 * \brief 协议版本长度大小
 */
static const uint8_t kDoIpProtocolVersionLength{1};

/**
 * \brief 协议版本字节取反后字节在DoIp头部的位置
 */
static const uint8_t kDoIpInvProtocolVersionOffset{1};

/**
 * \brief 协议版本字节取反后长度大小
 */
static const uint8_t kDoIpInvProtocolVersionLength{1};

/**
 * \brief 负载类型字节在DoIp头部的位置
 */
static const uint8_t kDoIpPayloadTypeOffset{2};

/**
 * \brief 负载类型长度大小
 */
static const uint8_t kDoIpPayloadTypeLength{2};

/**
 * \brief 负载长度字节在DoIp头部的位置
 */
static const uint8_t kDoIpPayloadLengthOffset{4};

/**
 * \brief 负载长度大小
 */
static const uint8_t kDoIpPayloadLength{4};

/**
 * \brief DoIp头部的字节数
 */
static const uint8_t kDoIpHeaderTotalLength{8};

/**
 * \brief Number of fields in the DoIp header
 */
static const uint8_t kDoIpHeaderTotalFields{5};

/**
 * \brief Definition of maximum protocol id
 */
static const uint8_t kDoIpProtocolVersionMax{0xFFU};

enum GenericDoIpHeader : uint8_t {
  kProtocolVersionIdx,
  kInvProtocolVersionIdx,
  kPayloadTypeIdx,
  kPayloadLengthIdx,
  kPayloadIdx
};

enum DoIpAckCodes : uint8_t {
  Ack = 0x00,
};

enum DoIpNackCodes : uint8_t {
  kIncorrectPatternFormat = 0x00,
  kUnknownPayloadType = 0x01,
  kMessageTooLarge = 0x02,
  kOutOfMemory = 0x30,
  kInvalidPayloadLength = 0x04,
  kNoError = 0xff
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

class DoIPPacket : public PayloadOwner<uint32_t> {
 public:

  typedef std::array<struct iovec, kDoIpHeaderTotalFields> ScatterArray;
  enum ByteOrder {
    kHost,
    kNetWork,
  };

  ByteOrder m_byteOrder;

  typedef uint16_t PayloadType;
  typedef uint8_t ProtocolVersion;
  typedef uint16_t AddressType;

 protected:
  ProtocolVersion m_protocolVersion;
  ProtocolVersion m_invProtocolVersion;

 public:
  PayloadType m_payloadType;
  PayloadLength m_payloadLength;

  explicit DoIPPacket(const ByteOrder byteOrder);
  virtual ~DoIPPacket();


  /**
   * @brief 设置负载长度
   */
  void SetPayloadLength(PayloadLength payloadLength,
                        bool force = false) override;
  /**
   * @brief 设置协议版本 
   */
  inline void SetProtocolVersion(ProtocolVersion protVersion) {
    m_protocolVersion = protVersion;
    m_invProtocolVersion = ~protVersion;
  }
  /**
   * @brief 获取协议版本
   */
  inline ProtocolVersion GetProtocolVersion() {
    return (m_protocolVersion);
  }
  /**
   * @brief 获取协议版本的反码
   */
  inline ProtocolVersion GetInverseProtocolVersion() {
    return (m_invProtocolVersion);
  }
  /**
   * @brief 设置负载类型
   */
  void SetPayloadType(PayloadType type) { m_payloadType = type; }
  void SetPayloadType(uint8_t uFirst, uint8_t uSecond) ;
  /**
   * @brief 本地字节序(小端)转为网络字节序(大端)
   */
  void Hton();
  /**
   * @brief 网络字节序(大端)转为本地字节序(小端)
   */
  void Ntoh();
  /**
   * @brief 验证负载类型 
   */
  DoIpNackCodes VerifyPayloadType();
  std::string GetVIN();
  ByteVector GetLogicalAddress();
  ByteVector GetEID();
  ByteVector GetGID();
  uint8_t GetFurtherActionRequied();
  /**
   * @brief 生成车辆识别请求数据报
   */
  void ConstructVehicleIdentificationRequest();
  /**
   * @brief 生成路由激活请求
   */
  void ConstructRoutingActivationRequest(uint16_t sourceAddress);
  /**
   * @brief 生成诊断数据报
   */
  void ConstructDiagnosticMessage(uint16_t sourceAddress, uint16_t targetAddress, ByteVector userData);

  void ConstructAliveCheckRequest();

  ScatterArray GetScatterArray();
  void PrintPacketByte();
};

#endif