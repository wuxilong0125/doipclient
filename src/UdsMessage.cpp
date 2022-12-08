#include "UdsMessage.h"

#include <algorithm>

UdsMessage::UdsMessage(const AddressType source_address,
                       const AddressType target_address,
                       const ByteVector& payload)
    : PayloadOwner<uint16_t>(payload),
      source_address_(source_address),
      target_address_(target_address) {}

UdsMessage::UdsMessage(const AddressType sourceAddress,
                       const AddressType targetAddress)
    : source_address_(sourceAddress), target_address_(targetAddress) {}

UdsMessage::~UdsMessage() {}

/**
 * Definition of the stream operator for UdsMessage objects
 */
std::ostream& operator<<(std::ostream& stream, const UdsMessage& message) {
  std::ios_base::fmtflags old_flags{stream.flags()};
  std::ios_base::fmtflags new_flags{old_flags | std::ios::right |
                                    std::ios::hex | std::ios::showbase};
  stream.flags(new_flags);
  std::streamsize old_width{stream.width()};

  stream << std::hex << message.GetSa() << ", " << message.GetTa() << ", ";
  std::ostream_iterator<int> oit(stream, ", ");
  std::copy(message.payload_.begin(), message.payload_.end(), oit);

  stream.flags(old_flags);
  stream.width(old_width);
  return stream;
}

bool operator==(const UdsMessage& left, const UdsMessage& right) {
  if ((left.GetSa() == right.GetSa()) && (left.GetTa() == right.GetTa()) &&
      (left.payload_.size() == right.payload_.size())) {
    ByteVector::const_iterator leftIt(left.payload_.begin());
    ByteVector::const_iterator rightIt(right.payload_.begin());

    while (leftIt != left.payload_.end() && rightIt != right.payload_.end()) {
      if (*leftIt != *rightIt) {
        return false;
      }
      ++leftIt;
      ++rightIt;
    }
    return true;
  } else {
    return false;
  }
}

bool EqualUds(ByteVector uds1, ByteVector uds2) {
  if (uds1.size() != uds2.size()) return false;
  for (int i = 0; i < uds1.size(); i ++) {
    if (uds1.at(i) != uds2.at(i)) return false;
  }
  return true;
}