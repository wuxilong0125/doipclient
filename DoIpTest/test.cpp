#include "DoIpClient.h"
#include "VehicleTools.h"
#include <iostream>
DoIpClient client;



void diagMsgCb(unsigned char *usd, int len) {
  printf("reveive Diagnostic Message: \n");
  printf("len: %d\n", len);
  for (int i = 0; i < len; i++) {
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
  // client.SetEcuGateWayMaps(0x000a, 0x0010, 500);
	client.SetCallBack(diagMsgCb);
	FindTargetVehicleAddress(VehicleGateWays);
  // std::vector<std::shared_ptr<GateWay>> GateWays;
  // client.GetGateWayInfo(GateWays);
  for (auto gateway : VehicleGateWays) {
    int re = client.TcpHandler(gateway);
    if (-1 == re) {
      std::cout << "TcpHandler ERROR " << std::endl;
      return 0;
    }
    // 源地址要自己设置吗？
    client.SetSourceAddress(0x0064);
    client.SendRoutingActivationRequest(gateway);
    
    // TODO: 发送给网关就行， 不需要指定ECU
    client.SendECUMeassage(0x000a, uds_1);
    client.SendECUMeassage(0x000a, uds_2);
    sleep(2);
    // client.TesterPresentThread(gateway);
  }
  #else
  FindTargetVehicleAddress(VehicleGateWays);
  
  #endif
  
	return 0;	
}