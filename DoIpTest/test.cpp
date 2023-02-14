#include "DoIpClient.h"
#include "VehicleTools.h"
#include <iostream>
DoIpClient client;



void diagMsgCb(ByteVector usd) {
  printf("reveive Diagnostic Message: \n");

  for (int i = 0; i < usd.size(); i++) {
    printf("0x%02X ", usd[i]);
  }
  printf("\n");
}
ByteVector uds_1 = {0x10, 0x01};
ByteVector uds_2 = {0x3e, 0x80};

std::vector<std::shared_ptr<GateWay>> VehicleGateWays;
int main() {
  std::cout << "START-------------------------------------------" << std::endl;
  #if 1
	client.SetCallBack(diagMsgCb);
	FindTargetVehicleAddress(VehicleGateWays);
  for (auto gateway : VehicleGateWays) {
    client.SetTargetIp(gateway);
    int re = client.TcpHandler();
    if (-1 == re) {
      std::cout << "TcpHandler ERROR " << std::endl;
      return 0;
    }
    client.SetSourceAddress(0x0064);
    client.SendRoutingActivationRequest();
    int k = 10;
    // while(k --) {
      client.SendECUMeassage(0x000a, uds_1);
      client.SendECUMeassage(0x000a, uds_2);
      sleep(1);
    // }
  }
  #else
  FindTargetVehicleAddress(VehicleGateWays);
  #endif

	return 0;	
}