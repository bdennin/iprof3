
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

class UDPSocket
{
public:
	UDPSocket(uint32_t ip_addr, uint32_t port)
		: m_ip_addr(ip_addr)
		, m_port(port)
		, m_fd(INVALID_SOCKET)
	{
		if(InitSocket() < 0)
		{
			std::cout << "Error creating socket!\n";
		}
		else
		{
			std::cout << "Socket created.\n";
		}
	}

	~UDPSocket()
	{
		if(INVALID_SOCKET != m_fd)
		{
			shutdown(m_fd, SD_SEND);
			closesocket(m_fd);
			WSACleanup();
		}
	}

	int32_t Send(char* p_bytes, int32_t num_bytes)
	{
		int32_t byte_count = 0;

		if(INVALID_SOCKET != m_fd)
		{
			byte_count = sendto(m_fd, p_bytes, num_bytes, 0, (struct sockaddr*)&m_target, sizeof(m_target));

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

	int32_t Receive(char* p_bytes, int32_t max_bytes)
	{
		sockaddr_in addr;
		int32_t size_addr  = sizeof(addr);

		//int32_t byte_count = recvfrom(m_fd, p_bytes, max_bytes, 0, (struct sockaddr*)&addr, &size_addr);

		return 0;
	}

private:
	int32_t InitSocket()
	{
		// Initialize Winsock
		if(0 != WSAStartup(MAKEWORD(2, 2), &m_wsa))
		{
			std::cout << "WSAStartup failed." << std::endl;
			return -1;
		}

		// Create a socket for connecting to server
		m_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if(INVALID_SOCKET == m_fd)
		{
			std::cout << "socket failed with error: " << WSAGetLastError() << std::endl;
			//freeaddrinfo(result);
			WSACleanup();
			return -1;
		}

		// Do not wait
		timeval duration;
		duration.tv_sec  = 0;
		duration.tv_usec = 100000;

		if(SOCKET_ERROR == setsockopt(m_fd, SOL_SOCKET, SO_RCVTIMEO, (const char*)&duration, sizeof(duration)))
		{
			std::cout << "Failed to set socket time out.\n";
			return -1;
		}

		// Assign server address
		ZeroMemory(&m_target, sizeof(m_target));
		m_target.sin_family      = AF_INET;
		m_target.sin_addr.s_addr = htonl(m_ip_addr);
		m_target.sin_port        = htons(m_port);

		return 0;
	}

	uint32_t m_ip_addr;
	uint16_t m_port;
	SOCKET m_fd;
	struct sockaddr_in m_target;
	WSAData m_wsa;
};
