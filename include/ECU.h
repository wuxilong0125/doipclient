#ifndef __ECU_H__
#define __ECU_H__

#include <stdint.h>
#include <unistd.h>

#include <functional>
#include <mutex>
#include <condition_variable>

#include "Payload.h"

enum ECUReplyCode : uint8_t {
  kACK = 0x01,
  kNACK = 0x02,
  kResp = 0x03,
  kTimeOut = 0x04,
  kSendError = 0xff
};

using ECUCallBack = std::function<ECUReplyCode()>;

class ECU {
  uint16_t address_;
  ByteVector uds_;
  int timeout_;
  ECUCallBack ecu_call_back_;
  
  bool send_diagnostic_again_{true}, is_tcp_socket_open_{false};

  std::atomic<bool> diagnostic_msg_ack_{false}, diagnostic_msg_nack_{false}, diagnostic_msg_response_{false}, diagnostic_msg_time_out_{false};
 public:

  ECU(uint16_t address, ByteVector uds, int timeout);
  ~ECU();
  std::condition_variable send_again_cv_, ecu_reply_cv_, ecu_reply_status_cv_;
  std::mutex thread_mutex_, write_mutex_, reply_Status_mutex_;
  int tcp_socket_ = -1;
  inline void SetCallBack(ECUCallBack cb);
  inline void SetAddress(uint16_t address);
  inline void SetUds(ByteVector uds);
  inline void SetTimeOut(int timeout);

  inline uint16_t GetAddress();
  inline void GetUds(ByteVector &uds);
  inline int GetTimeOut();
  void RunCallBack();

  // std::mutex *GetMutex() { return mutex_; }
  // std::mutex *GetWriteMutex() { return write_mutex_; }
  // std::mutex *GetReplyStatusMutex() { return reply_Status_mutex_; }
  // void SetMutex(std::mutex *m) { mutex_ = m; }
 
  bool GetSendAgain() { return send_diagnostic_again_; }
  bool GetDiagnosticAck() { return diagnostic_msg_ack_; }
  bool GetDiagnosticNack() { return diagnostic_msg_nack_; }
  bool GetDiagnosticResponse() { return diagnostic_msg_response_; }
  bool GetDiagnosticTimeOut() { return diagnostic_msg_time_out_; }
  bool GetSocketStatus() { return is_tcp_socket_open_; }


  void SetSendAgain(bool flag) { send_diagnostic_again_ = flag; }
  void SetDiagnosticAck(bool flag) { diagnostic_msg_ack_ = flag;}
  void SetDiagnosticNack(bool flag) { diagnostic_msg_nack_ = flag;}
  void SetDiagnosticResponse(bool flag) { diagnostic_msg_response_ = flag;}
  void SetDiagnosticTimeOut(bool flag) { diagnostic_msg_time_out_ = flag; }
  void SetTcpStatus(bool flag) { is_tcp_socket_open_ = flag; }  
  
};

#endif