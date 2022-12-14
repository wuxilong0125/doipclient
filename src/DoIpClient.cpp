#include "DoIpClient.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>

#include <condition_variable>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "MultiByteType.h"

#define DEBUG
#ifndef DEBUG
#define DEBUG(info) printf(info)
#endif

#define PRINT
#ifndef PRINT
#define PRINT(info_1, info_2) printf(info_1, info_2)
#endif

#define CPRINT
#ifndef CPRINT
#define CPRINT(x) std::cout << "[" << __FUNCTION__ << "]" << x << std::endl;
#endif

#define PRINT_V
#ifndef PRINT_V
#define PRINT_V(_1, _2, _3) \
  printf(_1);               \
  for (auto v : _2) {       \
    printf(_3, v);          \
  }                         \
  printf("\n")
#endif

DoIpClient::DoIpClient() {
  // timer = new CTimer(
  //     std::bind(&DoIpClient::TimerCallBack, this, std::placeholders::_1));
  // tester_present_timer = nullptr;
}
DoIpClient::~DoIpClient() {
  // if (timer) {
  //   delete timer;
  // }
  // if (tester_present_timer) {
  //   delete tester_present_timer;
  // }
  for (auto& mp : GateWays_) {
    if (mp != nullptr) {
      delete mp;
      mp = nullptr;
    } 
  }

  for (auto& mp : GateWays_map_) {
    if (mp.second != nullptr) {
      delete mp.second;
      mp.second = nullptr;
    }
  }

  for (auto& mp : ecu_map_) {
    if (mp.second != nullptr) {
      delete mp.second;
      mp.second = nullptr;
    }
  }
}

void DoIpClient::TimerCallBack(bool socket) {
  if (!socket) {
    return;
  }

  // CloseTcpConnection();
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
    PRINT("udp_socket: %d\n", udp_sockets[i]);
    re = SendVehicleIdentificationRequest(&broad_addr, udp_sockets[i]);
    if (re == -1) {
      DEBUG("Send SendVehicleIdentificationRequest ERROR.\n");
      return -1;
    }
  }

  // TODO: 修改
  // for (int i = 0; i < (kTimeOut * 1000) / kTimeToSleep; i++) {
  //   std::this_thread::sleep_for(std::chrono::milliseconds(kTimeToSleep));
  // }
  std::unique_lock<std::mutex> lock(udp_mutex_);
  udp_reply_cv_.wait_for(lock, std::chrono::seconds(time_vehicle_Id_req_));

  if (GateWays_.empty()) {
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

void DoIpClient::UdpHandler(int &udp_socket) {
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
      GateWay* gate_way = new GateWay;
      gate_way->vehicle_ip_ = client_addr;

      gate_way->VIN = udp_packet.GetVIN();
      gate_way->gate_way_address_ = (static_cast<uint16_t>(udp_packet.GetLogicalAddress().at(0)) << 8) | 
                          (static_cast<uint16_t>(udp_packet.GetLogicalAddress().at(1)));
      gate_way->EID = udp_packet.GetEID();
      gate_way->GID = udp_packet.GetGID();
      gate_way->FurtherActionRequired = udp_packet.GetFurtherActionRequied();
      GateWays_.push_back(gate_way);
      // GateWays_map_[addr_ip] = gate_way;
      GateWays_map_.insert({gate_way->gate_way_address_, gate_way});
      // udp_reply_cv_.notify_all();
      return;
    }
  }
}

