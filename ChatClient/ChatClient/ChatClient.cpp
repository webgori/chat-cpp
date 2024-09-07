#include <WinSock2.h>

#include <iostream>
#include <string>
#include <thread>
#include <limits>

#pragma comment(lib, "ws2_32")

#define PORT 4578
#define PACKET_SIZE 1024
// #define SERVER_IP "192.168.1.207"
#define SERVER_IP "127.0.0.1"

void ReceiveMessages(SOCKET hSocket) {
  char cBuffer[PACKET_SIZE] = {};
  while (true) {
    int receivedBytes = recv(hSocket, cBuffer, PACKET_SIZE, 0);
    if (receivedBytes == SOCKET_ERROR) {
      std::cerr << "Error receiving data" << std::endl;
      break;
    } else if (receivedBytes == 0) {
      std::cout << "Server closed the connection" << std::endl;
      break;
    } else {
      cBuffer[receivedBytes] = '\0';  // Null-terminate the received data
      std::cout << "Recv Msg : " << cBuffer << std::endl;
    }
  }
}

char *concatNickname(char *nickname, char *rawMessage) {
  char messageWithNickname[10000];

  int nicknameSize = strlen(nickname);
  int rawMessageSize = strlen(rawMessage);

  int messageWithNicknameIdx = 0;

  for (int i = 0; i < nicknameSize; i++) {
    messageWithNickname[messageWithNicknameIdx] = nickname[i];
    messageWithNicknameIdx++;
  }

  messageWithNickname[messageWithNicknameIdx] = ':';
  messageWithNicknameIdx++;

  messageWithNickname[messageWithNicknameIdx] = ' ';
  messageWithNicknameIdx++;

  for (int i = 0; i < rawMessageSize; i++) {
    messageWithNickname[messageWithNicknameIdx] = rawMessage[i];
    messageWithNicknameIdx++;
  }

  messageWithNickname[messageWithNicknameIdx] = '\0';

  return messageWithNickname;
}

int main() {
  WSADATA wsaData;
  WSAStartup(MAKEWORD(2, 2), &wsaData);

  SOCKET hSocket;
  hSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

  SOCKADDR_IN tAddr = {};
  tAddr.sin_family = AF_INET;
  tAddr.sin_port = htons(PORT);
  tAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

  connect(hSocket, (SOCKADDR *)&tAddr, sizeof(tAddr));

  std::thread recvThread(ReceiveMessages, hSocket);

  std::cout << "채팅 서버에 연결되었습니다." << std::endl << std::endl;

  char nickname[10000];
  std::cout << "닉네임을 입력해주세요: ";
  std::cin >> nickname;

  std::cin.ignore();

  //char rawMessage[1024];
  std::string rawMessage;

  while (true) {
    std::cout << nickname << ": ";
    // std::cin >> rawMessage;
    std::getline(std::cin, rawMessage);

    char rawMessage1[1024];
    
    strcpy(rawMessage1, rawMessage.c_str());
    char *message = concatNickname(nickname, rawMessage1);

    send(hSocket, message, strlen(message), 0);
  }

  closesocket(hSocket);

  WSACleanup();
  return 0;
}
