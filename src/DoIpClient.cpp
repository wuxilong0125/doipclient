#include "DoIpClient.h"

#include <arpa/inet.h>
#include <ifaddrs.h>

#include <cstring>
DoIpClient::DoIpClient() {}

/**
 * @brief 获取主机所有Ip地址(与DoIp服务Ip在一个网段下)
 *
 * @return int
 */
int DoIpClient::GetAllLocalIps() {
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

      if (0 == std::strncmp(addressBuffer, server_ip_prefix_.c_str(),
                            strlen(server_ip_prefix_.c_str()))) {
        local_ips_.push_back(addressBuffer);
      }
    }
    ifAddrStruct = ifAddrStruct->ifa_next;
  }
  if (local_ips_.size() == 0) {
    return -1;
  }
  return 0;
}

int DoIpClient::SetUdpSocket(const char *ip, int &udpSockFd) {
  if (ip == nullptr) return -1;
  udpSockFd = socket(AF_INET, SOCK_DGRAM, 0);
  if (udpSockFd < 0) {
    return -1;
  }

  struct sockaddr_in udp_sockaddr;
  udp_sockaddr.sin_family = AF_INET;
  udp_sockaddr.sin_port = htons(kUdpPort);
  udp_sockaddr.sin_addr.s_addr = inet_addr(ip);
  memset(&udp_sockaddr.sin_zero, 0, 8);

  int retval{bind(udpSockFd, (const sockaddr *)(&udp_sockaddr),
                  sizeof(struct sockaddr_in))};
  if (retval < 0) {
    return -1;
  }
  return 0;
}

int DoIpClient::UdpHandler(int udp_socket) {
  uint8_t receivedUdpData[kMaxDataSize];
  struct sockaddr client_addr;
  socklen_t length(sizeof(struct sockaddr));
  ssize_t bytes_received{recvfrom(udp_socket, receivedUdpData, kMaxDataSize, 0,
                                  (struct sockaddr *)&client_addr, &length)};
  if (bytes_received <= 0) {
    return -1;
  }
  if (bytes_received > std::numeric_limits<uint8_t>::max()) {
    return -1;
  }
  // TODO 处理Udp数据  HandleUdpMessage
}

int DoIpClient::HandleUdpMessage(uint8_t msg[], ssize_t bytes_available,
                                 DoIpPacket &udp_packet) {
  if (bytes_available < kDoIp_HeaderTotal_length) {
    return -1;
  }
  if (~msg[kDoIp_ProtocolVersion_offset] !=
      msg[kDoIp_InvProtocolVersion_offset]) {
        udp_packet.setPayloadType(DoIpPayload::kGenericDoIpNack);
    return 0;
  }
  DoIpPacket::PayloadLength expected_payload_length{bytes_available -
                                                    kDoIp_HeaderTotal_length};
  DoIpPacket udp_message{DoIpPacket::kHost};
  udp_message.SetPayloadLength(expected_payload_length);
  udp_message.setPayloadType(
      DoIpPacket::PayloadType((msg[kDoIp_PayloadType_offset]) << 8) +
      msg[kDoIp_PayloadType_offset + 1]);
  udp_message.SetProtocolVersion(
      DoIpPacket::ProtocolVersion(msg[kDoIp_ProtocolVersion_offset]));
  
  return 0;
}
