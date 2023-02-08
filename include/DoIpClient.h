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
#include <condition_variable>

#include "CTimer.h"
#include "DoIpPacket.h"
#include "ECU.h"
#include "GateWay.h"

class CTimer;
using DiagnosticMessageCallBack = std::function<void(unsigned char *, int)>;
static const in_port_t kPort = 13400;
// static const int kMaxDataSize = 8092;
// static const int kTimeOut = 6;
static const int kTimeToSleep = 200;  // millisecondss

class DoIpClient {
 private:
  // std::mutex udp_mutex_;
  // std::condition_variable udp_reply_cv_;
  // struct sockaddr_in vehicle_ip_;

  // bool is_tcp_socket_open_{false};
  
  // std::vector<std::thread> threads_;

  // std::thread tcp_handler_thread_;
  // std::vector<std::string> local_ips_;
  struct sockaddr_in tcp_sockaddr_;
  
  // TODO: 修改为智能指针
  CTimer *timer, *tester_present_timer;

  pthread_t  cycle_send_msg_handle_;
  std::vector<pthread_t> receive_tcp_thread_handles_;
  DiagnosticMessageCallBack diag_msg_cb_;
  int reconnect_tcp_counter_;
  // std::atomic<bool> route_response{false}, diagnostic_msg_ack{false}, 
  //                   diagnostic_msg_nack{false}, diagnostic_msg_response{false};
  uint16_t source_address_, target_address_;
  bool tcp_tester_present_flag_ = false;
  // 存储已响应的网关
  // std::vector<GateWay*> GateWays_;
  std::vector<std::shared_ptr<GateWay>> GateWays_;

  int time_route_act_req_, time_diagnostic_msg_, time_tester_present_req_, time_tester_present_thread_;
  std::mutex write_mtx;
  // 网关和ECU的映射 first: ecu  second: gateway 或 first: gateway  second: gateway
  // 发送uds给出的address有可能是ecu的也有可能是gateway的
  std::map<uint16_t, uint16_t> ecu_gateway_map_;
  
  std::map<uint16_t, std::shared_ptr<GateWay>> GateWays_map_;
  std::map<uint16_t, std::shared_ptr<ECU>> ecu_map_;
  std::map<uint16_t, pthread_t> ecu_thread_map_;
  std::map<uint16_t, ECUReplyCode> ecu_reply_status_map_;
 public:
  DoIpClient();
  ~DoIpClient();

  /**
   * @brief 获取已经响应的GateWay信息
   * 
   */

  void GetGateWayInfo(std::vector<std::shared_ptr<GateWay>>& GateWays) {
    GateWays.assign(GateWays_.begin(), GateWays_.end());
  }

  /**
   * @brief 接收处理TCP数据报
   */
  int HandleTcpMessage(std::shared_ptr<GateWay> gate_way);

  void TesterPresentThread(std::shared_ptr<GateWay> gate_way);
  /**
   * @brief 处理TCP连接
   */
  int TcpHandler(std::shared_ptr<GateWay> gate_way);
  /**
   * @brief 关闭TCP连接
   */
  void CloseTcpConnection(std::shared_ptr<GateWay> gate_way);
  /**
   * @brief TCP重连
   */
  void ReconnectServer(std::shared_ptr<GateWay> gate_way);

  /**
   * @brief 计时器回调函数
   */
  void TimerCallBack(bool socket);

  /**
   * @brief 设置回调函数
   */
  void SetCallBack(DiagnosticMessageCallBack diag_msg_cb);

  /**
   * @brief 发送路由激活请求
   */
  int SendRoutingActivationRequest(std::shared_ptr<GateWay> gate_way);
  /**
   * @brief 发送诊断数据报
   */
  void SendDiagnosticMessage(uint16_t ecu_address, ByteVector user_data, int time_out);
  void SendTesterRequest(std::shared_ptr<GateWay> gate_way);
  ECUReplyCode SendECUMeassage(uint16_t ecu_address, ByteVector uds);
  void SendDiagnosticMessageThread(uint16_t ecu_address);
  /**
   * @brief 设置数据报源地址
   */
  void SetSourceAddress(uint16_t s_addr);

  void GetAllGateWayAddress(std::vector<uint64_t>& gateway_addresses);

};
#endif