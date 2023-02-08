#include <arpa/inet.h>
#include <ifaddrs.h>
#include <cstring>
#include <iostream>
#include <thread>


#include "VehicleTools.h"
#include "DoIpPacket.h"
#include "CommonTools.h"

// #define DEBUG
#ifndef DEBUG
#define DEBUG(info) printf(info)
#endif

// #define PRINT
#ifndef PRINT
#define PRINT(info_1, info_2) printf(info_1, info_2)
#endif

// #define CPRINT
#ifndef CPRINT
#define CPRINT(x) std::cout << "[" << __FUNCTION__ << "]" << x << std::endl;
#endif

const in_port_t kPort = 13400;
const int kMaxDataSize = 8092;
const int time_vehicle_Id_req_ = 5;

std::string vehicle_ip_prefix_ = "169.254.";

struct sockaddr_in udp_sockaddr_;

std::mutex udp_mutex_;
std::condition_variable udp_reply_cv_;


/**
 * @brief 获取主机所有Ip地址(与DoIp服务Ip在一个网段下)
 *
 * @return int
 */
int GetAllLocalIps(std::vector<std::string>& vehicle_ips) {
  struct ifaddrs *ifAddrStruct = NULL;
  void *tmpAddrPtr = NULL;
  getifaddrs(&ifAddrStruct);

  while (ifAddrStruct != NULL) {
    if (ifAddrStruct->ifa_addr == NULL) {
      ifAddrStruct = ifAddrStruct->ifa_next;
      continue;
    }
    if (ifAddrStruct->ifa_addr->sa_family == AF_INET) {
      tmpAddrPtr = &((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
      char addressBuffer[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);

      if (0 == std::strncmp(addressBuffer, vehicle_ip_prefix_.c_str(),
                            strlen(vehicle_ip_prefix_.c_str()))) {
        vehicle_ips.push_back(addressBuffer);
        PRINT("push local_ip: %s\n", addressBuffer);
      }
    }
    ifAddrStruct = ifAddrStruct->ifa_next;
  }
  if (vehicle_ips.size() == 0) {
    return -1;
  }
  return 0;
}

int SetUdpSocket(const char *ip, int &udpSockFd) {
  if (ip == nullptr) return -1;
  udpSockFd = socket(AF_INET, SOCK_DGRAM, 0);
  if (udpSockFd < 0) {
    return -1;
  }
  udp_sockaddr_.sin_family = AF_INET;
  udp_sockaddr_.sin_port = htons(kPort);
  udp_sockaddr_.sin_addr.s_addr = inet_addr(ip);
  memset(&udp_sockaddr_.sin_zero, 0, 8);

  int retval{bind(udpSockFd, (const sockaddr *)(&udp_sockaddr_),
                  sizeof(struct sockaddr_in))};
  if (retval < 0) {
    PRINT("bind is error: %d\n", errno);
    return -1;
  }
  return 0;
}

/**
 * @brief 处理udp接收到的数据
 *
 * @param msg udp接收到的数据内容
 * @param bytes_available 数据大小
 * @param udp_packet DoIpPacket对象，用于填充udp数据
 * @return uint8_t 如果数据错误返回DoIpNackCodes中的对应值，否则返回0xFF
 */
DoIpNackCodes HandleUdpMessage(uint8_t msg[], ssize_t bytes_available,
                               DoIpPacket &udp_packet) {
  if (bytes_available < kDoIp_HeaderTotal_length) {
    return DoIpNackCodes::kInvalidPayloadLength;
  }
  // ~会将小整型提升提升为较大的整型
  if ((uint8_t)(~msg[kDoIp_ProtocolVersion_offset]) !=
      msg[kDoIp_InvProtocolVersion_offset]) {
    udp_packet.SetPayloadType(DoIpPayload::kGenericDoIpNack);
    DEBUG("HandleUdpMessage_f: ProtocolVersion not equal to ProtocolVersion\n");
    return DoIpNackCodes::kIncorrectPatternFormat;
  }
  DoIpPacket::PayloadLength expected_payload_length{bytes_available -
                                                    kDoIp_HeaderTotal_length};
  udp_packet.SetPayloadLength(expected_payload_length);
  PRINT("udp packet type first value is 0x%02x\n",
        msg[kDoIp_PayloadType_offset]);
  PRINT("udp packet type second value is 0x%02x\n",
        msg[kDoIp_PayloadType_offset + 1]);

  udp_packet.SetPayloadType(msg[kDoIp_PayloadType_offset],
                            msg[kDoIp_PayloadType_offset + 1]);
  udp_packet.SetProtocolVersion(
      DoIpPacket::ProtocolVersion(msg[kDoIp_ProtocolVersion_offset]));
  DoIpNackCodes re = udp_packet.VerifyPayloadType();
  if (DoIpNackCodes::kNoError != re) return re;
  // 填充负载内容
  for (int i = kDoIp_HeaderTotal_length; i < bytes_available; i++) {
    udp_packet.payload_.at(i - kDoIp_HeaderTotal_length) = msg[i];
  }

  return DoIpNackCodes::kNoError;
}

/**
 * @brief 接收udp数据
 *
 * @param udp_socket
 * @return int
 */
void UdpHandler(int &udp_socket, std::vector<std::shared_ptr<GateWay>>& VehicleGateWays) {
  uint8_t received_udp_datas[kMaxDataSize];
  struct sockaddr_in client_addr;
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(kPort);
  client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  socklen_t length(sizeof(struct sockaddr));
  while (true) {
    ssize_t bytes_received{recvfrom(udp_socket, received_udp_datas,
                                    kMaxDataSize, 0,
                                    (struct sockaddr *)&client_addr, &length)};
    if (bytes_received <= 0) {
      PRINT("recvfrom error: %d\n", errno);
      return;
    }
    if (bytes_received > std::numeric_limits<uint8_t>::max()) {
      return;
    }
    DoIpPacket udp_packet(DoIpPacket::kHost);
    DoIpNackCodes re =
        HandleUdpMessage(received_udp_datas, bytes_received, udp_packet);
    if (DoIpNackCodes::kNoError == re &&
        udp_packet.payload_type_ == DoIpPayload::kVehicleAnnouncement) {
      uint16_t gate_way_address_ =
          (static_cast<uint16_t>(udp_packet.GetLogicalAddress().at(0)) << 8) |
          (static_cast<uint16_t>(udp_packet.GetLogicalAddress().at(1)));
      CPRINT("check gate_way address.");

      auto gate_way = std::make_shared<GateWay>(
          client_addr, udp_packet.GetVIN(), udp_packet.GetEID(),
          udp_packet.GetGID(), udp_packet.GetFurtherActionRequied());
      VehicleGateWays.push_back(gate_way);

      return;
    }
  }
}



int SendVehicleIdentificationRequest(struct sockaddr_in *destination_address,
                                     int udp_socket) {
  if (destination_address == nullptr) {
    DEBUG("destination_address is null.\n");
    return -1;
  }
  int broadcast = 1;
  int socket_error = setsockopt(udp_socket, SOL_SOCKET, SO_BROADCAST,
                                &broadcast, sizeof(broadcast));
  if (-1 == socket_error) {
    DEBUG("setsockopt(broadcast) is error.\n");
    return -1;
  }
  // TODO: 设置超时接口
  struct timeval tv;
  tv.tv_sec = time_vehicle_Id_req_;
  tv.tv_usec = 0;
  socket_error =
      setsockopt(udp_socket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
  if (-1 == socket_error) {
    DEBUG("setsockopt(timeout) is error.\n");
    return -1;
  }
  DoIpPacket vehicle_Id_request(DoIpPacket::kHost);
  vehicle_Id_request.ConstructVehicleIdentificationRequest();
  int re = SocketWrite(udp_socket, vehicle_Id_request, destination_address);
  if (0 == re)
    return 0;
  else
    return -1;
}

int FindTargetVehicleAddress(std::vector<std::shared_ptr<GateWay>>& VehicleGateWays) {
  std::vector<std::string> vehicle_ips;
  int re = GetAllLocalIps(vehicle_ips);
  if (-1 == re) {
    // DEBUG("GetAllLocalIps ERROR.\n");
    return -1;
  }

  std::vector<int> udp_sockets(static_cast<int>(vehicle_ips.size()), -1);

  for (int i = 0; i < vehicle_ips.size(); i++) {
    // PRINT("local_ip: %s\n", local_ips_[i].c_str());
    re = SetUdpSocket(vehicle_ips[i].c_str(), udp_sockets[i]);
    if (-1 == re) {
      DEBUG("SetUdpSocket is error.\n");
      return -1;
    }
    std::thread receive_udp_msg_thread(UdpHandler, std::ref(udp_sockets[i]), std::ref(VehicleGateWays));
    receive_udp_msg_thread.detach();
    sockaddr_in broad_addr;
    broad_addr.sin_family = AF_INET;
    broad_addr.sin_port =
        htons(kPort);  // 将整形变量从主机字节序转变为网络字节序
    inet_aton("255.255.255.255", &(broad_addr.sin_addr));
    PRINT("udp_socket: %d\n", udp_sockets[i]);
    re = SendVehicleIdentificationRequest(&broad_addr, udp_sockets[i]);
    if (re == -1) {
      DEBUG("Send SendVehicleIdentificationRequest ERROR.\n");
      return -1;
    }
  }

  std::unique_lock<std::mutex> lock(udp_mutex_);
  udp_reply_cv_.wait_for(lock, std::chrono::seconds(time_vehicle_Id_req_));

  if (VehicleGateWays.empty()) {
    DEBUG("NO VehicleIdentificationResponse received.\n");
    return -1;
  }

  CPRINT("end find.");
  return 0;
}