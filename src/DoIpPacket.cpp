#include "DoIpPacket.h"
#include <netinet/in.h>
#include <iostream>
#include "MultiByteType.h"

DoIpPacket::DoIpPacket(const DoIpPacket::ByteOrder byte_order)
    : byte_order_(byte_order),
      protocol_version_(2),
      inv_protocol_version_(~protocol_version_),
      payload_type_(0),
      payload_length_(0) {}

DoIpPacket::~DoIpPacket() {}

void DoIpPacket::SetPayloadLength(PayloadLength payload_length, bool force) {
  PayloadOwner::SetPayloadLength(payload_length, force);
  if (force || (payload_length != this->payload_length_)) {
    this->payload_length_ = payload_length;
  }
}

void DoIpPacket::Hton() {
  if (byte_order_ != kNetWork) {
    payload_type_ = htons(payload_type_);
    payload_length_ = htonl(payload_length_);
    byte_order_ = kNetWork;
  }
}

void DoIpPacket::Ntoh() {
  if (byte_order_ != kHost) {
    payload_type_ = ntohs(payload_type_);
    payload_length_ = ntohl(payload_length_);
    byte_order_ = kHost;
  }
}


DoIpNackCodes DoIpPacket::VerifyPayloadType() {
  printf("[VerifyPayloadType] payload_type_: 0x%04x\n",payload_type_);
  switch (payload_type_) {
    case DoIpPayload::kRoutingActivationRequest: {
      if (payload_length_ != 11) {
        payload_type_ = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kRoutingActivationResponse: {
      if ((payload_length_ != 13) && (payload_length_ != 9)) {
        payload_type_ = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kAliveCheckResponse: {
      if (payload_length_ != 2) {
        payload_type_ = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kVehicleIdentificationRequest: {
      if (payload_length_ != 0) {
        payload_type_ = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kVehicleAnnouncement: {
      if (payload_length_ != 32 && payload_length_ != 33) {
        payload_type_ = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kDiagnosticMessage: {
      if (payload_length_ <= 4) {
        payload_type_ = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kDiagnosticAck: {
      if (payload_length_ != 5) {
        payload_type_ = DoIpPayload::kGenericDoIpNack;
        return DoIpNackCodes::kInvalidPayloadLength;
      }
      break;
    }
    case DoIpPayload::kDiagnosticNack: {
      if (payload_length_ != 5) {
        payload_type_ = DoIpPayload::kGenericDoIpNack;
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
  return std::string(payload_.begin(), payload_.begin() + 17);
}
ByteVector DoIpPacket::GetLogicalAddress() {
  return ByteVector(payload_.begin() + 17, payload_.begin() + 19);
}
ByteVector DoIpPacket::GetEID() {
  return ByteVector(payload_.begin() + 19, payload_.begin() + 25);
}
ByteVector DoIpPacket::GetGID() {
  return ByteVector(payload_.begin() + 25, payload_.begin() + 31);
}
uint8_t DoIpPacket::GetFurtherActionRequied() { return payload_.back(); }

DoIpPacket::ScatterArray DoIpPacket::GetScatterArray() {
  ScatterArray scatter_array;

  scatter_array[kProtocolVersionIdx].iov_base = &protocol_version_;
  scatter_array[kProtocolVersionIdx].iov_len = kDoIp_ProtocolVersion_length;
  scatter_array[kInvProtocolVersionIdx].iov_base = &inv_protocol_version_;
  scatter_array[kInvProtocolVersionIdx].iov_len =
      kDoIp_InvProtocolVersion_length;
  scatter_array[kPayloadTypeIdx].iov_base = &payload_type_;
  scatter_array[kPayloadTypeIdx].iov_len = kDoIp_PayloadType_length;
  scatter_array[kPayloadLengthIdx].iov_base = &payload_length_;
  scatter_array[kPayloadLengthIdx].iov_len = kDoIp_PayloadLength_length;
  scatter_array[kPayloadIdx].iov_base = payload_.data();
  scatter_array[kPayloadIdx].iov_len = payload_.size();

  return scatter_array;
}

void DoIpPacket::ConstructVehicleIdentificationRequest() {
  SetProtocolVersion(DoIpProtocolVersions::kVIDRequest);
  SetPayloadType(DoIpPayload::kVehicleIdentificationRequest);
  SetPayloadLength(0);
}

void DoIpPacket::ConstructRoutingActivationRequest(uint16_t source_address) {
  SetProtocolVersion(DoIpProtocolVersions::kDoIpIsoDis13400_2_2012);
  SetPayloadType(DoIpPayload::kRoutingActivationRequest);
  SetPayloadLength(0x000B);
  payload_.reserve(payload_length_);
  // std::cout << "source address: " << source_address << std::endl;
  payload_.at(0) = GetByte(source_address, 1);
  payload_.at(1) = GetByte(source_address, 0);
  payload_.insert(payload_.begin() + 2, kRoutingActivationRequestData.begin(), kRoutingActivationRequestData.end());
  payload_.erase(payload_.begin() + 11, payload_.end());
  // printf("payload size: %d\n",payload_.size());
  // for (auto x : payload_) {
  //   printf("0x%02x ", x);
  // }
  // printf("\n");
}

void DoIpPacket::ConstructDiagnosticMessage(uint16_t source_address, uint16_t target_address, ByteVector user_data) {
  SetProtocolVersion(DoIpProtocolVersions::kDoIpIsoDis13400_2_2012);
  SetPayloadType(DoIpPayload::kDiagnosticMessage);
  SetPayloadLength(static_cast<uint32_t>(2+2+user_data.size()));
  printf("source address & target address: 0x%04x, 0x%04x\n",source_address, target_address);
  printf("payload_length_: %d\n",payload_length_);
  printf("user_data size: %d\n",user_data.size());
  payload_.reserve(payload_length_);
  payload_.at(0) = GetByte(source_address, 1);
  // printf("1\n");
  payload_.at(1) = GetByte(source_address, 0);
  // printf("2\n");
  payload_.at(2) = GetByte(target_address, 1);
  // printf("3\n");
  payload_.at(3) = GetByte(target_address, 0);
  // printf("4\n");
  payload_.insert(payload_.begin() + 4, user_data.begin(), user_data.end());
  // printf("error: %s\n",std::to_string(errno));
  payload_.erase(payload_.begin() + 4 + user_data.size(), payload_.end());
  printf("6\n");
}



void DoIpPacket::SetPayloadType(uint8_t u_1, uint8_t u_2) {
  PayloadType type;
  type = ((uint16_t)u_1 << 8) + (uint16_t)u_2;

  payload_type_ = type;
    // printf("type: 0x%04x\n",payload_type_);
}

void DoIpPacket::PrintPacketByte(){
  printf("PrintPacketByte: \n");
  ByteVector packet_header;
  packet_header.push_back(protocol_version_);
  packet_header.push_back(inv_protocol_version_);
  packet_header.push_back(GetByte(payload_type_, 1));
  packet_header.push_back(GetByte(payload_type_, 0));
  for (int i = 3; i >= 0; i --) packet_header.push_back(GetByte(payload_length_,i));
  for (auto x : packet_header) {
    printf("0x%02x ",x);
  }
  for (auto x : payload_) {
    printf("0x%02x ",x);
  }
  printf("\n");
}