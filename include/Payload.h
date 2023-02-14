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
    m_payload.insert(m_payload.begin(), begin, end);
  }

  explicit PayloadOwner(const ByteVector& payload) {
    m_payload.reserve(payload.size());
    m_payload.insert(m_payload.begin(), payload.begin(), payload.end());
  }
  PayloadOwner() {}

  virtual ~PayloadOwner() {}

  ByteVector m_payload;
  /**
   * @brief Get the Size of payload
   * 
   * @return size_t 
   */
  inline size_t GetSize() const { return m_payload.size(); }

  inline virtual void SetPayloadLength(PayloadLength payloadLength,
                                       bool force = false) {
    m_payload.resize(payloadLength);
  }
};

#endif