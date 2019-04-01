
#undef UNICODE

#include <iostream>

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>

// Need to link with Ws2_32.lib
#pragma comment(lib, "Ws2_32.lib")

#define DEBUG 1
#define DEFAULT_PORT 44444

class client
{
public:
	client()
		: m_socket(INVALID_SOCKET)
	{
		if(init_socket() < 0)
		{
			std::cout << "Error creating socket!\n";
		}
		else
		{
			std::cout << "Socket created.\n";
		}
	}

	~client()
	{
		if(INVALID_SOCKET != m_socket)
		{
			shutdown(m_socket, SD_SEND);
			closesocket(m_socket);
			WSACleanup();
		}
	}

	int32_t init_socket()
	{
		// Initialize Winsock
		if(0 != WSAStartup(MAKEWORD(2, 2), &m_wsa))
		{
			std::cout << "WSAStartup failed." << std::endl;
			return -1;
		}

		// Create a socket for connecting to server
		m_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if(INVALID_SOCKET == m_socket)
		{
			std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
			//freeaddrinfo(result);
			WSACleanup();
			return -1;
		}

		// Set socket timeout
		timeval duration;
		duration.tv_sec  = 1;
		duration.tv_usec = 0;

		int32_t timeout;

		if(SOCKET_ERROR == setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&duration, sizeof(timeout)))
		{
			std::cout << "Failed to set socket time out.\n";
			return -1;
		}

		// Assign server address
		ZeroMemory(&m_server_addr, sizeof(m_server_addr));
		m_server_addr.sin_family      = AF_INET;
		m_server_addr.sin_addr.s_addr = htonl(0xC0A8006A);
		m_server_addr.sin_port        = htons(DEFAULT_PORT);

		return 0;
	}

	int32_t send(char* p_bytes, int32_t num_bytes)
	{
		int32_t byte_count = 0;

		if(INVALID_SOCKET != m_socket)
		{
			byte_count = sendto(m_socket, p_bytes, num_bytes, 0, (struct sockaddr*)&m_server_addr, sizeof(m_server_addr));

#ifdef DEBUG
			if(SOCKET_ERROR != byte_count)
			{
				std::cout << "Sent " << byte_count << " bytes to server: " << p_bytes << std::endl;
			}
			else
			{
				std::cout << "Error on socket: " << WSAGetLastError() << "\n";
			}
#endif
		}

		return byte_count;
	}

	int32_t receive(char* p_bytes, int32_t max_bytes)
	{
		sockaddr_in addr;
		int32_t size_addr  = sizeof(addr);
		int32_t byte_count = 0;

		do
		{
			byte_count += recvfrom(m_socket, p_bytes, max_bytes - byte_count, 0, (struct sockaddr*)&addr, &size_addr);

		} while(byte_count < max_bytes);

		return byte_count;
	}

private:
	SOCKET m_socket;
	struct sockaddr_in m_server_addr;
	WSAData m_wsa;
};
