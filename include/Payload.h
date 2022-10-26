#include <cstddef>
#include <cstdint>
#include <vector>

#ifndef __PAYLOAD_H__
#define __PAYLOAD_H__

typedef std::vector<uint8_t> ByteVector;

template <typename PayloadLengthType>
class PayloadOwner {
 public:
  typedef PayloadLengthType PayloadLength;

  template <typename InputIterator>
  PayloadOwner(InputIterator begin, const InputIterator& end) {
    payload_.insert(payload_.begin(), begin, end);
  }

  explicit PayloadOwner(const ByteVector& payload) {
    payload_.reserve(payload.size());
    payload_.insert(payload_.begin(), payload.begin(), payload.end());
  }
  PayloadOwner() {}

  virtual ~PayloadOwner() {}

  ByteVector payload_;

  inline size_t GetSize() const { return payload_.size(); }

  inline virtual void SetPayloadLength(PayloadLength payload_length,
                                       bool force = false) {
    payload_.resize(payload_length);
  }
};

#endif