#include "DoIPClient.h"

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

DoIpClient::DoIpClient(int testPresentTime, bool testPresent = true) {
  m_testerPresentThreadTime = testPresentTime;
  m_tcpTesterPresentFlag = testPresent;
}
DoIpClient::~DoIpClient() {
  std::cout << "开始析构!!!!!" << std::endl;
  // m_ecuReplyStatusCondition.notify_all();
}

int DoIpClient::TcpHandler() {
  CPRINT("TcpHandler--------------------------------------START");
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
  std::thread receiveTcpMsgThread(&DoIpClient::HandleTcpMessage, this);
  receiveTcpMsgThread.detach();



  m_tcpConnected = true;
  return 0;
}

void DoIpClient::TesterPresentThread() {
  while (m_tcpConnected) {
    SendTesterRequest();
    std::this_thread::sleep_for(
        std::chrono::seconds(m_testerPresentThreadTime));
  }
}

void DoIpClient::CloseTcpConnection() {
  if (m_tcpConnected) {
    return;
  }

  // pthread_cancel(cycle_send_msg_handle_);
  // close(m_tcpSocket);
  if (-1 == shutdown(m_tcpSocket, SHUT_RDWR)) {
    return;
  }

  m_tcpConnected = false;
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

  while (m_tcpConnected) {
    
    DoIPPacket doip_tcp_message(DoIPPacket::kHost);
    int re = SocketReadHeader(m_tcpSocket, doip_tcp_message);
    std::cout << "*****************   re: " << re << std::endl;
    doip_tcp_message.Ntoh();
    if (re == -1) {
      CPRINT("SocketReadHeader is ERROR.");
      return -1;
    }
    if (0 == re) {
      CPRINT("判断TCP重连情况");
      m_reconnectTcpCounter++;
      if (m_reconnectTcpCounter == 5) {
        m_reconnectTcpCounter = 0;
        ReconnectServer();
      }
      continue;
    }

    uint8_t v_code = doip_tcp_message.VerifyPayloadType();

    if (v_code != DoIpNackCodes::kNoError) {
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
        // doip_tcp_message.PrintPacketByte();
        uint16_t replySourceAddress =
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
            //  通过路由回复得到目标网关地址
            m_targetAddress = replySourceAddress;
            m_routeResponse = true;
            m_routeResponseCondition.notify_all();

            break;
          }
          case DoIpPayload::kDiagnosticAck: {
            CPRINT("---DiagnosticAck---");
            if (replySourceAddress == m_targetAddress) {
              m_testPresentReplyAck = true;
              m_testerPresentResponseCondition.notify_all();
            } else {
              m_ecuMap.at(replySourceAddress)->SetDiagnosticAck(true);
              // diagnostic_msg_ack = true;
              m_ecuMap.at(replySourceAddress)->m_ecuReplyCondition.notify_all();
            }
            break;
          }
          case DoIpPayload::kDiagnosticNack: {
            m_ecuMap.at(replySourceAddress)->SetDiagnosticNack(true);
            // diagnostic_msg_nack = true;
            m_ecuMap.at(replySourceAddress)->m_ecuReplyCondition.notify_all();
            break;
          }
          case DoIpPayload::kDiagnosticMessage: {
            CPRINT("---DiagnosticMessage---");
            m_ecuMap.at(replySourceAddress)->SetDiagnosticResponse(true);
            m_ecuMap.at(replySourceAddress)->m_ecuReplyCondition.notify_all();
            if (m_ecuReplyCallBack) {
              m_ecuReplyCallBack(m_ecuMap.at(replySourceAddress)->GetAddress(), doip_tcp_message.m_payload, ECUReplyCode::kResp);
            }
          }
          default:
            break;
        }
      }
    }
  }
}

void DoIpClient::SetCallBack(EcuReplyCallBack callBack) {
  m_ecuReplyCallBack = callBack;
}

