#ifndef __COMMON_TOOLS_H__
#define __COMMON_TOOLS_H__

#include "DoIpPacket.h"

int SocketWrite(int socket, DoIpPacket &doip_packet,
                struct sockaddr_in *destination_address);
/**
 * @brief 封装接收数据报负载功能
 */
int SocketReadPayload(int socket, DoIpPacket &doip_packet);
/**
 * @brief 封装接收数据报头部功能
 */
int SocketReadHeader(int socket, DoIpPacket &doip_packet);

#endif