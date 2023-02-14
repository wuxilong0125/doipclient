#include "DoIpClient.h"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <unistd.h>

#include <condition_variable>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include "CommonTools.h"
#include "MultiByteType.h"
#include "VehicleTools.h"

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
#define PRINT_V(_1, _2, _3) printf(_1);
for (auto v : _2) {
  printf(_3, v);
}
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

int DoIpClient::TcpHandler() {
  if (inet_ntoa(m_targetIp.sin_addr) == nullptr) {
    DEBUG("Get ServerIP ERROR.\n");
    return -1;
  }
  m_tcpSocket = socket(AF_INET, SOCK_STREAM, 0);

  if (m_tcpSocket <= 0) {
    return -1;
  }
  struct sockaddr_in tcpSocketAddr;
  tcpSocketAddr.sin_family = AF_INET;
  tcpSocketAddr.sin_port = htons(kPort);
  tcpSocketAddr.sin_addr = m_targetIp.sin_addr;
  int re = connect(m_tcpSocket, (struct sockaddr*)&tcpSocketAddr,
                   sizeof(tcpSocketAddr));
  if (-1 == re) {
    return -1;
  }
  std::thread receive_tcp_msg_thread_(&DoIpClient::HandleTcpMessage, this);
  pthread_t receive_handle = receive_tcp_msg_thread_.native_handle();
  m_receiveTcpThreadHandles.push_back(receive_handle);
  receive_tcp_msg_thread_.detach();

  m_TcpConnected = true;
  return 0;
}

void DoIpClient::TesterPresentThread() {
  while (m_TcpConnected) {
    SendTesterRequest();
    std::this_thread::sleep_for(
        std::chrono::seconds(m_testerPresentThreadTime));
  }
}

void DoIpClient::CloseTcpConnection() {
  if (m_TcpConnected) {
    return;
  }

  // pthread_cancel(cycle_send_msg_handle_);
  // close(m_tcpSocket);
  if (-1 == shutdown(m_tcpSocket, SHUT_RDWR)) {
    return;
  }

  m_TcpConnected = false;
}

void DoIpClient::ReconnectServer() {
  CloseTcpConnection();
  TcpHandler();
}

int DoIpClient::HandleTcpMessage() {
  CPRINT("START---------------------------------------------");
  if (m_tcpSocket < 0) {
    CPRINT("ERROR : No Tcp Socket set.");
    return -1;
  }

  while (m_TcpConnected) {
    DoIpPacket doip_tcp_message(DoIpPacket::kHost);
    int re = SocketReadHeader(m_tcpSocket, doip_tcp_message);
    doip_tcp_message.Ntoh();
    if (re == -1) {
      CPRINT("SocketReadHeader is ERROR.");
      return -1;
      continue;
    }
    if (0 == re) {
      m_reconnectTcpCounter++;
      if (m_reconnectTcpCounter == 5) {
        m_reconnectTcpCounter = 0;
        ReconnectServer();
      }
      continue;
    }

    uint8_t v_code = doip_tcp_message.VerifyPayloadType();

    if (v_code != 0xFF) {
      CPRINT("PayloadType ERROR: " + std::to_string(v_code));
      continue;
    }

    if (doip_tcp_message.m_payloadLength != 0) {
      doip_tcp_message.SetPayloadLength(doip_tcp_message.m_payloadLength, true);
      // 接收负载
      int re = SocketReadPayload(m_tcpSocket, doip_tcp_message);
      if (re != 0) {
        continue;
      } else {
        doip_tcp_message.PrintPacketByte();
        uint16_t ecu_address =
            (static_cast<uint16_t>(doip_tcp_message.m_payload.at(0)) << 8) |
            (static_cast<uint16_t>(doip_tcp_message.m_payload.at(1)));

        switch (doip_tcp_message.m_payloadType) {
          case DoIpPayload::kRoutingActivationResponse: {
            CPRINT("kRoutingActivationResponse");
            // timer->Stop();
            if (doip_tcp_message.m_payload.at(4) != 0x10) {
              CPRINT("Routing Activation ERROR.");
              CloseTcpConnection();
              break;
            }
            CPRINT("route_respone is true!");
            m_routeResponse = true;
            m_routeResponseCondition.notify_all();

            break;
          }
          case DoIpPayload::kDiagnosticAck: {
            CPRINT("---DiagnosticAck---");
            m_ecuMap.at(ecu_address)->SetDiagnosticAck(true);
            // diagnostic_msg_ack = true;
            m_ecuMap.at(ecu_address)->ecu_reply_cv_.notify_all();
            break;
          }
          case DoIpPayload::kDiagnosticNack: {
            m_ecuMap.at(ecu_address)->SetDiagnosticNack(true);
            // diagnostic_msg_nack = true;
            m_ecuMap.at(ecu_address)->ecu_reply_cv_.notify_all();
            break;
          }
          case DoIpPayload::kDiagnosticMessage: {
            CPRINT("---DiagnosticMessage---");
            m_ecuMap.at(ecu_address)->SetDiagnosticResponse(true);
            // diagnostic_msg_response = true;
            m_ecuMap.at(ecu_address)->ecu_reply_cv_.notify_all();
            // timer->Stop();
            // TODO: 修改
            if (m_diagnosticMsgCallBack) {
              m_diagnosticMsgCallBack(doip_tcp_message.m_payload);
            }
          }
          default:
            break;
        }
      }
    }
  }
}

