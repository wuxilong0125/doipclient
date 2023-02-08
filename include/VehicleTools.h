#ifndef __VEHICLE_TOOLS_H__
#define __VEHICLE_TOOLS_H__

#include <condition_variable>
#include <mutex>
#include <string>
#include <vector>

#include "DoIpPacket.h"
#include "GateWay.h"


int GetAllLocalIps(std::vector<std::string>& vehicle_ips);
int SetUdpSocket(const char *ip, int &udpSockFd);
DoIpNackCodes HandleUdpMessage(uint8_t msg[], ssize_t bytes_available, DoIpPacket &udp_packet);
void UdpHandler(int &udp_socket, std::vector<std::shared_ptr<GateWay>>& VehicleGateWays);
int SendVehicleIdentificationRequest(struct sockaddr_in *destination_address, int udp_socket);
int FindTargetVehicleAddress(std::vector<std::shared_ptr<GateWay>>& VehicleGateWays);

#endif