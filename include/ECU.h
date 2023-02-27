#ifndef __ECU_H__
#define __ECU_H__

#include <stdint.h>
#include <unistd.h>

#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <queue>

#include "PayloadOwner.h"

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
  std::atomic<bool> m_diagnosticMsgAck{false}, m_diagnosticMsgNack{false}, m_diagnosticMsgResponse{false};
  std::queue<MsgType> m_msgQueue;
  std::mutex m_queueMutex;
  std::condition_variable m_queueCondition;

 public:
  ECU(uint16_t address, ByteVector uds);
  ~ECU();
  std::condition_variable m_ecuReplyCondition;

  
  inline void SetAddress(uint16_t address) { m_address = address; };
  inline void SetUds(ByteVector uds) { m_uds.assign(uds.begin(), uds.end()); };
  inline uint16_t GetAddress() { return m_address; };
  inline void GetUds(ByteVector &uds) { uds = m_uds; };


  bool GetDiagnosticAck() { return m_diagnosticMsgAck; }
  bool GetDiagnosticNack() { return m_diagnosticMsgNack; }
  bool GetDiagnosticResponse() { return m_diagnosticMsgResponse; }


  void SetDiagnosticAck(bool flag) { m_diagnosticMsgAck = flag; }
  void SetDiagnosticNack(bool flag) { m_diagnosticMsgNack = flag; }
  void SetDiagnosticResponse(bool flag) { m_diagnosticMsgResponse = flag; }


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