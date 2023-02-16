#ifndef __DO_IP_CLIENT_H__
#define __DO_IP_CLIENT_H__
#include <netinet/in.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <map>
#include <set>
#include <future>
#include <condition_variable>

#include "CTimer.h"
#include "DoIpPacket.h"
#include "ECU.h"
#include "GateWay.h"

class CTimer;
using DiagnosticMessageCallBack = std::function<void(ByteVector)>;
static const in_port_t kPort = 13400;


class DoIpClient {
 private:
  struct sockaddr_in m_targetIp;
  std::atomic<int> m_tcpSocket{-1};
  bool m_TcpConnected = false;
  // TODO: 修改为智能指针
  CTimer *m_timer, *m_testerPresentTimer;
  
  pthread_t  m_cycleSendMsgHandle;
  std::map<pthread_t, bool> m_threadId;
  DiagnosticMessageCallBack m_diagnosticMsgCallBack;
  int m_reconnectTcpCounter;

  uint16_t m_sourceAddress, m_targetAddress;
  bool m_tcpTesterPresentFlag = false;
  bool m_routeResponse{false};
  // 存储已响应的网关
  // std::vector<GateWay*> GateWays_;
  std::vector<std::shared_ptr<GateWay>> m_gateWays;

  const int m_routeActiveReqestTime = 2, m_diagnosticMsgTime = 2,m_testerPresentRequestTime = 2, m_testerPresentThreadTime = 2;
  std::mutex m_writeMutex, m_routeMutex, m_ecuReplyStatusMutex, m_TcpRecvMutex;
  std::condition_variable m_routeResponseCondition, m_testerPresentResponseCondition, m_ecuReplyStatusCondition, m_killThreadCondition;
  std::map<uint16_t, std::shared_ptr<GateWay>> m_gateWaysMap;
  // std::map<uint16_t, std::shared_ptr<ECU>> ecu_map_;
  std::map<uint16_t, pthread_t> m_ecuThreadMap;
  std::map<uint16_t, ECUReplyCode> m_ecuReplyStatusMap;
  std::atomic<bool> m_sendDiagnosticAgain{true}, m_diagnosticMsgAck{false}, m_diagnosticMsgNack{false}, m_diagnosticMsgResponse{false}, m_diagnosticMsgTimeOut{false};
  std::atomic<bool> m_ecuReplyed{false};
  std::map<uint16_t, std::shared_ptr<ECU>> m_ecuMap;
  uint16_t m_ecuAddress;
  ECUReplyCode m_ecuReplyStatus;
  bool m_killAllThread = false;

 public:
  DoIpClient();
  ~DoIpClient();

  /**
   * @brief 获取已经响应的GateWay信息
   * 
   */

  void GetGateWayInfo(std::vector<std::shared_ptr<GateWay>>& gateWays) {
    gateWays.assign(m_gateWays.begin(), m_gateWays.end());
  }

  /**
   * @brief 接收处理TCP数据报
   */
  int HandleTcpMessage();
  /**
   * @brief 处理ECU回复的状态
   * 
   */
  void HandleEcuReplyStatus();

  void TesterPresentThread();
  /**
   * @brief 处理TCP连接
   */
  int TcpHandler();
  /**
   * @brief 关闭TCP连接
   */
  void CloseTcpConnection();
  /**
   * @brief TCP重连
   */
  void ReconnectServer();

  /**
   * @brief 计时器回调函数
   */
  void TimerCallBack(bool socket);

  /**
   * @brief 设置回调函数
   */
  void SetCallBack(DiagnosticMessageCallBack diagnosticMsgCallBack);

  /**
   * @brief 发送路由激活请求
   */
  int SendRoutingActivationRequest();
  /**
   * @brief 发送诊断数据报
   */
  void SendDiagnosticMessage(uint16_t targetAddress, ByteVector userData);
  void SendTesterRequest();
  void SendECUMeassage(uint16_t ecuAddress, ByteVector uds);
  void SendDiagnosticMessageThread(uint16_t ecuAddress);
  /**
   * @brief 设置数据报源地址
   */
  void SetSourceAddress(uint16_t addr);

  void GetAllGateWayAddress(std::vector<uint64_t>& gatewayAddressVec);


  bool GetRouteResponse() { return m_routeResponse; };
  void SetTargetIp(std::shared_ptr<GateWay> gateWay) { m_targetIp = gateWay->vehicle_ip_; }


  bool GetSendAgain() { return m_sendDiagnosticAgain; }
  bool GetDiagnosticAck() { return m_diagnosticMsgAck; }
  bool GetDiagnosticNack() { return m_diagnosticMsgNack; }
  bool GetDiagnosticResponse() { return m_diagnosticMsgResponse; }
  bool GetDiagnosticTimeOut() { return m_diagnosticMsgTimeOut; }
  bool GetSocketStatus() { return m_TcpConnected; }
  bool GetEcuReplyed() { return m_ecuReplyed; }
  bool GetKillAllThreadStatus() { return m_killAllThread; }

  void SetSendAgain(bool flag) { m_sendDiagnosticAgain = flag; }
  void SetDiagnosticAck(bool flag) { m_diagnosticMsgAck = flag;}
  void SetDiagnosticNack(bool flag) { m_diagnosticMsgNack = flag;}
  void SetDiagnosticResponse(bool flag) { m_diagnosticMsgResponse = flag;}
  void SetDiagnosticTimeOut(bool flag) { m_diagnosticMsgTimeOut = flag; }
  void SetEcuReplyed(bool flag) { m_ecuReplyed = flag; }

};
#endif