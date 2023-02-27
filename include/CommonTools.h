#ifndef __COMMON_TOOLS_H__
#define __COMMON_TOOLS_H__

#include "DoIPPacket.h"

int SocketWrite(int socket, DoIPPacket &doipPacket,
                struct sockaddr_in *destinationAddress);
/**
 * @brief 封装接收数据报负载功能
 */
int SocketReadPayload(int socket, DoIPPacket &doipPacket);
/**
 * @brief 封装接收数据报头部功能
 */
int SocketReadHeader(int socket, DoIPPacket &doipPacket);

#endif