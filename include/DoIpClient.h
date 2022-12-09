#ifndef __DO_IP_CLIENT_H__
#define __DO_IP_CLIENT_H__
#include <netinet/in.h>
#include <unistd.h>

#include <array>
#include <atomic>
#include <functional>
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
static const int kMaxDataSize = 8092;
static const int kTimeOut = 6;
static const int kTimeToSleep = 200;  // milliseconds

// struct vechicle_msg {
//   struct sockaddr_in vehicle_ip;
//   std::string VIN;
//   ByteVector EID;
//   ByteVector GID;
//   ByteVector LogicalAddress;
//   uint8_t FurtherActionRequired;
// };
class DoIpClient {
 private:
  std::mutex udp_mutex_;
  std::condition_variable udp_reply_cv_;
  struct sockaddr_in vehicle_ip_;
  // int tcp_socket_ = -1;
  int udp_socket_ = -1;

  // bool is_tcp_socket_open_{false};
  
  std::string server_ip_prefix_ = "169.254.";
  std::vector<std::thread> threads_;

  // std::thread tcp_handler_thread_;
  std::vector<std::string> local_ips_;
  struct sockaddr_in udp_sockaddr_, tcp_sockaddr_;
  
  CTimer *timer, *tester_present_timer;
  pthread_t  cycle_send_msg_handle_;
  std::vector<pthread_t> receive_tcp_thread_handles_;
  DiagnosticMessageCallBack diag_msg_cb_;
  int reconnect_tcp_counter_;
  // std::atomic<bool> route_response{false}, diagnostic_msg_ack{false}, 
  //                   diagnostic_msg_nack{false}, diagnostic_msg_response{false};
  uint16_t source_address_, target_address_;
  bool tcp_tester_present_flag_ = false;
  std::vector<GateWay> GateWays_;
  

  int time_vehicle_Id_req_, time_route_act_req_, time_diagnostic_msg_, time_tester_present_req_, time_tester_present_thread_;
  std::mutex write_mtx;
  // 网关和ECU的映射
  std::map<uint16_t, std::set<uint16_t>> GateWays_map_;
  std::map<uint16_t, ECU> ecu_map_;
  std::map<uint16_t, pthread_t> ecu_thread_map_;
  std::map<uint16_t, ECUReplyCode> ecu_reply_status_map_;
 public:
  DoIpClient();
  ~DoIpClient();
  void f() {
    GateWays_map_.insert({0x0001, {0x0002,0x0003}});
  }
  void GetVechicleMsgs(std::vector<GateWay> &gate_ways);
  void SetTimeOut(int time_vehicle_Id_req, int time_route_act_req, 
                            int time_diagnostic_msg, int time_tester_present_req, 
                            int time_tester_present_thread);
  /**
   * @brief 获取所有本机的IP地址
   */
  int GetAllLocalIps();
  /**
   * @brief 查找目标车辆IP地址
   */
  int FindTargetVehicleAddress();
  /**
   * @brief 接收处理TCP数据报
   */
  int HandleTcpMessage(GateWay &gate_way);

  void TesterPresentThread();
  
  /**
   * @brief 处理TCP连接
   */
  int TcpHandler(GateWay &gate_way);
  /**
   * @brief 关闭TCP连接
   */
  void CloseTcpConnection(GateWay &gate_way);
  /**
   * @brief TCP重连
   */
  void ReconnectServer(GateWay &gate_way);
  /**
   * @brief 设置目标服务的IP前缀
   */
  inline void SetServerIpPrefix(std::string ip) { server_ip_prefix_ = ip; }
  /**
   * @brief 设置UDP套接字
   */
  int SetUdpSocket(const char *ip, int &udpSockFd);
  /**
   * @brief 处理UDP连接
   */
  void UdpHandler(int &udp_socket);

  /**
   * @brief 接收处理UDP数据报
   */
  DoIpNackCodes HandleUdpMessage(uint8_t msg[], ssize_t length,
                           DoIpPacket &udp_packet);

  /**
   * @brief 封装发送数据报功能
   */
  int SocketWrite(int socket, DoIpPacket &doip_packet,
                  struct sockaddr_in *destination_address);
  /**
   * @brief 封装接收数据报头部功能
   */
  int SocketReadHeader(int socket, DoIpPacket &doip_packet);
  /**
   * @brief 封装接收数据报负载功能
   */
  int SocketReadPayload(int socket, DoIpPacket &doip_packet);

  /**
   * @brief 计时器回调函数
   */
  void TimerCallBack(bool socket);

  /**
   * @brief 设置回调函数
   */
  void SetCallBack(DiagnosticMessageCallBack diag_msg_cb);
  /**
   * @brief 发送车辆识别请求报文
   */
  int SendVehicleIdentificationRequest(struct sockaddr_in *destination_address,
                                       int udp_socket);
  /**
   * @brief 发送路由激活请求
   */
  int SendRoutingActivationRequest();
  /**
   * @brief 发送诊断数据报
   */
  int SendDiagnosticMessage(uint16_t ecu_address, ByteVector user_data, int timeout);
  void SendTesterRequest(uint16_t target_address);
  void SendECUsMeassage(std::vector<ECU> ecus, bool suppress_flag);
  void SendDiagnosticMessageThread(uint16_t ecu_address);
  /**
   * @brief 设置数据报源地址
   */
  void SetSourceAddress(uint16_t s_addr);
};
#endif