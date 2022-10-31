#include <cstddef>
#include <cstdint>
#include <vector>

#ifndef __PAYLOAD_H__
#define __PAYLOAD_H__

typedef std::vector<uint8_t> ByteVector;
enum UdsPayloadParamOffsets : uint8_t { kUdsPayloadSidOffset = 0, kUdsPayloadSubFuncOffset = 1 };

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
  /**
   * @brief Get the Size of payload
   * 
   * @return size_t 
   */
  inline size_t GetSize() const { return payload_.size(); }

  inline virtual void SetPayloadLength(PayloadLength payload_length,
                                       bool force = false) {
    payload_.resize(payload_length);
  }
};

#endif