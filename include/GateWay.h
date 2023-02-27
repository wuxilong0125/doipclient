#ifndef __GATE_WAY_H__
#define __GATE_WAY_H__
#include <netinet/in.h>
#include <string>
#include <atomic>
#include <mutex>
#include <string>
#include <condition_variable>

#include "PayloadOwner.h"

// enum GateWayReplyCode : uint8_t {
//   kACK = 0x01,
//   kNACK = 0x02,
//   kResp = 0x03,
//   kTimeOut = 0x04,
//   kSendError = 0xff
// };
class GateWay { 
 public:
  struct sockaddr_in vehicle_ip_;

  std::string VIN;
  ByteVector EID;
  ByteVector GID;
  uint16_t gate_way_address_;
  uint8_t FurtherActionRequired;
  bool tcp_tester_present_flag_ = false;
  bool is_tcp_socket_open_ = false;
  bool route_response{false};

  std::string gate_way_ip_;
  GateWay(uint16_t address) {
    gate_way_address_ = address;
  }
  GateWay(struct sockaddr_in ip,  std::string vin, ByteVector eid, ByteVector gid, uint8_t fur) {
    vehicle_ip_ = ip;
    VIN = vin;
    EID.assign(eid.begin(), eid.end());
    GID.assign(gid.begin(), gid.end());
    FurtherActionRequired = fur;
  }
  ~GateWay() {
  }

};


#endif