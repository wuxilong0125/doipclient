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

static const in_port_t kUdpPort = 13400;
static const int kMaxDataSize = 8092;
static const int kTimeOut = 6;
static const int kTimeToSleep = 200; // milliseconds
class DoIpClient {
 private:
  struct sockaddr_in vehicle_ip_;
  int tcp_socket_ = -1;
  int udp_socket_ = -1;
  std::string server_ip_prefix_ = "169.254.";
  std::vector<std::thread> threads_;

  std::thread tcp_handler_thread_;
  std::vector<std::string> local_ips_;
  struct sockaddr_in udp_sockaddr_; 
  std::string VIN;
  ByteVector EID;
  ByteVector GID;
  ByteVector LogicalAddress_;
  uint8_t FurtherActionRequired_;


 public:
  DoIpClient();
  ~DoIpClient();
  void start();
  int GetAllLocalIps();
  void ReceiveTcpMessages(int tcp_socket);
  inline void SetServerIpPrefix(std::string ip) { server_ip_prefix_ = ip; }
  int SetUdpSocket(const char *ip, int &udpSockFd);
  int UdpHandler(int &udp_socket);
  uint8_t HandleUdpMessage(uint8_t msg[], ssize_t length,
                           DoIpPacket &udp_packet);
  int SendVehicleIdentificationRequest(struct sockaddr_in *destination_address,
                                       int udp_socket);
  int SocketWrite(int socket, DoIpPacket &doip_packet,
                  struct sockaddr_in *destination_address);
  int FindTargetVehicleAddress();
};
#endif