#ifndef __DO_IP_CLIENT_H__
#define __DO_IP_CLIENT_H__
#include <netinet/in.h>

#include <array>
#include <atomic>
#include <functional>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

#include "DoIpPacket.h"

class DoIpClient {
 private:
  struct in_addr client_ip_, server_ip_;
  int tcp_socket_;
  int udp_socket_;
  std::string server_ip_prefix_ = "169.254.";
  std::vector<std::thread> threads_;

  std::thread tcp_handler_thread_;
  std::vector<std::string> local_ips_;

 protected:
  void UdpHandler();
  void TcpHandler();

 public:
  DoIpClient();
  ~DoIpClient();
  void start();
  int GetAllLocalIps();
  void ReceiveMessages(int socket);
  inline void SetServerIpPrefix(std::string ip) { server_ip_prefix_ = ip; }
};
#endif