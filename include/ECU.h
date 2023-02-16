#ifndef __ECU_H__
#define __ECU_H__

#include <stdint.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>

#include "Payload.h"

enum ECUReplyCode : uint8_t {
  kACK = 0x01,
  kNACK = 0x02,
  kResp = 0x03,
  kTimeOut = 0x04,
  kSendError = 0xff
};

using ECUCallBack = std::function<ECUReplyCode()>;
using MsgType = std::pair<uint16_t, ByteVector>;
class ECU {
  uint16_t m_address;
  ByteVector m_uds;
  int m_timeout;
  ECUCallBack m_ecuCallBack;

  bool m_sendDiagnosticAgain{true}, m_TcpConnected{false};
  bool m_isReply{false};
  std::atomic<bool> m_diagnosticMsgAck{false}, m_diagnosticMsgNack{false}, m_diagnosticMsgResponse{false}, m_diagnosticMsgTimeOut{false};
  std::queue<MsgType> m_msgQueue;
  std::mutex m_queueMutex;
  std::condition_variable m_queueCondition;

 public:
  ECU(uint16_t address, ByteVector uds);
  ECU(uint16_t address, int timeout);
  ~ECU();
  std::condition_variable m_sendAgainCondition, m_ecuReplyCondition, m_ecuReplyStatusCondition;
  std::mutex m_threadMutex, m_writeMutex, m_replyStatusMutex;
  int tcp_socket_ = -1;
  inline void SetCallBack(ECUCallBack ecuCallBack) { m_ecuCallBack = ecuCallBack; };
  inline void SetAddress(uint16_t address) { m_address = address; };
  inline void SetUds(ByteVector uds) { m_uds.assign(uds.begin(), uds.end()); };
  inline void SetTimeOut(int timeout) { m_timeout = timeout; };
  inline uint16_t GetAddress() { return m_address; };
  inline void GetUds(ByteVector &uds) { uds = m_uds; };
  inline int GetTimeOut() { return m_timeout; };
  void RunCallBack();

  bool GetSendAgain() { return m_sendDiagnosticAgain; }
  bool GetDiagnosticAck() { return m_diagnosticMsgAck; }
  bool GetDiagnosticNack() { return m_diagnosticMsgNack; }
  bool GetDiagnosticResponse() { return m_diagnosticMsgResponse; }
  bool GetDiagnosticTimeOut() { return m_diagnosticMsgTimeOut; }
  bool GetSocketStatus() { return m_TcpConnected; }
  bool GetReply() { return m_isReply; }

  void SetSendAgain(bool flag) { m_sendDiagnosticAgain = flag; }
  void SetDiagnosticAck(bool flag) { m_diagnosticMsgAck = flag; }
  void SetDiagnosticNack(bool flag) { m_diagnosticMsgNack = flag; }
  void SetDiagnosticResponse(bool flag) { m_diagnosticMsgResponse = flag; }
  void SetDiagnosticTimeOut(bool flag) { m_diagnosticMsgTimeOut = flag; }
  void SetTcpStatus(bool flag) { m_TcpConnected = flag; }
  void SetReply(bool flag) { m_isReply = flag; }

  void PushECUMsg(MsgType data) {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    m_msgQueue.push(data);
    m_queueCondition.notify_one();   
  }

  MsgType PopECUMsg() {
    std::unique_lock<std::mutex> lock(m_queueMutex);
    while(m_msgQueue.empty()) {
      m_queueCondition.wait(lock);
    }
    auto ret = std::move(m_msgQueue.front());
    m_msgQueue.pop();
    return ret;
  }
};

#endif