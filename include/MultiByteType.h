#ifndef __MULTI_BYTE_TYPE_H__
#define __MULTI_BYTE_TYPE_H__

#include <stdexcept>

/**
 * @brief 返回一个单字节，该字节是value中从右到左第byte_offset个字节
 *
 */
template <typename T>
uint8_t GetByte(const T& value, size_t byteOffset) {
  if (byteOffset >= sizeof(T)) {
    throw std::out_of_range("MultiByteType byte_offset out of range");
  }
  return ((value >> (byteOffset * 8)) & 0xFF);
}

/**
 * @brief 将byte_value设置为value中从右到左第byte_offset个字节
 *
 */
template <typename T>
void SetByte(T& value, const uint8_t byteValue, const size_t byteOffset) {
  if (byteOffset >= sizeof(T)) {
    throw std::out_of_range("MultiByteType byte_offset out of range");
  }
  T clear_mask(0);
  clear_mask = (0xFF << (byteOffset * 8));
  clear_mask = ~clear_mask;
  value &= clear_mask;

  T temp(byteValue);
  temp <<= byteOffset * 8;
  value |= temp;
}

#endif