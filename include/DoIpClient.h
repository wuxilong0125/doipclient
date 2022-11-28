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

#include "CTimer.h"
#include "DoIpPacket.h"

class CTimer;
using DiagnosticMessageCallBack = std::function<void(unsigned char *, int)>;
static const in_port_t kPort = 13400;
static const int kMaxDataSize = 8092;
static const int kTimeOut = 6;
static const int kTimeToSleep = 200;  // milliseconds

class DoIpClient {
 private:
  struct sockaddr_in vehicle_ip_;
  int tcp_socket_ = -1;
  int udp_socket_ = -1;
  std::string server_ip_prefix_ = "169.254.";
  std::vector<std::thread> threads_;

  std::thread tcp_handler_thread_;
  std::vector<std::string> local_ips_;
  struct sockaddr_in udp_sockaddr_, tcp_sockaddr_;
  std::string VIN;
  ByteVector EID;
  ByteVector GID;
  ByteVector LogicalAddress_;
  uint8_t FurtherActionRequired_;
  CTimer *timer;
  pthread_t handle_of_tcp_thread_;
  DiagnosticMessageCallBack diag_msg_cb_;
  int reconnect_tcp_counter_;
  std::atomic<int> route_respone{false}, diagnostic_msg_ack{false};
  uint16_t source_address_, target_address_;

 public:
  DoIpClient();
  ~DoIpClient();
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
  int HandleTcpMessage(int tcp_socket);
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
  int UdpHandler(int &udp_socket);

  /**
   * @brief 接收处理UDP数据报
   */
  uint8_t HandleUdpMessage(uint8_t msg[], ssize_t length,
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
  int SendDiagnosticMessage(ByteVector user_data);
  /**
   * @brief 获取路由回复状态
   */
  bool GetIsRouteResponse();
  /**
   * @brief 获取诊断肯定请求状态
   */
  bool GetIsDiagnosticAck();
  /**
   * @brief 设置数据报源地址
   */
  void SetSourceAddress(uint16_t s_addr);
};
#endif