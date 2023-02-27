#include "DoIPClient.h"
#include "VehicleTools.h"
#include <iostream>


void diagMsgCb(uint16_t ecuAddress, ByteVector uds, ECUReplyCode replyCode) {
  std::cout << "********************ECU回复回调函数********************" << std::endl;
  printf("ecu address: 0x%02X%02X\n", (ecuAddress >> 8), (ecuAddress & 0xff));
  
  if(replyCode == ECUReplyCode::kACK) {
    std::cout << "ecu reply code: ACK" << std::endl;
    printf("Send Uds Message: ");
    for (int i = 0; i < uds.size(); i++) {
      printf("0x%02X ", uds[i]);
    }
    printf("\n");
  }else if (replyCode == ECUReplyCode::kNACK) {
    std::cout << "ecu reply code: NACK" << std::endl;
    printf("Send Uds Message: ");
    for (int i = 0; i < uds.size(); i++) {
      printf("0x%02X ", uds[i]);
    }
    printf("\n");
  }else if (replyCode == ECUReplyCode::kTimeOut) {
    std::cout << "ecu reply code: TimeOut" << std::endl;
    printf("Send Uds Message: ");
    for (int i = 0; i < uds.size(); i++) {
      printf("0x%02X ", uds[i]);
    }
    printf("\n");
  }else if(replyCode == ECUReplyCode::kResp) {
    printf("reveive Diagnostic Message: ");
    for (int i = 0; i < uds.size(); i++) {
      printf("0x%02X ", uds[i]);
    }
    printf("\n");
  }
  std::cout << "*************************************************************" << std::endl;
}

ByteVector uds_1 = {0x10, 0x01};
ByteVector uds_2 = {0x10, 0x01};

std::vector<std::shared_ptr<GateWay>> VehicleGateWays;
int main() {
  std::cout << "START-------------------------------------------" << std::endl;
  
	FindTargetVehicleAddress(VehicleGateWays);
  std::cout << "GateWays size: " << VehicleGateWays.size() << std::endl;
  for (auto gateway : VehicleGateWays) {
    DoIpClient* client = new DoIpClient(2, true);
	  client->SetCallBack(diagMsgCb);
    client->SetTargetIp(gateway);
    int re = client->TcpHandler();
    if (-1 == re) {
      std::cout << "TcpHandler ERROR " << std::endl;
      return 0;
    }
    client->SetSourceAddress(0x0064);
    client->SendRoutingActivationRequest();
    int k = 10;
    while(k --) {
      std::cout << "FIRST-----------------------------------" << std::endl;
      client->SendECUMeassage(0x0001, uds_1);
      client->SendECUMeassage(0x000a, uds_2);
      
      std::cout << "SECOND-----------------------------------" << std::endl;
    }
    sleep(10);
  }
  
  std::cout << "COUT--------------------------------------------" << std::endl;
	return 0;
}