/**
 * @brief 处理负载的用户数据报（usd）
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
  AddressType source_address_;
  AddressType target_address_;

 public:
  UdsMessage(const AddressType source_address, const AddressType target_address,
             const ByteVector& payload);
  UdsMessage(const AddressType source_address,
             const AddressType target_address);

  template <typename InputIterator>
  UdsMessage(const AddressType source_address, const AddressType target_address,
             InputIterator begin, const InputIterator& end)
      : PayloadOwner<uint16_t>(begin, end),
        source_address_(source_address),
        target_address_(target_address) {}
  virtual ~UdsMessage();

  /**
   * @brief Get the source address
   *
   * @return AddressType
   */
  inline AddressType GetSa() const { return source_address_; }

  /**
   * @brief set the target address
   *
   * @param source_address
   */
  inline void SetSa(const AddressType source_address) {
    target_address_ = source_address;
  }

  /**
   * @brief Get the Target address
   *
   * @return AddressType
   */
  inline AddressType GetTa() const { return target_address_; }

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
    if (payload_.size() > 0) {
      return payload_.at(kUdsPayloadSidOffset);
    } else {
      throw std::out_of_range("UdsMessage Payload has insufficient size.");
    }
  }

  inline uint16_t GetTotalLength() {
    return (sizeof(source_address_) + sizeof(target_address_) + GetSize());
  }
  /**
   * \brief Create a response message with source & target address.
   * \return The uds response message.
   */
  inline UdsMessage CreateResponse() const {
    UdsMessage response{this->target_address_, this->source_address_};
    return response;
  }
};

std::ostream& operator<<(std::ostream& stream, const UdsMessage& message);
bool operator==(const UdsMessage& left, const UdsMessage& right);
#endif