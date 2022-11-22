#include "DoIpClient.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <thread>

#include "CTimer.h"
#include "MultiByteType.h"

#define DEBUG
#ifndef DEBUG
#define DEBUG(info) printf(info)
#endif

#define PRINT
#ifndef PRINT
#define PRINT(info_1, info_2) printf(info_1, info_2)
#endif

// #define CPRINT
#ifndef CPRINT
#define CPRINT(x) std::cout << "[" << __FUNCTION__ << "]" << x << std::endl;
#endif

// #define PRINT_V
#ifndef PRINT_V
#define PRINT_V(_1, _2, _3) \
  printf(_1);               \
  for (auto v : _2) {       \
    printf(_3, v);          \
  }                         \
  printf("\n")
#endif

DoIpClient::DoIpClient() {
  timer = new CTimer(
      std::bind(&DoIpClient::TimerCallBack, this, std::placeholders::_1));
}
DoIpClient::~DoIpClient() {
  if (timer) {
    delete timer;
  }
}

void DoIpClient::TimerCallBack(bool socket) {
  if (!socket) {
    return;
  }

  CloseTcpConnection();
}
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
        PRINT("push local_ip: %s\n", addressBuffer);
      }
    }
    ifAddrStruct = ifAddrStruct->ifa_next;
  }
  if (local_ips_.size() == 0) {
    return -1;
  }
  return 0;
}

int DoIpClient::FindTargetVehicleAddress() {
  int re = GetAllLocalIps();
  if (-1 == re) {
    DEBUG("GetAllLocalIps ERROR.\n");
    return -1;
  }
  int *udp_sockets;
  udp_sockets = (int *)malloc(sizeof(int) * local_ips_.size());
  for (int i = 0; i < local_ips_.size(); i++) {
    PRINT("local_ip: %s\n", local_ips_[i].c_str());
    re = SetUdpSocket(local_ips_[i].c_str(), udp_sockets[i]);
    if (-1 == re) {
      DEBUG("SetUdpSocket is error.\n");
      return -1;
    }
    std::thread receive_udp_msg_thread(&DoIpClient::UdpHandler, this,
                                       std::ref(udp_sockets[i]));
    receive_udp_msg_thread.detach();
    sockaddr_in broad_addr;
    broad_addr.sin_family = AF_INET;
    broad_addr.sin_port =
        htons(kPort);  //将整形变量从主机字节序转变为网络字节序
    inet_aton("255.255.255.255", &(broad_addr.sin_addr));
    // broad_addr.sin_addr.s_addr = inet_addr("255.255.255.255");
    PRINT("udp_socket: %d\n", udp_sockets[i]);
    re = SendVehicleIdentificationRequest(&broad_addr, udp_sockets[i]);
    if (re == -1) {
      DEBUG("Send SendVehicleIdentificationRequest ERROR.\n");
      return -1;
    }
    if (udp_socket_ < 0) break;
  }

  for (int i = 0; i < (kTimeOut * 1000) / kTimeToSleep; i++) {
    if (udp_socket_ != -1) break;
    std::this_thread::sleep_for(std::chrono::milliseconds(kTimeToSleep));
  }
  // sleep(5);

  if (udp_socket_ < 0) {
    DEBUG("NO VehicleIdentificationResponse received.\n");
    return -1;
  }

  for (int i = 0; i < local_ips_.size(); i++) close(udp_sockets[i]);
  free(udp_sockets);

  return 0;
}

