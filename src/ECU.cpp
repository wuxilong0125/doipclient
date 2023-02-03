#include "ECU.h"

ECU::ECU(uint16_t address, ByteVector uds, int timeout)
    : address_(address),
      uds_(uds),
      timeout_(timeout){

      };
ECU::ECU(uint16_t address, int timeout) : address_(address), timeout_(timeout){};
ECU::~ECU() {}
void ECU::SetAddress(uint16_t address) { address_ = address; };
void ECU::SetCallBack(ECUCallBack cb) { ecu_call_back_ = cb; };
void ECU::SetTimeOut(int timeout) { timeout_ = timeout; };
void ECU::SetUds(ByteVector uds) { uds_ = uds; };

uint16_t ECU::GetAddress() { return address_; };
int ECU::GetTimeOut() { return timeout_; };
void ECU::GetUds(ByteVector &uds) { uds = uds_; };
void ECU::RunCallBack() { ecu_call_back_(); };
