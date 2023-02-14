#include "DoIpPacket.h"
#include <netinet/in.h>
#include <iostream>
#include "MultiByteType.h"

DoIpPacket::DoIpPacket(const DoIpPacket::ByteOrder byte_order)
    : m_byteOrder(byte_order),
      m_protocolVersion(2),
      m_invProtocolVersion(~m_protocolVersion),
      m_payloadType(0),
      m_payloadLength(0) {}

DoIpPacket::~DoIpPacket() {}

void DoIpPacket::SetPayloadLength(PayloadLength payloadLength, bool force) {
  PayloadOwner::SetPayloadLength(payloadLength, force);
  if (force || (payloadLength != this->m_payloadLength)) {
    this->m_payloadLength = payloadLength;
  }
}

void DoIpPacket::Hton() {
  if (m_byteOrder != kNetWork) {
    m_payloadType = htons(m_payloadType);
    m_payloadLength = htonl(m_payloadLength);
    m_byteOrder = kNetWork;
  }
}

void DoIpPacket::Ntoh() {
  if (m_byteOrder != kHost) {
    m_payloadType = ntohs(m_payloadType);
    m_payloadLength = ntohl(m_payloadLength);
    m_byteOrder = kHost;
  }
}


DoIpNackCodes DoIpPacket::VerifyPayloadType() {
  printf("[VerifyPayloadType] m_payloadType: 0x%04x\n",m_payloadType);
  switch (m_payloadType) {
    case DoIpPayload::kRoutingActivationRequest: {
      if (m_payloadLength != 11) {
        m_payloadType = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kRoutingActivationResponse: {
      if ((m_payloadLength != 13) && (m_payloadLength != 9)) {
        m_payloadType = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kAliveCheckResponse: {
      if (m_payloadLength != 2) {
        m_payloadType = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kVehicleIdentificationRequest: {
      if (m_payloadLength != 0) {
        m_payloadType = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kVehicleAnnouncement: {
      if (m_payloadLength != 32 && m_payloadLength != 33) {
        m_payloadType = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kDiagnosticMessage: {
      if (m_payloadLength <= 4) {
        m_payloadType = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kDiagnosticAck: {
      if (m_payloadLength != 5) {
        m_payloadType = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kDiagnosticNack: {
      if (m_payloadLength != 5) {
        m_payloadType = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }

    default:
      break;
  }
  return DoIpNackCodes::kNoError;
}

std::string DoIpPacket::GetVIN() {
  return std::string(m_payload.begin(), m_payload.begin() + 17);
}
ByteVector DoIpPacket::GetLogicalAddress() {
  return ByteVector(m_payload.begin() + 17, m_payload.begin() + 19);
}
ByteVector DoIpPacket::GetEID() {
  return ByteVector(m_payload.begin() + 19, m_payload.begin() + 25);
}
ByteVector DoIpPacket::GetGID() {
  return ByteVector(m_payload.begin() + 25, m_payload.begin() + 31);
}
uint8_t DoIpPacket::GetFurtherActionRequied() { return m_payload.back(); }

DoIpPacket::ScatterArray DoIpPacket::GetScatterArray() {
  ScatterArray scatter_array;

  scatter_array[kProtocolVersionIdx].iov_base = &m_protocolVersion;
  scatter_array[kProtocolVersionIdx].iov_len = kDoIpProtocolVersionLength;
  scatter_array[kInvProtocolVersionIdx].iov_base = &m_invProtocolVersion;
  scatter_array[kInvProtocolVersionIdx].iov_len =
      kDoIpInvProtocolVersionLength;
  scatter_array[kPayloadTypeIdx].iov_base = &m_payloadType;
  scatter_array[kPayloadTypeIdx].iov_len = kDoIpPayloadTypeLength;
  scatter_array[kPayloadLengthIdx].iov_base = &m_payloadLength;
  scatter_array[kPayloadLengthIdx].iov_len = kDoIpPayloadLength;
  scatter_array[kPayloadIdx].iov_base = m_payload.data();
  scatter_array[kPayloadIdx].iov_len = m_payload.size();

  return scatter_array;
}

void DoIpPacket::ConstructVehicleIdentificationRequest() {
  SetProtocolVersion(DoIpProtocolVersions::kVIDRequest);
  SetPayloadType(DoIpPayload::kVehicleIdentificationRequest);
  SetPayloadLength(0);
}

void DoIpPacket::ConstructRoutingActivationRequest(uint16_t sourceAddress) {
  SetProtocolVersion(DoIpProtocolVersions::kDoIpIsoDis13400_2_2012);
  SetPayloadType(DoIpPayload::kRoutingActivationRequest);
  SetPayloadLength(0x000B);
  m_payload.reserve(m_payloadLength);
  // std::cout << "source address: " << source_address << std::endl;
  m_payload.at(0) = GetByte(sourceAddress, 1);
  m_payload.at(1) = GetByte(sourceAddress, 0);
  m_payload.insert(m_payload.begin() + 2, kRoutingActivationRequestData.begin(), kRoutingActivationRequestData.end());
  m_payload.erase(m_payload.begin() + 11, m_payload.end());
  // printf("payload size: %d\n",m_payload.size());
  // for (auto x : m_payload) {
  //   printf("0x%02x ", x);
  // }
  // printf("\n");
}

void DoIpPacket::ConstructDiagnosticMessage(uint16_t sourceAddress, uint16_t targetAddress, ByteVector userData) {
  SetProtocolVersion(DoIpProtocolVersions::kDoIpIsoDis13400_2_2012);
  SetPayloadType(DoIpPayload::kDiagnosticMessage);
  SetPayloadLength(static_cast<uint32_t>(2+2+userData.size()));
  // printf("source address & target address: 0x%04x, 0x%04x\n",source_address, target_address);
  // printf("m_payloadLength: %d\n",m_payloadLength);
  // printf("user_data size: %d\n",user_data.size());
  m_payload.reserve(m_payloadLength);
  m_payload.at(0) = GetByte(sourceAddress, 1);
  m_payload.at(1) = GetByte(sourceAddress, 0);
  m_payload.at(2) = GetByte(targetAddress, 1);
  m_payload.at(3) = GetByte(targetAddress, 0);
  m_payload.insert(m_payload.begin() + 4, userData.begin(), userData.end());
  m_payload.erase(m_payload.begin() + 4 + userData.size(), m_payload.end());
}


void DoIpPacket::ConstructAliveCheckRequest() {
  SetProtocolVersion(DoIpProtocolVersions::kDoIpIsoDis13400_2_2012);
  SetPayloadType(DoIpPayload::kAliveCheckRequest);
  SetPayloadLength(0);
}

void DoIpPacket::SetPayloadType(uint8_t uFirst, uint8_t uSecond) {
  PayloadType type;
  type = ((uint16_t)uFirst << 8) + (uint16_t)uSecond;

  m_payloadType = type;
    // printf("type: 0x%04x\n",m_payloadType);
}

void DoIpPacket::PrintPacketByte(){
  printf("PrintPacketByte: \n");
  ByteVector packetHeader;
  packetHeader.push_back(m_protocolVersion);
  packetHeader.push_back(m_invProtocolVersion);
  packetHeader.push_back(GetByte(m_payloadType, 1));
  packetHeader.push_back(GetByte(m_payloadType, 0));
  for (int i = 3; i >= 0; i --) packetHeader.push_back(GetByte(m_payloadLength,i));
  for (auto x : packetHeader) {
    printf("0x%02x ",x);
  }
  for (auto x : m_payload) {
    printf("0x%02x ",x);
  }
  printf("\n");
}