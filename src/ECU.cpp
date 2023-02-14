#include "ECU.h"

ECU::ECU(uint16_t address, ByteVector uds)
    : m_address(address),
      m_uds(uds){};
ECU::ECU(uint16_t address, int timeout)
    : m_address(address), m_timeout(timeout){};
ECU::~ECU() {}

void ECU::RunCallBack() { m_ecuCallBack(); };