void DoIpClient::SetCallBack(DiagnosticMessageCallBack diagnosticMsgCallBack) {
  m_diagnosticMsgCallBack = diagnosticMsgCallBack;
}

int DoIpClient::SendRoutingActivationRequest() {
  CPRINT("run----------");

  if (m_tcpSocket < 0) {
    CPRINT("TCP socket is error.");
    return -1;
  }
  DoIpPacket routeing_act_req(DoIpPacket::kHost);
  routeing_act_req.ConstructRoutingActivationRequest(m_sourceAddress);
  if (m_routeResponse) {
    m_routeResponse = false;
  }
  // CTimer::Clock::duration timeout =
  // std::chrono::seconds(time_route_act_req_); timer->StartOnce(timeout, true);
  CPRINT("SocketWrite -------------");
  int re = SocketWrite(m_tcpSocket, routeing_act_req, &m_targetIp);
  // printf("re: %d\n",re);
  if (-1 == re) {
    // timer->Stop();
    return -1;
  }
  std::unique_lock<std::mutex> lock(m_RouteMutex);
  CPRINT("wait_for -----------------");

  m_routeResponseCondition.wait_for(lock, std::chrono::seconds(m_routeActiveReqestTime),
                              [this] { return GetRouteResponse(); });
  CPRINT("wait for end ----------------------");
  if (m_routeResponse) {
    m_routeResponse = false;
    CPRINT("Routing Activation successful.");
    // TODO: 修改 发送 tester present
    if (m_tcpTesterPresentFlag) {
      if (m_testerPresentTimer) {
        delete m_testerPresentTimer;
        m_testerPresentTimer = nullptr;
      }
      m_testerPresentTimer = new CTimer(
          std::bind(&DoIpClient::TimerCallBack, this, std::placeholders::_1));

      std::thread sendMsg(&DoIpClient::TesterPresentThread, this);
      m_cycleSendMsgHandle = sendMsg.native_handle();
      sendMsg.detach();
    }
  } else {
    CPRINT("RoutingActivationResponse timeout.");
    return -1;
  }
  return 0;
}

void DoIpClient::SendECUMeassage(uint16_t ecuAddress, ByteVector uds) {
  std::cout << "SendECUMeassage--------------------------------" << std::endl;
  if (m_ecuMap.find(ecuAddress) == m_ecuMap.end()) {
    auto p = std::make_shared<ECU>(ecuAddress, uds);
    m_ecuMap.insert({ecuAddress, p});
    CPRINT("-----------------------");
  } else {
    m_ecuMap.at(ecuAddress)->SetUds(uds);
  }
  m_ecuMap.at(ecuAddress)->PushECUMsg({ecuAddress, uds});
  if (m_ecuThreadMap.find(ecuAddress) == m_ecuThreadMap.end()) {
    std::thread send_ecu_msg_thread(&DoIpClient::SendDiagnosticMessageThread,
                                    this, ecuAddress);
    m_ecuThreadMap.insert({ecuAddress, send_ecu_msg_thread.native_handle()});
    send_ecu_msg_thread.detach();
  } 
}

void DoIpClient::SendDiagnosticMessageThread(uint16_t ecuAddress) {
  auto ecu = m_ecuMap.at(ecuAddress);
  while (true) {
    auto uds = ecu->PopECUMsg();
    SendDiagnosticMessage(uds.first, uds.second);
  }

  // std::unique_lock<std::mutex> lock( ecu->thread_mutex_);
  // while(true) {
  //   ecu->send_again_cv_.wait(lock, [this, ecu]{ return ecu->GetSendAgain();
  //   }); std::unique_lock<std::mutex> lk(ecu->reply_Status_mutex_);
  //   ecu->ecu_reply_status_cv_.wait(lk, [ecu]{ return ecu->GetReply(); });
  //   ecu->SetReply(false);
  //   ecu->SetSendAgain(false);
  //   ByteVector uds;
  //   ecu->SetUds(uds);
  //   SendDiagnosticMessage(ecu_address, uds);
  // }
}

