#ifndef __COMMON_TOOLS_H__
#define __COMMON_TOOLS_H__

#include "DoIpPacket.h"

int SocketWrite(int socket, DoIpPacket &doipPacket,
                struct sockaddr_in *destinationAddress);
/**
 * @brief 封装接收数据报负载功能
 */
int SocketReadPayload(int socket, DoIpPacket &doipPacket);
/**
 * @brief 封装接收数据报头部功能
 */
int SocketReadHeader(int socket, DoIpPacket &doipPacket);

#endif