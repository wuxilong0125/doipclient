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
  struct sockaddr_in client_addr;
  socklen_t length(sizeof(struct sockaddr));
  ssize_t bytes_received{recvfrom(udp_socket, receivedUdpData, kMaxDataSize, 0,
                                  (struct sockaddr *)&client_addr, &length)};
  if (bytes_received <= 0) {
    return -1;
  }
  if (bytes_received > std::numeric_limits<uint8_t>::max()) {
    return -1;
  }

  DoIpPacket udp_packet(DoIpPacket::kHost);
  uint8_t re = HandleUdpMessage(receivedUdpData, bytes_received, udp_packet);
  if (0xFF == re &&
      udp_packet.payload_type_ == DoIpPayload::kVehicleAnnouncement) {
    if (udp_socket_ == -1) {
      udp_socket_ = udp_socket;
      vehicle_ip_ = client_addr;
      VIN = udp_packet.GetVIN();
      LogicalAddress_ = udp_packet.GetLogicalAddress();
      EID = udp_packet.GetEID();
      GID = udp_packet.GetGID();
      FurtherActionRequired_ = udp_packet.GetFurtherActionRequied();
      return 0;
    }
  }
  return -1;
}

/**
 * @brief 处理udp接收到的数据
 *
 * @param msg udp接收到的数据内容
 * @param bytes_available 数据大小
 * @param udp_packet DoIpPacket对象，用于填充udp数据
 * @return uint8_t 如果数据错误返回DoIpNackCodes中的对应值，否则返回0xFF
 */
uint8_t DoIpClient::HandleUdpMessage(uint8_t msg[], ssize_t bytes_available,
                                     DoIpPacket &udp_packet) {
  if (bytes_available < kDoIp_HeaderTotal_length) {
    return DoIpNackCodes::kInvalidPayloadLength;
  }
  if (~msg[kDoIp_ProtocolVersion_offset] !=
      msg[kDoIp_InvProtocolVersion_offset]) {
    udp_packet.setPayloadType(DoIpPayload::kGenericDoIpNack);
    return DoIpNackCodes::kIncorrectPatternFormat;
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
  uint8_t re = udp_message.VerifyPayloadType();
  if (0xFF != re) return re;
  if (re == DoIpPayload::kRoutingActivationResponse) {
    // 填充负载内容
    for (int i = kDoIp_HeaderTotal_length; i < bytes_available; i++) {
      udp_message.payload_.at(i - kDoIp_HeaderTotal_length) = msg[i];
    }
  }

  return 0xFF;
}

int DoIpClient::SocketWrite(int socket, DoIpPacket &doip_packet,
                             bool &is_socket_open,
                             struct sockaddr_in *destination_address) {
    if ((socket < 0) || !is_socket_open) {
    is_socket_open = false;
    return -1;
  }

  DoIpPacket::ScatterArray scatter_array(doip_packet.GetScatterArray());

  // Use Message Header for a scattered send
  struct msghdr message_header;
  if (destination_address != nullptr) {
    message_header.msg_name = destination_address;
    message_header.msg_namelen = sizeof(sockaddr_in);
  } else {
    message_header.msg_name = NULL;
    message_header.msg_namelen = 0;
  }
  message_header.msg_iov = scatter_array.begin();
  message_header.msg_iovlen = scatter_array.size();
  message_header.msg_control = nullptr;
  message_header.msg_controllen = 0;
  message_header.msg_flags = 0;

  // Briefly switch DoIpPacket to network byte order
  doip_packet.Hton();
  ssize_t bytesSent{sendmsg(socket, &message_header, 0)};

  

  doip_packet.Ntoh();

  if (bytesSent < 0) {
    // Check for error or socket closed
    // int current_errno{errno};
    // if (IsSocketClosed(current_errno)) {
    
    //   is_socket_open = false;
    // } else {
     
    // }
    return -1;
  } else if (((size_t)bytesSent) <
             (doip_packet.payload_length_ + kDoIp_HeaderTotal_length)) {
    // Check whether insufficient data was sent
    // throw exception::IOException();
    // T_UTILS_PRINT("IOException ERROR");
    return -1;
  }  // else we are happy.
  return 0;
}
