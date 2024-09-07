#include <winsock2.h>
#include <ws2tcpip.h>

#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

#define PACKET_SIZE 1024

std::vector<SOCKET> clients;
std::mutex clientsMutex;

void BroadcastMessage(const std::string& message, SOCKET senderSocket) {
  std::lock_guard<std::mutex> lock(clientsMutex);
  for (SOCKET client : clients) {
    if (client != senderSocket) {
      send(client, message.c_str(), message.size(), 0);
    }
  }
}

void ClientHandler(SOCKET clientSocket) {
  char cBuffer[PACKET_SIZE] = {};
  while (true) {
    int receivedBytes = recv(clientSocket, cBuffer, PACKET_SIZE, 0);

    if (receivedBytes == SOCKET_ERROR) {
      std::cout << "클라이언트의 연결이 종료되었습니다." << std::endl;
      break;
    } else if (receivedBytes == 0) {
      std::cout << "Client disconnected" << std::endl;
      break;
    } else {
      std::string message(cBuffer, receivedBytes);
      std::cout << message << std::endl;
      BroadcastMessage(message, clientSocket);
    }
  }

  // Remove the client from the list and close the socket
  {
    std::lock_guard<std::mutex> lock(clientsMutex);
    auto it = std::remove(clients.begin(), clients.end(), clientSocket);
    clients.erase(it, clients.end());
  }

  closesocket(clientSocket);
}

int main() {
  WSADATA wsaData;
  SOCKET hListen;
  struct sockaddr_in serverAddr;
  int port = 4578;  // Example port number

  // Initialize Winsock
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    std::cout << "Failed to initialize Winsock" << std::endl;
    return 1;
  }

  // Create socket
  hListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (hListen == INVALID_SOCKET) {
    std::cout << "Failed to create socket" << std::endl;
    WSACleanup();
    return 1;
  }

  // Bind the socket
  serverAddr.sin_family = AF_INET;
  serverAddr.sin_addr.s_addr = INADDR_ANY;
  serverAddr.sin_port = htons(port);
  if (bind(hListen, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) ==
      SOCKET_ERROR) {
    std::cout << "Bind failed" << std::endl;
    closesocket(hListen);
    WSACleanup();
    return 1;
  }

  // Listen for incoming connections
  if (listen(hListen, SOMAXCONN) == SOCKET_ERROR) {
    std::cout << "Listen failed" << std::endl;
    closesocket(hListen);
    WSACleanup();
    return 1;
  }

  std::vector<std::thread> clientThreads;

  std::cout << "클라이언트의 연결을 기다리는 중입니다…" << std::endl;

  while (true) {
    SOCKADDR_IN tClntAddr = {};
    int iClntSize = sizeof(tClntAddr);
    SOCKET hClient = accept(hListen, (SOCKADDR*)&tClntAddr, &iClntSize);

    if (hClient == INVALID_SOCKET) {
      std::cout << "Accept failed" << std::endl;
      continue;
    }

    std::cout << "클라이언트가 연결되었습니다." << std::endl;

    // Add the client socket to the list
    {
      std::lock_guard<std::mutex> lock(clientsMutex);
      clients.push_back(hClient);
    }

    // Create a new thread to handle the client
    clientThreads.emplace_back(ClientHandler, hClient);
  }

  // Wait for all threads to complete
  for (auto& t : clientThreads) {
    if (t.joinable()) {
      t.join();
    }
  }

  closesocket(hListen);
  WSACleanup();
  return 0;
}