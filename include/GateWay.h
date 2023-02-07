#ifndef __GATE_WAY_H__
#define __GATE_WAY_H__
#include <netinet/in.h>
#include <string>
#include <atomic>
#include <mutex>
#include <string>
#include <condition_variable>

#include "Payload.h"
class GateWay { 
 public:
  struct sockaddr_in vehicle_ip_;
  std::mutex route_mutex_, write_mutex_;
  std::condition_variable route_response_cv_, tester_present_response_;
  int tcp_socket_ = -1;
  std::string VIN;
  ByteVector EID;
  ByteVector GID;
  uint16_t gate_way_address_;
  uint8_t FurtherActionRequired;
  bool tcp_tester_present_flag_ = false;
  bool is_tcp_socket_open_ = false;
  bool route_response{false};
  bool diagnostic_msg_ack{false};
  std::string gate_way_ip_;

  bool GetRouteResponse() { return route_response; };
  bool GetSocketStatus() { return is_tcp_socket_open_; }
  GateWay(uint16_t address) {
    gate_way_address_ = address;
  }
  
  // GateWay(const GateWay& gate_way){
  //   this->vehicle_ip_ = gate_way.vehicle_ip_;
  //   this->route_mutex_ = gate_way.route_mutex_;
  //   this->route_response_cv_ = gate_way.route_response_cv_;
  //   this->tcp_socket_ = gate_way.tcp_socket_;
  //   this->VIN = gate_way.VIN;
  //   this->EID = gate_way.EID;
  //   this->GID = gate_way.GID;
  //   this->Address_ = gate_way.Address_;
  //   this->FurtherActionRequired = gate_way.FurtherActionRequired;
  //   this->tcp_tester_present_flag_ = gate_way.tcp_tester_present_flag_;
  //   this->is_tcp_socket_open_ = false;
  //   this->route_response = gate_way.route_response;
  //   this->gate_way_ip_ = gate_way.gate_way_ip_;
  // }

  // GateWay& operator=(GateWay& gate_way) {
  //   GateWay(gate_way);
  // }

  ~GateWay() {

  }
};


#endif