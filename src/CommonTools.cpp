#include "CommonTools.h"
#include <arpa/inet.h>
#include <ifaddrs.h>

int SocketWrite(int socket, DoIpPacket &doip_packet,
                struct sockaddr_in *destination_address) {
  if (socket < 0) {
    // DEBUG("SocketWrite's socket param ERROR.\n");
    return -1;
  }
  DoIpPacket::ScatterArray scatter_array(doip_packet.GetScatterArray());
  struct msghdr message_header;
  if (destination_address != nullptr) {
    // printf("broadaddr: %s \n", inet_ntoa(destination_address->sin_addr));
    message_header.msg_name = destination_address;
    message_header.msg_namelen = sizeof(sockaddr_in);
  } else {
    // CPRINT("destination_address is null.");
    return -1;
  }
  message_header.msg_iov = scatter_array.begin();
  message_header.msg_iovlen = scatter_array.size();
  message_header.msg_control = nullptr;
  message_header.msg_controllen = 0;
  message_header.msg_flags = 0;

  // Briefly switch DoIpPacket to network byte order
  doip_packet.Hton();

  ssize_t bytesSent{sendmsg(socket, &message_header, 0)};
  // printf("bytesSent: %d\n",bytesSent);
  doip_packet.Ntoh();

  if (bytesSent < 0) {
    // PRINT("sendmsg error : %d\n", errno);

    return -1;
  } else if (((size_t)bytesSent) <
             (doip_packet.payload_length_ + kDoIp_HeaderTotal_length)) {
    // DEBUG("SocketWrite bytesSent is smaller than packet Length.\n");
    return -1;
  }
  return 0;
}

int SocketReadPayload(int socket, DoIpPacket &doip_packet) {
  if (socket < 0) {
    // CPRINT("socket is error");
    return -1;
  }
  DoIpPacket::PayloadLength buffer_fill(0);

  // while ((buffer_fill < doip_packet.payload_.size()) && timer->IsRunning()) {
    while ((buffer_fill < doip_packet.payload_.size())) {
    ssize_t bytedRead{recv(socket, &(doip_packet.payload_.at(buffer_fill)),
                           (doip_packet.payload_.size() - buffer_fill), 0)};
    if (bytedRead < 0) {
      if ((errno == EWOULDBLOCK) || (errno == EAGAIN) || (errno == EINTR)) {
        continue;
      }
      // CPRINT("recv ERROR :" + std::to_string(errno));
      break;
    }
    if (bytedRead == 0) {
      break;
    }
    buffer_fill += bytedRead;
  };
  if (buffer_fill != doip_packet.payload_.size()) {
    // CPRINT("ERROR: recv 超时！");
    return -1;
  }
  return 0;
}

int SocketReadHeader(int socket, DoIpPacket &doip_packet) {
  doip_packet.SetPayloadLength(0);
  DoIpPacket::PayloadLength expected_payload_length{
      doip_packet.payload_length_};
  DoIpPacket::ScatterArray scatter_arry(doip_packet.GetScatterArray());
  struct msghdr message_header;
  message_header.msg_name = NULL;
  message_header.msg_namelen = 0;
  message_header.msg_iov = scatter_arry.begin();
  message_header.msg_iovlen = scatter_arry.size();
  message_header.msg_control = nullptr;
  message_header.msg_controllen = 0;
  message_header.msg_flags = 0;
  doip_packet.Hton();
  ssize_t bytesReceived{recvmsg(socket, &message_header, 0)};
  if (bytesReceived == 0) return 0;
  if (bytesReceived < 0) {
    // CPRINT("ERROR: " + std::to_string(errno));
    return -1;
  }
  if (bytesReceived < ((ssize_t)kDoIp_HeaderTotal_length)) {
    // CPRINT(" Incomplete DoIp Header received.");
    return -1;
  }

  if (bytesReceived > ((ssize_t)kDoIp_HeaderTotal_length)) {
    // CPRINT("Extra Payload bytes read for DoIp Packet.");
    return -1;
  }

  return bytesReceived;
}