int DoIpClient::SendRoutingActivationRequest() {
  if (m_tcpSocket < 0) {
    CPRINT("TCP socket is error.");
    return -1;
  }
  DoIPPacket routeing_act_req(DoIPPacket::kHost);
  routeing_act_req.ConstructRoutingActivationRequest(m_sourceAddress);
  if (m_routeResponse) {
    m_routeResponse = false;
  }
  int re = SocketWrite(m_tcpSocket, routeing_act_req, &m_targetIp);
  // printf("re: %d\n",re);
  if (-1 == re) {
    // timer->Stop();
    return -1;
  }
  std::unique_lock<std::mutex> lock(m_routeMutex);
  CPRINT("wait_for -----------------");

  m_routeResponseCondition.wait_for(lock, std::chrono::seconds(m_routeActiveReqestTime),
                              [this] { return GetRouteResponse(); });
  CPRINT("wait for end ----------------------");
  if (m_routeResponse) {
    m_routeResponse = false;
    CPRINT("Routing Activation successful.");
    // TODO: 修改 发送 tester present
    if (m_tcpTesterPresentFlag) {
      std::thread sendMsg(&DoIpClient::TesterPresentThread, this);
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
  std::cout << "SendECUMeassage-------------------END" << std::endl;
}

void DoIpClient::SendDiagnosticMessageThread(uint16_t ecuAddress) {
  auto ecu = m_ecuMap.at(ecuAddress);
  while (true) {
    auto uds = ecu->PopECUMsg();
    SendDiagnosticMessage(uds.first, uds.second);
  }
}

void DoIpClient::SendDiagnosticMessage(uint16_t targetAddress, ByteVector userData) {
  auto ecu = m_ecuMap.at(targetAddress);
  std::unique_lock<std::mutex> lock(m_writeMutex);
  if (userData.size() == 0) {
    CPRINT("Payload is NULL.");
    if (m_ecuReplyCallBack) {
      m_ecuReplyCallBack(ecu->GetAddress(), userData, ECUReplyCode::kSendError);
    }
    return;
  }
  if (m_tcpSocket <= 0) {
    CPRINT("TCP socket is error.");
    if (m_ecuReplyCallBack) {
      m_ecuReplyCallBack(ecu->GetAddress(), userData, ECUReplyCode::kSendError);
    }
    return;
  }
  if (!m_tcpConnected) {
    CPRINT("TCP is not connected");
    if (m_ecuReplyCallBack) {
      m_ecuReplyCallBack(ecu->GetAddress(), userData, ECUReplyCode::kSendError);
    }
    return;
  }
  DoIPPacket diagnostic_msg(DoIPPacket::kHost);
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

  int re = SocketWrite(m_tcpSocket, diagnostic_msg, &m_targetIp);
  if (-1 == re) {
    if (m_ecuReplyCallBack) {
      m_ecuReplyCallBack(ecu->GetAddress(), userData, ECUReplyCode::kSendError);
    }
    return;
  }

  ecu->m_ecuReplyCondition.wait_for(
      lock, std::chrono::seconds(m_diagnosticMsgTime), [&ecu] {
        return (ecu->GetDiagnosticAck() || ecu->GetDiagnosticNack());
      });
  if (ecu->GetDiagnosticAck()) {
    ecu->SetDiagnosticAck(false);
    if (m_ecuReplyCallBack) {
      m_ecuReplyCallBack(ecu->GetAddress(), userData, ECUReplyCode::kACK);
    }
  } else if (ecu->GetDiagnosticNack()) {
    ecu->SetDiagnosticNack(false);
    if (m_ecuReplyCallBack) {
      m_ecuReplyCallBack(ecu->GetAddress(), userData, ECUReplyCode::kNACK);
    }
  } else {
    CPRINT("DiagnosticMessage ACK timeout.");
    if (m_ecuReplyCallBack) {
      m_ecuReplyCallBack(ecu->GetAddress(), userData, ECUReplyCode::kTimeOut);
    }
  }

  return;
}

void DoIpClient::SendTesterRequest() {
  std::unique_lock<std::mutex> lock(m_writeMutex);
  if (m_tcpSocket <= 0) {
    CPRINT("SendTesterRequest ERROR, TCP socket is not open.");
    return;
  }
  if (!m_tcpConnected) {
    CPRINT("SendTesterRequest ERROR, TCP is not connected.");
    return;
  }
  ByteVector user_data{0x3e, 0x80};
  DoIPPacket testerPresentRequest(DoIPPacket::kHost);
  testerPresentRequest.ConstructDiagnosticMessage(m_sourceAddress,
                                                    m_targetAddress, user_data);

  if (m_testPresentReplyAck) {
    m_testPresentReplyAck = false;
  }
  int re = SocketWrite(m_tcpSocket, testerPresentRequest, &m_targetIp);
  if (-1 == re) {
    PRINT("SendTesterRequest ERROR, {}", strerror(errno));
    return;
  }
  m_testerPresentResponseCondition.wait_for(
      lock, std::chrono::seconds(m_diagnosticMsgTime),
      [this] { return GetTestPresentReplyAck(); });

  if (m_testPresentReplyAck) {
    m_testPresentReplyAck = false;
  } else {
    CPRINT("SendTesterRequest ACK timeout.");
  }
}

void DoIpClient::SetSourceAddress(uint16_t s_addr) { m_sourceAddress = s_addr; }

void DoIpClient::GetAllGateWayAddress(
    std::vector<uint64_t>& gatewayAddressVec) {
  for (auto gateway : m_gateWaysMap) {
    gatewayAddressVec.push_back(gateway.second->gate_way_address_);
  }
}