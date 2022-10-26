#ifndef __DO_IP_PACKET_H__
#define __DO_IP_PACKET_H__
#include <stdint.h>

#include "Payload.h"

enum DoIpProtocolVersions : uint8_t {
  kDoIpIsoDis13400_2_2010 = 0x01,
  kDoIpIsoDis13400_2_2012 = 0x02
};

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

class DoIpPacket : public PayloadOwner<uint32_t> {
 public:
  enum ByteOrder {
    kHost,
    kNetWork,
  };

  ByteOrder byte_order_;

  typedef int16_t PayloadType;
  typedef uint8_t ProtocolVersion;

 protected:
  ProtocolVersion protocol_version_;
  ProtocolVersion inv_protocol_version_;

 public:
  PayloadType payload_type_;
  PayloadType payload_length_;

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

  void setPayloadType(PayloadType type);
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
};

#endif