int DoIpClient::SetUdpSocket(const char *ip, int &udpSockFd) {
  if (ip == nullptr) return -1;
  udpSockFd = socket(AF_INET, SOCK_DGRAM, 0);
  if (udpSockFd < 0) {
    return -1;
  }

  // struct sockaddr_in udp_sockaddr;
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
 * @brief 接收udp数据
 *
 * @param udp_socket
 * @return int
 */
int DoIpClient::UdpHandler(int &udp_socket) {
  PRINT("UdpHandler socket: %d\n", udp_socket);
  uint8_t received_udp_datas[kMaxDataSize];
  struct sockaddr_in client_addr;
  client_addr.sin_family = AF_INET;
  client_addr.sin_port = htons(kPort);
  client_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  socklen_t length(sizeof(struct sockaddr));
  DEBUG("recvfrom start: \n");
  ssize_t bytes_received{recvfrom(udp_socket, received_udp_datas, kMaxDataSize,
                                  0, (struct sockaddr *)&client_addr, &length)};
  DEBUG("recvfrom end!\n");

  PRINT("received_udp_datas length: %d\n", bytes_received);
  if (bytes_received <= 0) {
    PRINT("recvfrom error: %d\n", errno);
    return -1;
  }
  if (bytes_received > std::numeric_limits<uint8_t>::max()) {
    return -1;
  }

  DoIpPacket udp_packet(DoIpPacket::kHost);
  uint8_t re = HandleUdpMessage(received_udp_datas, bytes_received, udp_packet);
  PRINT("HandleUdpMessage return value is : 0x%02x\n", re);
  PRINT("udp packet type is : 0x%04x\n", udp_packet.payload_type_);
  if (0xFF == re &&
      udp_packet.payload_type_ == DoIpPayload::kVehicleAnnouncement) {
    if (udp_socket_ == -1) {
      udp_socket_ = udp_socket;
      vehicle_ip_ = client_addr;
      VIN = udp_packet.GetVIN();
      LogicalAddress_ = udp_packet.GetLogicalAddress();
      printf("LogicalAddress_:");
      for (auto x : LogicalAddress_) {
        printf("0x%02x ", x);
      }
      printf("\n");
      target_address_ = (LogicalAddress_.at(0) << 8) + LogicalAddress_.at(1);

      EID = udp_packet.GetEID();
      GID = udp_packet.GetGID();
      FurtherActionRequired_ = udp_packet.GetFurtherActionRequied();
      PRINT("vehicle ip: %s\n", inet_ntoa(vehicle_ip_.sin_addr));
      PRINT("VIN : %s\n", VIN.c_str());
      PRINT_V("LogicalAddress_: 0x", LogicalAddress_, "%02x");
      PRINT_V("EID: 0x", EID, "%02x");
      PRINT_V("GID: 0x", GID, "%02x");
      return 0;
    }
  }
  return -1;
}

int DoIpClient::TcpHandler() {
  CPRINT("run---------");
  if (inet_ntoa(vehicle_ip_.sin_addr) == nullptr) {
    DEBUG("Get ServerIP ERROR.\n");
    return -1;
  }

  tcp_socket_ = socket(AF_INET, SOCK_STREAM, 0);

  if (tcp_socket_ <= 0) return -1;
  tcp_sockaddr_.sin_family = AF_INET;
  tcp_sockaddr_.sin_port = htons(kPort);
  tcp_sockaddr_.sin_addr = vehicle_ip_.sin_addr;
  int re = connect(tcp_socket_, (struct sockaddr *)&tcp_sockaddr_,
                   sizeof(tcp_sockaddr_));
  if (-1 == re) {
    return -1;
  }

  std::thread receive_tcp_msg_thread_(&DoIpClient::HandleTcpMessage, this,
                                      tcp_socket_);
  handle_of_tcp_thread_ = receive_tcp_msg_thread_.native_handle();
  receive_tcp_msg_thread_.detach();
  CPRINT("end---------");
  return 0;
}

void DoIpClient::CloseTcpConnection() {
  pthread_cancel(handle_of_tcp_thread_);
  close(tcp_socket_);
  PRINT("Disconnect from server %s\n", inet_ntoa(tcp_sockaddr_.sin_addr));
}

void DoIpClient::ReconnectServer() {
  CloseTcpConnection();
  TcpHandler();
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
  // ~会将小整型提升提升为较大的整型
  if ((uint8_t)(~msg[kDoIp_ProtocolVersion_offset]) !=
      msg[kDoIp_InvProtocolVersion_offset]) {
    udp_packet.SetPayloadType(DoIpPayload::kGenericDoIpNack);
    printf("ProtocolVersion is 0x%02x\n",
           (uint8_t)(~msg[kDoIp_ProtocolVersion_offset]));
    printf("InvProtocolVersion is 0x%02x\n", msg[1]);
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
  uint8_t re = udp_packet.VerifyPayloadType();
  if (0xFF != re) return re;
  // 填充负载内容
  for (int i = kDoIp_HeaderTotal_length; i < bytes_available; i++) {
    udp_packet.payload_.at(i - kDoIp_HeaderTotal_length) = msg[i];
  }

  return 0xFF;
}

int DoIpClient::HandleTcpMessage(int socket) {
  CPRINT("run-----");
  if (socket < 0) {
    CPRINT("ERROR : No Tcp Socket set.");
    return -1;
  }

  while (true) {
    DoIpPacket doip_tcp_message(DoIpPacket::kHost);

    int re = SocketReadHeader(socket, doip_tcp_message);
    doip_tcp_message.Ntoh();
    // doip_tcp_message.PrintPacketByte();
    printf("tcp_message type: 0x%04x\n", doip_tcp_message.payload_type_);
    if (re == -1) {
      CPRINT("SocketReadHeader is ERROR.");
      return -1;
      continue;
    }
    if (0 == re) {
      reconnect_tcp_counter_++;
      if (reconnect_tcp_counter_ == 5) {
        reconnect_tcp_counter_ = 0;
        ReconnectServer();
      }
      continue;
    }
    if (!timer->IsRunning()) {
      CPRINT("time-over.");
      continue;
    }
    uint8_t v_code = doip_tcp_message.VerifyPayloadType();
    // printf("v_code: 0x%02x\n", v_code);
    if (v_code != 0xFF) {
      CPRINT("PayloadType ERROR: " + std::to_string(v_code));
      continue;
    }
    if (doip_tcp_message.payload_length_ != 0) {
      // CPRINT("111");
      doip_tcp_message.SetPayloadLength(doip_tcp_message.payload_length_, true);
      // CPRINT("2222");
      int re = SocketReadPayload(socket, doip_tcp_message);

      // CPRINT("3333");
      if (re != 0) {
        printf("re: %d\n", re);
        continue;
      } else {
        printf("re: %d\n", re);

        switch (doip_tcp_message.payload_type_) {
          case DoIpPayload::kRoutingActivationResponse: {
            CPRINT("kRoutingActivationResponse");
            timer->Stop();
            if (doip_tcp_message.payload_.at(4) != 0x10) {
              CPRINT("Routing Activation ERROR.");
              CloseTcpConnection();
              break;
            }
            CPRINT("route_respone is true!");
            route_respone = true;
            break;
          }
          case DoIpPayload::kDiagnosticAck: {
            CPRINT("---DiagnosticAck---");
            diagnostic_msg_ack = true;
            break;
          }
          case DoIpPayload::kDiagnosticNack: {
            break;
          }
          case DoIpPayload::kDiagnosticMessage: {
            CPRINT("---DiagnosticMessage---");
            timer->Stop();
            if (diag_msg_cb_) {
              diag_msg_cb_(doip_tcp_message.payload_.data() + 4,
                           doip_tcp_message.payload_.size() - 4);
            }
          }
          default:
            break;
        }
      }
    }
  }
}

int DoIpClient::SocketWrite(int socket, DoIpPacket &doip_packet,
                            struct sockaddr_in *destination_address) {
  CPRINT("run--------------");
  if (socket < 0) {
    DEBUG("SocketWrite's socket param ERROR.\n");
    return -1;
  }

  DoIpPacket::ScatterArray scatter_array(doip_packet.GetScatterArray());
  // {
  //   for (auto a : scatter_array) {
  //     std::cout << &(a.iov_base) << " " << a.iov_len << std::endl;
  //   }
  // }
  // Use Message Header for a scattered send
  struct msghdr message_header;

  if (destination_address != nullptr) {
    printf("broadaddr: %s \n", inet_ntoa(destination_address->sin_addr));
    message_header.msg_name = destination_address;
    message_header.msg_namelen = sizeof(sockaddr_in);
  } else {
    CPRINT("destination_address is null.");
    return -1;
    // message_header.msg_name = NULL;
    // message_header.msg_namelen = 0;
  }
  message_header.msg_iov = scatter_array.begin();
  message_header.msg_iovlen = scatter_array.size();
  message_header.msg_control = nullptr;
  message_header.msg_controllen = 0;
  message_header.msg_flags = 0;

  // Briefly switch DoIpPacket to network byte order
  doip_packet.Hton();

  ssize_t bytesSent{sendmsg(socket, &message_header, 0)};
  // printf("bytesSent: %d\n",bytesSent);
  doip_packet.Ntoh();

  if (bytesSent < 0) {
    PRINT("sendmsg error : %d\n", errno);
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
    DEBUG("SocketWrite bytesSent is smaller than packet Length.\n");
    return -1;
  }  // else we are happy.

  CPRINT("end--------------");
  return 0;
}

int DoIpClient::SocketReadHeader(int socket, DoIpPacket &doip_packet) {
  CPRINT("run");
  // doip_packet.Ntoh();
  doip_packet.SetPayloadLength(0);
  DoIpPacket::PayloadLength expected_payload_length{
      doip_packet.payload_length_};
  DoIpPacket::ScatterArray scatter_arry(doip_packet.GetScatterArray());

  struct msghdr message_header;
  message_header.msg_name = NULL;
  message_header.msg_namelen = 0;
  message_header.msg_iov = scatter_arry.begin();
  message_header.msg_iovlen = scatter_arry.size();
  message_header.msg_control = nullptr;
  message_header.msg_controllen = 0;
  message_header.msg_flags = 0;

  doip_packet.Hton();

  ssize_t bytesReceived{recvmsg(socket, &message_header, 0)};
  printf("bytesReceived: %d\n", bytesReceived);
  if (bytesReceived == 0) return 0;
  if (bytesReceived < 0) {
    CPRINT("ERROR: " + std::to_string(errno));
    return -1;
  }
  if (bytesReceived < ((ssize_t)kDoIp_HeaderTotal_length)) {
    CPRINT(" Incomplete DoIp Header received.");
    return -1;
  }

  if (bytesReceived > ((ssize_t)kDoIp_HeaderTotal_length)) {
    CPRINT("Extra Payload bytes read for DoIp Packet.");
    return -1;
  }

  return bytesReceived;
}

int DoIpClient::SocketReadPayload(int socket, DoIpPacket &doip_packet) {
  CPRINT("run-----");
  if (socket < 0) {
    CPRINT("socket is error");
    return -1;
  }
  DoIpPacket::PayloadLength buffer_fill(0);
  printf("payload size: %d\n", doip_packet.payload_.size());
  while ((buffer_fill < doip_packet.payload_.size()) && timer->IsRunning()) {
    CPRINT("recv strart-----");
    ssize_t bytedRead{recv(socket, &(doip_packet.payload_.at(buffer_fill)),
                           (doip_packet.payload_.size() - buffer_fill), 0)};
    printf("bytedRead: %d\n", bytedRead);
    CPRINT("recv end--------");
    if (bytedRead < 0) {
      if ((errno == EWOULDBLOCK) || (errno == EAGAIN) || (errno == EINTR)) {
        continue;
      }
      CPRINT("recv ERROR :" + std::to_string(errno));
      break;
    }
    if (bytedRead == 0) {
      break;
    }
    buffer_fill += bytedRead;
  };
  if (buffer_fill != doip_packet.payload_.size()) {
    CPRINT("ERROR: recv 超时！");
    return -1;
  }
  return 0;
}

int DoIpClient::SendVehicleIdentificationRequest(
    struct sockaddr_in *destination_address, int udp_socket) {
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
  struct timeval tv;
  tv.tv_sec = 5;
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

void DoIpClient::SetCallBack(DiagnosticMessageCallBack diag_msg_cb) {
  diag_msg_cb_ = diag_msg_cb;
}

int DoIpClient::SendRoutingActivationRequest() {
  CPRINT("run----------");
  if (tcp_socket_ < 0) {
    CPRINT("TCP socket is error.");
    return -1;
  }
  DoIpPacket routeing_act_req(DoIpPacket::kHost);
  routeing_act_req.ConstructRoutingActivationRequest(source_address_);
  if (route_respone) {
    route_respone = false;
  }
  CTimer::Clock::duration timeout = std::chrono::seconds(2);
  timer->StartOnce(timeout, true);
  // routeing_act_req.PrintPacketByte();
  int re = SocketWrite(tcp_socket_, routeing_act_req, &vehicle_ip_);
  // printf("re: %d\n",re);
  if (-1 == re) {
    timer->Stop();
    return -1;
  }

  useconds_t inter_us = 200;
  while (!GetIsRouteResponse() && timer->IsRunning()) {
    usleep(inter_us);
  }

  // sleep(5);
  if (route_respone) {
    route_respone = false;
    CPRINT("Routing Activation successful.");
  } else {
    CPRINT("RoutingActivationResponse timeout.");
    return -1;
  }

  return 0;
}

int DoIpClient::SendDiagnosticMessage(ByteVector user_data) {
  if (tcp_socket_ < 0) {
    CPRINT("TCP socket is error.");
    return -1;
  }
  DoIpPacket diagnostic_msg(DoIpPacket::kHost);
  diagnostic_msg.ConstructDiagnosticMessage(source_address_, target_address_,
                                            user_data);
  CTimer::Clock::duration timeout = std::chrono::seconds(2);
  if (diagnostic_msg_ack) {
    diagnostic_msg_ack = false;
  }
  timer->StartOnce(timeout);
  int re = SocketWrite(tcp_socket_, diagnostic_msg, &vehicle_ip_);
  if (-1 == re) {
    timer->Stop();
    return -1;
  }

  useconds_t inter_us = 100;
  while (!GetIsDiagnosticAck() && timer->IsRunning()) {
    usleep(inter_us);
  }

  if (GetIsDiagnosticAck()) {
    diagnostic_msg_ack = false;
  } else {
    CPRINT("DiagnosticMessage ACK timeout.");
    return -1;
  }
  return 0;
}

bool DoIpClient::GetIsRouteResponse() { return route_respone; };
bool DoIpClient::GetIsDiagnosticAck() { return diagnostic_msg_ack; };

void DoIpClient::SetSourceAddress(uint16_t s_addr) { source_address_ = s_addr; }
