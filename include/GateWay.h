#ifndef __GATE_WAY_H__
#define __GATE_WAY_H__
#include <netinet/in.h>
#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "Payload.h"
class GateWay {
 public:
  struct sockaddr_in vehicle_ip_;
  std::mutex route_mutex_;
  std::condition_variable route_response_cv_;
  int tcp_socket_ = -1;
  std::string VIN;
  ByteVector EID;
  ByteVector GID;
  uint16_t Address_;
  uint8_t FurtherActionRequired;
  bool tcp_tester_present_flag_ = false;
  bool is_tcp_socket_open_ = false;
  std::atomic<bool> route_response{false};

  bool GetRouteResponse() { return route_response; };
};


#endif