#ifndef __GATE_WAY_H__
#define __GATE_WAY_H__
#include <netinet/in.h>
#include <string>
#include <atomic>
#include <mutex>
#include <string>
#include <condition_variable>

#include "Payload.h"

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
  // std::mutex route_mutex_, write_mutex_, thread_mutex_, reply_Status_mutex_;
  // std::condition_variable route_response_cv_, tester_present_response_, 
  //                         send_again_cv_, gateway_reply_cv_, gateway_reply_status_cv_;
  // int tcp_socket_ = -1;
  std::string VIN;
  ByteVector EID;
  ByteVector GID;
  uint16_t gate_way_address_;
  uint8_t FurtherActionRequired;
  bool tcp_tester_present_flag_ = false;
  bool is_tcp_socket_open_ = false;
  bool route_response{false};

  // bool is_reply_{false};
  std::string gate_way_ip_;
  // std::atomic<bool> send_diagnostic_again_{true}, diagnostic_msg_ack_{false}, diagnostic_msg_nack_{false}, diagnostic_msg_response_{false}, diagnostic_msg_time_out_{false};


  bool GetRouteResponse() { return route_response; };
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


  // bool GetSendAgain() { return send_diagnostic_again_; }
  // bool GetDiagnosticAck() { return diagnostic_msg_ack_; }
  // bool GetDiagnosticNack() { return diagnostic_msg_nack_; }
  // bool GetDiagnosticResponse() { return diagnostic_msg_response_; }
  // bool GetDiagnosticTimeOut() { return diagnostic_msg_time_out_; }
  // bool GetSocketStatus() { return is_tcp_socket_open_; }
  // bool GetReply() { return is_reply_; }


  // void SetSendAgain(bool flag) { send_diagnostic_again_ = flag; }
  // void SetDiagnosticAck(bool flag) { diagnostic_msg_ack_ = flag;}
  // void SetDiagnosticNack(bool flag) { diagnostic_msg_nack_ = flag;}
  // void SetDiagnosticResponse(bool flag) { diagnostic_msg_response_ = flag;}
  // void SetDiagnosticTimeOut(bool flag) { diagnostic_msg_time_out_ = flag; }
  // void SetTcpStatus(bool flag) { is_tcp_socket_open_ = flag; }  
  // void SetReply(bool flag) { is_reply_ = flag; }
};


#endif