int DoIpClient::TcpHandler(GateWay* gate_way) {
  if (inet_ntoa(gate_way->vehicle_ip_.sin_addr) == nullptr) {
    DEBUG("Get ServerIP ERROR.\n");
    return -1;
  }
  // vehicle_ip_ = vehicle_ip;
  gate_way->tcp_socket_ = socket(AF_INET, SOCK_STREAM, 0);

  if (gate_way->tcp_socket_ <= 0) {
    return -1;
  }
  
  tcp_sockaddr_.sin_family = AF_INET;
  tcp_sockaddr_.sin_port = htons(kPort);
  tcp_sockaddr_.sin_addr = gate_way->vehicle_ip_.sin_addr;
  int re = connect(gate_way->tcp_socket_, (struct sockaddr *)&tcp_sockaddr_,
                   sizeof(tcp_sockaddr_));
  if (-1 == re) {
    return -1;
  }
  std::thread receive_tcp_msg_thread_(&DoIpClient::HandleTcpMessage, this,
                                      gate_way);
  pthread_t receive_handle = receive_tcp_msg_thread_.native_handle();
  receive_tcp_thread_handles_.push_back(receive_handle);
  receive_tcp_msg_thread_.detach();
  
  gate_way->is_tcp_socket_open_ = true;
  return 0;
}

void DoIpClient::TesterPresentThread() {
  while (is_tcp_socket_open_) {
    SendTesterRequest(target_address_);
    std::this_thread::sleep_for(
        std::chrono::seconds(time_tester_present_thread_));
  }
}

void DoIpClient::CloseTcpConnection(GateWay* gate_way) {
  if (!gate_way->is_tcp_socket_open_) {
    return;
  }
  for (auto t : receive_tcp_thread_handles_) {
    pthread_cancel(t);
  }
  
  // pthread_cancel(cycle_send_msg_handle_);
  // close(tcp_socket_);
  if (-1 == shutdown(gate_way->tcp_socket_, SHUT_RDWR)) {
    PRINT("Disconnect from server %s\n", inet_ntoa(gate_way->vehicle_ip_.sin_addr));
    return;
  }

  gate_way->is_tcp_socket_open_ = false;
  PRINT("Disconnect from server %s\n", inet_ntoa(gate_way->vehicle_ip_.sin_addr));
}

void DoIpClient::ReconnectServer(GateWay* gate_way) {
  CloseTcpConnection(gate_way);
  TcpHandler(gate_way);
}
/**
 * @brief 处理udp接收到的数据
 *
 * @param msg udp接收到的数据内容
 * @param bytes_available 数据大小
 * @param udp_packet DoIpPacket对象，用于填充udp数据
 * @return uint8_t 如果数据错误返回DoIpNackCodes中的对应值，否则返回0xFF
 */

