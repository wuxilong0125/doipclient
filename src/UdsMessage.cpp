#include "UdsMessage.h"

#include <algorithm>

UdsMessage::UdsMessage(const AddressType sourceAddress,
                       const AddressType targetAddress,
                       const ByteVector& payload)
    : PayloadOwner<uint16_t>(payload),
      m_sourceAddress(sourceAddress),
      m_targetAddress(targetAddress) {}

UdsMessage::UdsMessage(const AddressType sourceAddress,
                       const AddressType targetAddress)
    : m_sourceAddress(sourceAddress), m_targetAddress(targetAddress) {}

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
  std::copy(message.m_payload.begin(), message.m_payload.end(), oit);

  stream.flags(old_flags);
  stream.width(old_width);
  return stream;
}

bool operator==(const UdsMessage& left, const UdsMessage& right) {
  if ((left.GetSa() == right.GetSa()) && (left.GetTa() == right.GetTa()) &&
      (left.m_payload.size() == right.m_payload.size())) {
    ByteVector::const_iterator leftIt(left.m_payload.begin());
    ByteVector::const_iterator rightIt(right.m_payload.begin());

    while (leftIt != left.m_payload.end() && rightIt != right.m_payload.end()) {
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