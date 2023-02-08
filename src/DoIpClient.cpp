#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>

#include <condition_variable>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "MultiByteType.h"
#include "DoIpClient.h"
#include "VehicleTools.h"
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

#define PRINT_V
#ifndef PRINT_V
#define PRINT_V(_1, _2, _3) \
  printf(_1);               
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
  // for (auto mp : GateWays_) {
  //   if (mp != nullptr) {
  //     std::cout << "delete gate way: " << mp->gate_way_address_ << std::endl;
  //     std::cout << mp << std::endl;
  //     delete mp;
  //     mp = nullptr;
  //   } 
  // }

  // for (auto mp : GateWays_map_) {
  //   if (mp.second != nullptr) {
  //     std::cout << "delete gate way: " << mp.first << std::endl;
  //     std::cout << mp.second << std::endl;
  //     delete mp.second;
  //     mp.second = nullptr;
  //   }
  // }

  // for (auto& mp : ecu_map_) {
  //   if (mp.second != nullptr) {
  //     delete mp.second;
  //     mp.second = nullptr;
  //   }
  // }
}

void DoIpClient::TimerCallBack(bool socket) {
  if (!socket) {
    return;
  }

  // CloseTcpConnection();
}





int DoIpClient::TcpHandler(std::shared_ptr<GateWay> gate_way) {
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

void DoIpClient::TesterPresentThread(std::shared_ptr<GateWay> gate_way) {
  while (gate_way->is_tcp_socket_open_) {
    SendTesterRequest(gate_way);
    std::this_thread::sleep_for(
        std::chrono::seconds(time_tester_present_thread_));
  }
}

void DoIpClient::CloseTcpConnection(std::shared_ptr<GateWay> gate_way) {
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

void DoIpClient::ReconnectServer(std::shared_ptr<GateWay> gate_way) {
  CloseTcpConnection(gate_way);
  TcpHandler(gate_way);
}

int DoIpClient::HandleTcpMessage(std::shared_ptr<GateWay> gate_way) {
  CPRINT("START---------------------------------------------");
  if (gate_way == nullptr) {
    CPRINT("ERROR : gate_way don't exist.");
    return -1;
  }
  if (gate_way->tcp_socket_ < 0) {
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
        doip_tcp_message.PrintPacketByte();
        uint16_t ecu_address = (static_cast<uint16_t>(doip_tcp_message.payload_.at(0)) << 8) 
                              | (static_cast<uint16_t>(doip_tcp_message.payload_.at(1))); 
        
        if (doip_tcp_message.payload_type_ != DoIpPayload::kRoutingActivationResponse &&
            ecu_map_.find(ecu_address) == ecu_map_.end()) {
          CPRINT("ERROR: ecu can not find.");
          std::cout << "ecu address: " << std::hex << ecu_address << std::endl;
          return -1;
        }
        
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


void DoIpClient::SetCallBack(DiagnosticMessageCallBack diag_msg_cb) {
  diag_msg_cb_ = diag_msg_cb;
}

int DoIpClient::SendRoutingActivationRequest(std::shared_ptr<GateWay> gate_way) {
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
  CPRINT("SocketWrite -------------");
  int re = SocketWrite(gate_way->tcp_socket_, routeing_act_req, &gate_way->vehicle_ip_);
  // printf("re: %d\n",re);
  if (-1 == re) {
    // timer->Stop();
    return -1;
  }
  std::unique_lock<std::mutex> lock(gate_way->route_mutex_);
  CPRINT("wait_for -----------------");
  std::cout << "GetRouteResponse: " << gate_way->GetRouteResponse() << std::endl;
  std::cout << "time_route_act_req_: " << time_route_act_req_ << std::endl;
  gate_way->route_response_cv_.wait_for(lock, std::chrono::seconds(time_route_act_req_), 
                                    [gate_way]{ return gate_way->GetRouteResponse();});
  
  CPRINT("wait for end ----------------------");
  // useconds_t inter_us = 200;
  // while (!route_respone && timer->IsRunning()) {
  //   usleep(inter_us);
  // }

  // sleep(5);
  if (gate_way->route_response) {
    gate_way->route_response = false;
    CPRINT("Routing Activation successful.");
    // TODO: 修改 发送 tester present
    if (gate_way->tcp_tester_present_flag_) {
      if (tester_present_timer) {
        delete tester_present_timer;
        tester_present_timer = nullptr;
      }
      tester_present_timer = new CTimer(
          std::bind(&DoIpClient::TimerCallBack, this, std::placeholders::_1));
      
      std::thread sendMsg(&DoIpClient::TesterPresentThread, this, gate_way);
      cycle_send_msg_handle_ = sendMsg.native_handle();
      sendMsg.detach();
    }
  } else {
    CPRINT("RoutingActivationResponse timeout.");
    return -1;
  }
  return 0;
}

ECUReplyCode DoIpClient::SendECUMeassage(uint16_t ecu_address, ByteVector uds) {
  // TODO: 检查address是ecu的地址
  // if (ecu_map_.find(ecu_address) == ecu_map_.end()) {
  //   CPRINT("ecu_address is not find !\n");
  //   return ECUReplyCode::kSendError;
  // }
  std::shared_ptr<ECU> ecu = ecu_map_[ecu_address];
  
  ecu->SetUds(uds);
  if (ecu_thread_map_.find(ecu_address) == ecu_thread_map_.end()) {
    std::thread send_ecu_msg_thread(&DoIpClient::SendDiagnosticMessageThread, this,
                                      ecu_address);
    ecu_thread_map_.insert(
        {ecu_address, send_ecu_msg_thread.native_handle()});
    send_ecu_msg_thread.detach();
  } else {
    // ecu->SetUds(uds);
    //  唤醒SendDiagnosticMessage
    ecu->SetSendAgain(true);
    ecu->send_again_cv_.notify_all();
    CPRINT("Send Again");
  }
  std::unique_lock<std::mutex> lock(ecu->reply_Status_mutex_);
  // TODO: 计时
  ecu->ecu_reply_status_cv_.wait_for(lock,std::chrono::milliseconds(time_route_act_req_));

  return ecu_reply_status_map_[ecu_address];
}

void DoIpClient::SendDiagnosticMessageThread(uint16_t ecu_address) {
  if (ecu_map_.find(ecu_address) == ecu_map_.end()) {
    CPRINT("ecu_address is not find !\n");
    return ;
  }
  
  std::shared_ptr<ECU> ecu = ecu_map_[ecu_address];
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
  if (ecu_map_.find(ecu_address) == ecu_map_.end()) {
    CPRINT("ecu_address is not find !\n");
    return ;
  }
  
  std::shared_ptr<ECU> ecu = ecu_map_[ecu_address];
  std::shared_ptr<GateWay> gateway = GateWays_map_[ecu_gateway_map_[ecu_address]];
  std::unique_lock<std::mutex> lock(ecu->write_mutex_);
  if (user_data.size() == 0) {
    CPRINT("Payload is NULL.");
    ecu_reply_status_map_.insert({ecu_address, ECUReplyCode::kSendError});
    ecu->ecu_reply_status_cv_.notify_all();
    return ;
  }
  if (gateway->tcp_socket_ <= 0) {
    CPRINT("TCP socket is error.");
    // std::cout << ecu->tcp_socket_ << std::endl;
    ecu_reply_status_map_.insert({ecu_address, ECUReplyCode::kSendError});
    ecu->ecu_reply_status_cv_.notify_all();
    return ;
  }
  if (!gateway->GetSocketStatus()) {
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
  // TODO: 修改了&gateway->vehicle_ip_
  int re = SocketWrite(gateway->tcp_socket_, diagnostic_msg, &gateway->vehicle_ip_);
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

void DoIpClient::SendTesterRequest(std::shared_ptr<GateWay> gate_way) {
  std::unique_lock<std::mutex> lock(gate_way->write_mutex_);
  if (gate_way->tcp_socket_ <= 0) {
    CPRINT("SendTesterRequest ERROR, TCP socket is not open.");
    return;
  }
  if (!gate_way->is_tcp_socket_open_) {
    CPRINT("SendTesterRequest ERROR, TCP is not connected.");
    return;
  }
  ByteVector user_data{0x3e, 0x80};
  DoIpPacket tester_present_request(DoIpPacket::kHost);
  tester_present_request.ConstructDiagnosticMessage(source_address_,
                                                    target_address_, user_data);

  if (gate_way->diagnostic_msg_ack) {
    gate_way->diagnostic_msg_ack = false;
  }
  // TODO: 修改了 &gate_way->vehicle_ip_
  int re = SocketWrite(gate_way->tcp_socket_, tester_present_request, &gate_way->vehicle_ip_);
  if (-1 == re) {

    PRINT("SendTesterRequest ERROR, {}", strerror(errno));
    return;
  }
  gate_way->tester_present_response_.wait_for(lock, std::chrono::seconds(time_tester_present_req_), 
                                              [gate_way]{ return gate_way->diagnostic_msg_ack;});


  if (gate_way->diagnostic_msg_ack) {
    // tester_present_timer->Stop();
    gate_way->diagnostic_msg_ack = false;
  } else {
    CPRINT("SendTesterRequest ACK timeout.");
    return;
  }
}

void DoIpClient::SetSourceAddress(uint16_t s_addr) { source_address_ = s_addr; }


void DoIpClient::GetAllGateWayAddress(std::vector<uint64_t>& gateway_addresses) {
  for (auto gateway : GateWays_map_) {
    gateway_addresses.push_back(gateway.second->gate_way_address_);
  }
}