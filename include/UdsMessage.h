/**
 * @brief 处理负载的用户数据报（uds）
 */

#ifndef __UDS_MESSAGE_H__
#define __UDS_MESSAGE_H__
#include <cstdint>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <tuple>
#include <vector>

#include "MultiByteType.h"
#include "Payload.h"

/** First byte of negative response */
static const uint8_t kUdsNackSid{0x7f};
/** Offset on SID in positive response */
static const uint8_t kUdsResponseSidOffset{0x40};
/** Bitmask to suppress negative response */
static const uint8_t kSuppressPositiveResponseBitMask = 0x80;

class UdsMessage : public PayloadOwner<uint16_t> {
 public:
  typedef uint16_t AddressType;
  typedef std::vector<uint8_t> ByteVector;

 private:
  AddressType m_sourceAddress;
  AddressType m_targetAddress;

 public:
  UdsMessage(const AddressType sourceAddress, const AddressType targetAddress,
             const ByteVector& payload);
  UdsMessage(const AddressType sourceAddress,
             const AddressType targetAddress);

  template <typename InputIterator>
  UdsMessage(const AddressType sourceAddress, const AddressType targetAddress,
             InputIterator begin, const InputIterator& end)
      : PayloadOwner<uint16_t>(begin, end),
        m_sourceAddress(sourceAddress),
        m_targetAddress(targetAddress) {}
  virtual ~UdsMessage();

  /**
   * @brief Get the source address
   *
   * @return AddressType
   */
  inline AddressType GetSa() const { return m_sourceAddress; }

  /**
   * @brief set the target address
   *
   * @param source_address
   */
  inline void SetSa(const AddressType address) { m_sourceAddress = address; }

  /**
   * @brief Get the Target address
   *
   * @return AddressType
   */
  inline AddressType GetTa() const { return m_targetAddress; }

  inline void SetTa(const AddressType address) { m_targetAddress = address; }
  /**
   * @brief Get a bool flag whether message is negative response or not
   *
   * @return true
   * @return false
   */
  inline bool IsNegativeResponseMessage() const {
    return GetSid() == kUdsNackSid;
  }

  /**
   * @brief Get the service id of the uds message.
   *
   * @return uint8_t
   */
  inline uint8_t GetSid() const {
    if (m_payload.size() > 0) {
      return m_payload.at(kUdsPayloadSidOffset);
    } else {
      throw std::out_of_range("UdsMessage Payload has insufficient size.");
    }
  }

  inline uint16_t GetTotalLength() {
    return (sizeof(m_sourceAddress) + sizeof(m_targetAddress) + GetSize());
  }
  /**
   * \brief Create a response message with source & target address.
   * \return The uds response message.
   */
  inline UdsMessage CreateResponse() const {
    UdsMessage response{this->m_targetAddress, this->m_sourceAddress};
    return response;
  }
}; 

std::ostream& operator<<(std::ostream& stream, const UdsMessage& message);
bool operator==(const UdsMessage& left, const UdsMessage& right);

bool EqualUds(ByteVector uds1, ByteVector uds2);
#endif