DoIpNackCodes DoIpClient::HandleUdpMessage(uint8_t msg[],
                                           ssize_t bytes_available,
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

int DoIpClient::HandleTcpMessage(GateWay* gate_way) {
  if (socket < 0) {
    CPRINT("ERROR : No Tcp Socket set.");
    return -1;
  }

  while (gate_way->is_tcp_socket_open_) {
    DoIpPacket doip_tcp_message(DoIpPacket::kHost);
    int re = SocketReadHeader(gate_way->tcp_socket_, doip_tcp_message);
    doip_tcp_message.Ntoh();
    if (re == -1) {
      CPRINT("SocketReadHeader is ERROR.");
      return -1;
      continue;
    }
    if (0 == re) {
      reconnect_tcp_counter_++;
      if (reconnect_tcp_counter_ == 5) {
        reconnect_tcp_counter_ = 0;
        ReconnectServer(gate_way);
      }
      continue;
    }
    if (!timer->IsRunning()) {
      CPRINT("time-over.");
      continue;
    }
    uint8_t v_code = doip_tcp_message.VerifyPayloadType(); 

    if (v_code != 0xFF) {
      CPRINT("PayloadType ERROR: " + std::to_string(v_code));
      continue;
    }
    if (doip_tcp_message.payload_length_ != 0) {
      doip_tcp_message.SetPayloadLength(doip_tcp_message.payload_length_, true);
      // 接收负载
      int re = SocketReadPayload(gate_way->tcp_socket_, doip_tcp_message);
      if (re != 0) {
        continue;
      } else {
        uint16_t ecu_address = (static_cast<uint16_t>(doip_tcp_message.payload_.at(0)) << 8) 
                              | (static_cast<uint16_t>(doip_tcp_message.payload_.at(1))); 
        switch (doip_tcp_message.payload_type_) {
          case DoIpPayload::kRoutingActivationResponse: {
            CPRINT("kRoutingActivationResponse");
            // timer->Stop();
            if (doip_tcp_message.payload_.at(4) != 0x10) {
              CPRINT("Routing Activation ERROR.");
              
              CloseTcpConnection(gate_way);
              break;
            }
            CPRINT("route_respone is true!");
            gate_way->route_response = true;
            gate_way->route_response_cv_.notify_all();

            break;
          }
          case DoIpPayload::kDiagnosticAck: {
            CPRINT("---DiagnosticAck---");
            ecu_map_[ecu_address]->SetDiagnosticAck(true);
            // diagnostic_msg_ack = true;
            ecu_map_[ecu_address]->ecu_reply_cv_.notify_all();
            break;
          }
          case DoIpPayload::kDiagnosticNack: {
            ecu_map_[ecu_address]->SetDiagnosticNack(true);
            // diagnostic_msg_nack = true;
            ecu_map_[ecu_address]->ecu_reply_cv_.notify_all();
            break;
          }
          case DoIpPayload::kDiagnosticMessage: {
            CPRINT("---DiagnosticMessage---");
            ecu_map_[ecu_address]->SetDiagnosticResponse(true);
            // diagnostic_msg_response = true;
            ecu_map_[ecu_address]->ecu_reply_cv_.notify_all();
            // timer->Stop();
            // TODO: 修改
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
  if (socket < 0) {
    DEBUG("SocketWrite's socket param ERROR.\n");
    return -1;
  }
  DoIpPacket::ScatterArray scatter_array(doip_packet.GetScatterArray());
  struct msghdr message_header;
  if (destination_address != nullptr) {
    // printf("broadaddr: %s \n", inet_ntoa(destination_address->sin_addr));
    message_header.msg_name = destination_address;
    message_header.msg_namelen = sizeof(sockaddr_in);
  } else {
    CPRINT("destination_address is null.");
    return -1;
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

    return -1;
  } else if (((size_t)bytesSent) <
             (doip_packet.payload_length_ + kDoIp_HeaderTotal_length)) {
    DEBUG("SocketWrite bytesSent is smaller than packet Length.\n");
    return -1;
  }
  return 0;
}

int DoIpClient::SocketReadHeader(int socket, DoIpPacket &doip_packet) {
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
  if (socket < 0) {
    CPRINT("socket is error");
    return -1;
  }
  DoIpPacket::PayloadLength buffer_fill(0);
  while ((buffer_fill < doip_packet.payload_.size()) && timer->IsRunning()) {
    ssize_t bytedRead{recv(socket, &(doip_packet.payload_.at(buffer_fill)),
                           (doip_packet.payload_.size() - buffer_fill), 0)};
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

void DoIpClient::SetCallBack(DiagnosticMessageCallBack diag_msg_cb) {
  diag_msg_cb_ = diag_msg_cb;
}

int DoIpClient::SendRoutingActivationRequest(GateWay* gate_way) {
  CPRINT("run----------");
  if (gate_way->tcp_socket_ < 0) {
    CPRINT("TCP socket is error.");
    return -1;
  }
  DoIpPacket routeing_act_req(DoIpPacket::kHost);
  routeing_act_req.ConstructRoutingActivationRequest(source_address_);
  if (gate_way->route_response) {
    gate_way->route_response = false;
  }
  // CTimer::Clock::duration timeout = std::chrono::seconds(time_route_act_req_);
  // timer->StartOnce(timeout, true);

  int re = SocketWrite(gate_way->tcp_socket_, routeing_act_req, &gate_way->vehicle_ip_);
  // printf("re: %d\n",re);
  if (-1 == re) {
    // timer->Stop();
    return -1;
  }
  std::unique_lock<std::mutex> lock(gate_way->route_mutex_);
  
  gate_way->route_response_cv_.wait_for(lock, std::chrono::seconds(time_route_act_req_), 
                                    [gate_way]{ return gate_way->GetRouteResponse();});
  // useconds_t inter_us = 200;
  // while (!route_respone && timer->IsRunning()) {
  //   usleep(inter_us);
  // }

  // sleep(5);
  if (gate_way->route_response) {
    gate_way->route_response = false;
    CPRINT("Routing Activation successful.");\
    // TODO: 修改 发送 tester present
    if (gate_way->tcp_tester_present_flag_) {
      if (tester_present_timer) {
        delete tester_present_timer;
        tester_present_timer = nullptr;
      }
      tester_present_timer = new CTimer(
          std::bind(&DoIpClient::TimerCallBack, this, std::placeholders::_1));
      std::thread sendMsg(&DoIpClient::TesterPresentThread, this);
      cycle_send_msg_handle_ = sendMsg.native_handle();
      sendMsg.detach();
    }
  } else {
    CPRINT("RoutingActivationResponse timeout.");
    return -1;
  }
  return 0;
}

ECUReplyCode DoIpClient::SendECUMeassage(uint16_t ecu_address, ByteVector uds, bool suppress_flag) {

  ECU* ecu = ecu_map_[ecu_address];
  if (ecu_thread_map_.find(ecu_address) == ecu_thread_map_.end()) {
    std::thread send_ecu_msg_thread(&DoIpClient::SendDiagnosticMessageThread, this,
                                      ecu_address);
    ecu_thread_map_.insert(
        {ecu_address, send_ecu_msg_thread.native_handle()});
    send_ecu_msg_thread.detach();
  } else {
    ecu->SetUds(uds);
      
    //  唤醒SendDiagnosticMessage
    ecu->SetSendAgain(true);
    ecu->send_again_cv_.notify_all();
  }
  std::unique_lock<std::mutex> lock(ecu->reply_Status_mutex_);
  ecu->ecu_reply_status_cv_.wait(lock);

  return ecu_reply_status_map_[ecu_address];
}

void DoIpClient::SendDiagnosticMessageThread(uint16_t ecu_address) {
  ECU* ecu = ecu_map_[ecu_address];
  std::unique_lock<std::mutex> lock(ecu->thread_mutex_);
  while(true) {
    ecu->send_again_cv_.wait(lock, [ecu]{ return ecu->GetSendAgain();});
    ByteVector user_data;
    ecu->GetUds(user_data);
    SendDiagnosticMessage(ecu_address, user_data, ecu->GetTimeOut());
    ecu->SetSendAgain(false);
  }
}

void DoIpClient::SendDiagnosticMessage(uint16_t ecu_address, ByteVector user_data, int time_out) {
  ECU* ecu = ecu_map_[ecu_address];
  std::unique_lock<std::mutex> lock(ecu->write_mutex_);
  if (user_data.size() == 0) {
    CPRINT("Payload is NULL.");
    ecu_reply_status_map_.insert({ecu_address, ECUReplyCode::kSendError});
    ecu->ecu_reply_status_cv_.notify_all();
    return ;
  }
  if (ecu->tcp_socket_ <= 0) {
    CPRINT("TCP socket is error.");
    ecu_reply_status_map_.insert({ecu_address, ECUReplyCode::kSendError});
    ecu->ecu_reply_status_cv_.notify_all();
    return ;
  }
  if (!ecu->GetSocketStatus()) {
    CPRINT("TCP is not connected");
    ecu_reply_status_map_.insert({ecu_address, ECUReplyCode::kSendError});
    ecu->ecu_reply_status_cv_.notify_all();
    return ;
  }
  DoIpPacket diagnostic_msg(DoIpPacket::kHost);
  diagnostic_msg.ConstructDiagnosticMessage(source_address_, ecu_address,
                                            user_data);


  if (ecu->GetDiagnosticAck()) {
    ecu->SetDiagnosticAck(false);
  }
  if (ecu->GetDiagnosticNack()) {
    ecu->SetDiagnosticNack(false);
  }
  if (ecu->GetDiagnosticResponse()) {
    ecu->SetDiagnosticResponse(false);
  }
  // timer->StartOnce(timeout);
  int re = SocketWrite(ecu->tcp_socket_, diagnostic_msg, &vehicle_ip_);
  if (-1 == re) {
    ecu_reply_status_map_.insert({ecu_address, ECUReplyCode::kSendError});
    ecu->ecu_reply_status_cv_.notify_all();
    return ;
  }

  ecu_map_[ecu_address]->ecu_reply_cv_.wait_for(lock, std::chrono::seconds(time_out), 
                                              [&ecu]{ return (ecu->GetDiagnosticAck() 
                                                              || ecu->GetDiagnosticNack()
                                                              || ecu->GetDiagnosticResponse());});

  // useconds_t inter_us = 100;
  // while (!diagnostic_msg_ack && timer->IsRunning()) {
  //   usleep(inter_us);
  // }


  if (ecu->GetDiagnosticAck()) {
    ecu_reply_status_map_.insert({ecu_address, ECUReplyCode::kACK});
    ecu->SetDiagnosticAck(false);
    ecu->ecu_reply_status_cv_.notify_all();
  }else if (ecu->GetDiagnosticNack()) {
    ecu_reply_status_map_.insert({ecu_address, ECUReplyCode::kNACK});
    ecu->SetDiagnosticNack(false);
    ecu->ecu_reply_status_cv_.notify_all();
  }else if (ecu->GetDiagnosticResponse()) {
    ecu_reply_status_map_.insert({ecu_address, ECUReplyCode::kResp});
    ecu->SetDiagnosticResponse(false);
    ecu->ecu_reply_status_cv_.notify_all();
  }else {
    CPRINT("DiagnosticMessage ACK timeout.");
    ecu_reply_status_map_.insert({ecu_address, ECUReplyCode::kTimeOut});
    ecu->ecu_reply_status_cv_.notify_all();
    return ;
  }
}

void DoIpClient::SendTesterRequest(uint16_t target_address) {
  std::lock_guard<std::mutex> lk(write_mtx);
  if (tcp_socket_ <= 0) {
    CPRINT("SendTesterRequest ERROR, TCP socket is not open.");
    return;
  }
  if (!is_tcp_socket_open_) {
    CPRINT("SendTesterRequest ERROR, TCP is not connected.");
    return;
  }
  ByteVector user_data{0x3e, 0x80};
  DoIpPacket tester_present_request(DoIpPacket::kHost);
  tester_present_request.ConstructDiagnosticMessage(source_address_,
                                                    target_address_, user_data);
  CTimer::Clock::duration timeout =
      std::chrono::seconds(time_tester_present_req_);
  if (diagnostic_msg_ack) {
    diagnostic_msg_ack = false;
  }
  tester_present_timer->StartOnce(timeout);
  int re = SocketWrite(tcp_socket_, tester_present_request, &vehicle_ip_);
  if (-1 == re) {
    tester_present_timer->Stop();
    CPRINT("SendTesterRequest ERROR, {}", strerror(errno));
    return;
  }
  useconds_t inter_us = 100;
  while (!diagnostic_msg_ack && tester_present_timer->IsRunning()) {
    usleep(inter_us);
  }

  if (diagnostic_msg_ack) {
    tester_present_timer->Stop();
    diagnostic_msg_ack = false;
  } else {
    CPRINT("SendTesterRequest ACK timeout.");
    return;
  }
}

void DoIpClient::SetSourceAddress(uint16_t s_addr) { source_address_ = s_addr; }

void DoIpClient::GetVechicleMsgs(std::vector<GateWay*> &gate_ways) {
  gate_ways = GateWays_;
}

void DoIpClient::SetTimeOut(int time_vehicle_Id_req, int time_route_act_req,
                            int time_diagnostic_msg,
                            int time_tester_present_req,
                            int time_tester_present_thread) {
  time_vehicle_Id_req_ = time_vehicle_Id_req;
  time_route_act_req_ = time_route_act_req;
  time_diagnostic_msg_ = time_diagnostic_msg;
  time_tester_present_req_ = time_tester_present_req;
  time_tester_present_thread_ = time_tester_present_thread;
}