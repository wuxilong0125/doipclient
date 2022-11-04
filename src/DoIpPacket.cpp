#include "DoIpPacket.h"

#include <netinet/in.h>

DoIpPacket::DoIpPacket(const DoIpPacket::ByteOrder byte_order)
    : byte_order_(byte_order),
      protocol_version_(2),
      inv_protocol_version_(~protocol_version_),
      payload_type_(0),
      payload_length_(0) {
}

DoIpPacket::~DoIpPacket() {
}

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

void DoIpPacket::setPayloadType(PayloadType type) { payload_type_ = type; }