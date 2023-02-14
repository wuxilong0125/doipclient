#ifndef __VEHICLE_TOOLS_H__
#define __VEHICLE_TOOLS_H__

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

#include "DoIpPacket.h"
#include "GateWay.h"


int GetAllLocalIps(std::vector<std::string>& vehicleIps);
int SetUdpSocket(const char *ip, int &udpSockFd);
DoIpNackCodes HandleUdpMessage(uint8_t msg[], ssize_t bytesAvailable, DoIpPacket &udpPacket);
void UdpHandler(int &udpSocket, std::vector<std::shared_ptr<GateWay>>& vehicleGateWays);
int SendVehicleIdentificationRequest(struct sockaddr_in *destinationAddress, int udpSocket);
int FindTargetVehicleAddress(std::vector<std::shared_ptr<GateWay>>& vehicleGateWays);

#endif