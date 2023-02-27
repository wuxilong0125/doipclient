#include "ECU.h"

ECU::ECU(uint16_t address, ByteVector uds)
    : m_address(address),
      m_uds(uds){};
ECU::~ECU() {}