void DoIpClient::SendDiagnosticMessage(uint16_t targetAddress, ByteVector userData) {
  auto ecu = m_ecuMap.at(targetAddress);
  std::unique_lock<std::mutex> lock(ecu->write_mutex_);
  if (userData.size() == 0) {
    CPRINT("Payload is NULL.");
    m_ecuReplyStatusMap.insert({ecu->GetAddress(), ECUReplyCode::kSendError});
    // ecu->ecu_reply_status_cv_.notify_all();
    return;
  }
  if (m_tcpSocket <= 0) {
    CPRINT("TCP socket is error.");
    // std::cout << ecu->m_tcpSocket << std::endl;
    m_ecuReplyStatusMap.insert({ecu->GetAddress(), ECUReplyCode::kSendError});
    // ecu->ecu_reply_status_cv_.notify_all();
    return;
  }
  if (!m_TcpConnected) {
    CPRINT("TCP is not connected");
    m_ecuReplyStatusMap.insert({ecu->GetAddress(), ECUReplyCode::kSendError});
    // ecu->ecu_reply_status_cv_.notify_all();
    return;
  }
  DoIpPacket diagnostic_msg(DoIpPacket::kHost);
  diagnostic_msg.ConstructDiagnosticMessage(m_sourceAddress, ecu->GetAddress(),
                                            userData);
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
  int re = SocketWrite(m_tcpSocket, diagnostic_msg, &m_targetIp);
  if (-1 == re) {
    m_ecuReplyStatusMap.insert({ecu->GetAddress(), ECUReplyCode::kSendError});
    // ecu->ecu_reply_status_cv_.notify_all();
    return;
  }

  ecu->ecu_reply_cv_.wait_for(
      lock, std::chrono::seconds(m_diagnosticMsgTime), [&ecu] {
        return (ecu->GetDiagnosticAck() || ecu->GetDiagnosticNack() ||
                ecu->GetDiagnosticResponse());
      });
  ecu->SetReply(true);
  if (ecu->GetDiagnosticAck()) {
    m_ecuReplyStatusMap.insert({ecu->GetAddress(), ECUReplyCode::kACK});
    ecu->SetDiagnosticAck(false);
    // ecu->ecu_reply_status_cv_.notify_all();
  } else if (ecu->GetDiagnosticNack()) {
    m_ecuReplyStatusMap.insert({ecu->GetAddress(), ECUReplyCode::kNACK});
    ecu->SetDiagnosticNack(false);
    // ecu->ecu_reply_status_cv_.notify_all();
  } else if (ecu->GetDiagnosticResponse()) {
    m_ecuReplyStatusMap.insert({ecu->GetAddress(), ECUReplyCode::kResp});
    ecu->SetDiagnosticNack(false);
    // ecu->ecu_reply_status_cv_.notify_all();
  } else {
    CPRINT("DiagnosticMessage ACK timeout.");
    m_ecuReplyStatusMap.insert({ecu->GetAddress(), ECUReplyCode::kTimeOut});
    // ecu->ecu_reply_status_cv_.notify_all();
  }

  return;
}

void DoIpClient::SendTesterRequest() {
  std::unique_lock<std::mutex> lock(m_writeMutex);
  if (m_tcpSocket <= 0) {
    CPRINT("SendTesterRequest ERROR, TCP socket is not open.");
    return;
  }
  if (!m_TcpConnected) {
    CPRINT("SendTesterRequest ERROR, TCP is not connected.");
    return;
  }
  ByteVector user_data{0x3e, 0x80};
  DoIpPacket tester_present_request(DoIpPacket::kHost);
  tester_present_request.ConstructDiagnosticMessage(m_sourceAddress,
                                                    m_targetAddress, user_data);

  if (m_diagnosticMsgAck) {
    m_diagnosticMsgAck = false;
  }
  // TODO: 修改了 &gate_way->vehicle_ip_
  int re = SocketWrite(m_tcpSocket, tester_present_request, &m_targetIp);
  if (-1 == re) {
    PRINT("SendTesterRequest ERROR, {}", strerror(errno));
    return;
  }
  m_testerPresentResponseCondition.wait_for(
      lock, std::chrono::seconds(m_testerPresentRequestTime),
      [this] { return GetDiagnosticAck(); });

  if (m_diagnosticMsgAck) {
    // tester_present_timer->Stop();
    m_diagnosticMsgAck = false;
  } else {
    CPRINT("SendTesterRequest ACK timeout.");
    return;
  }
}

void DoIpClient::SetSourceAddress(uint16_t s_addr) { m_sourceAddress = s_addr; }

void DoIpClient::GetAllGateWayAddress(
    std::vector<uint64_t>& gatewayAddressVec) {
  for (auto gateway : m_gateWaysMap) {
    gatewayAddressVec.push_back(gateway.second->gate_way_address_);
  }
}