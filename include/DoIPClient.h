#ifndef __DOIPCLIENT_H__
#define __DOIPCLIENT_H__
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
#include "DoIPPacket.h"
#include "ECU.h"
#include "GateWay.h"

class CTimer;
// using DiagnosticMessageCallBack = std::function<void(ByteVector)>;
using EcuReplyCallBack = std::function<void(uint16_t, ByteVector, ECUReplyCode)>;
static const in_port_t kPort = 13400;


class DoIpClient {
 private:
  struct sockaddr_in m_targetIp;
  std::atomic<int> m_tcpSocket{-1};
  bool m_tcpConnected = false;
  
  EcuReplyCallBack m_ecuReplyCallBack;
  int m_reconnectTcpCounter;

  uint16_t m_sourceAddress, m_targetAddress;
  bool m_tcpTesterPresentFlag = true;
  bool m_routeResponse{false};
  // 存储已响应的网关
  std::vector<std::shared_ptr<GateWay>> m_gateWays;

  const int m_routeActiveReqestTime = 2, m_diagnosticMsgTime = 2;
  int m_testerPresentThreadTime;
  std::mutex m_writeMutex, m_routeMutex;
  std::condition_variable m_routeResponseCondition, m_testerPresentResponseCondition;
  std::map<uint16_t, std::shared_ptr<GateWay>> m_gateWaysMap;
  std::map<uint16_t, pthread_t> m_ecuThreadMap;
  std::atomic<bool> m_testPresentReplyAck{false};
  std::map<uint16_t, std::shared_ptr<ECU>> m_ecuMap;


 public:
  DoIpClient(int testPresentTime, bool testPresent);
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
   * @brief 设置回调函数
   */
  void SetCallBack(EcuReplyCallBack callBack);

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



  bool GetTestPresentReplyAck() { return m_testPresentReplyAck; }
  bool GetSocketStatus() { return m_tcpConnected; }




};
#endif