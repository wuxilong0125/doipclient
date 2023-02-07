#include "DoIpClient.h"
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


int main() {
  client.SetEcuGateWayMaps(0x000a, 0x0010, 500);
	client.SetCallBack(diagMsgCb);
  client.SetTimeOut(5,2,2,2,2);
  std::cout << "START-------------------------------------------" << std::endl;
	client.FindTargetVehicleAddress();
  std::vector<std::shared_ptr<GateWay>> GateWays;
  client.GetGateWayInfo(GateWays);
  for (auto gateway : GateWays) {
    int re = client.TcpHandler(gateway);
    if (-1 == re) {
      std::cout << "TcpHandler ERROR " << std::endl;
      return 0;
    }
    // 源地址要自己设置吗？
    client.SetSourceAddress(0x0064);
    client.SendRoutingActivationRequest(gateway);
    
    // 发送uds是对gateway的tcp socket发送吗？
    client.SendECUMeassage(0x000a, uds_2, false);
    client.SendECUMeassage(0x000a, uds_1, false);
    sleep(2);
    // client.TesterPresentThread(gateway);
  }
  
	return 0;	
}