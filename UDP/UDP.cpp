#include <WinSock2.h>
#include <stdio.h>
#include <stdint.h>
#include <malloc.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#pragma comment(lib,"ws2_32.lib")

#define m_RecvNum  15  //MMocap UDPsocket一次接收数据长度
#define s_RecvNum  164  //sEMG UDPsocket一次接收数据长度
#define myAddress "192.168.10.136" //本地IP地址
#define m_Port 1234 //接收运动数据端口
#define s_Port 1221 //接收表面肌电数据端口

int main()
{
	/* Winsock动态链接库初始化 */
	WORD wVersionRequested;
	WSADATA wsaData;
	int sErr;
	wVersionRequested = MAKEWORD(2, 0);
	sErr = WSAStartup(wVersionRequested, &wsaData);
	if (sErr != 0) {
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	/* 创建MMocap UDP套接字 */
	SOCKET m_sock = INVALID_SOCKET;
	m_sock = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET != m_sock) {
		printf("create MMocap socket successed\n");
	}
	else {
		printf("create MMocap socket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}
	/* 创建sEMG UDP套接字 */
	SOCKET s_sock = INVALID_SOCKET;
	s_sock = WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (INVALID_SOCKET != s_sock) {
		printf("create sEMG socket successed\n");
	}
	else {
		printf("create sEMG socket failed with error: %d\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	/* 绑定MMocap UDP套接字 */
	SOCKADDR_IN m_sockInfo;
	int m_bErr;
	m_sockInfo.sin_family = AF_INET;
	m_sockInfo.sin_port = htons(m_Port);
	m_sockInfo.sin_addr.s_addr = inet_addr(myAddress);
	m_bErr = bind(m_sock, (SOCKADDR *)&m_sockInfo, sizeof(m_sockInfo));
	if (m_bErr != 0) {
		printf("bind MMocap socket failed with error: %d\n", WSAGetLastError());
		closesocket(m_sock);
		WSACleanup();
		return 1;
	}
	else {
		printf("bind MMocap socket successed\n");
	}
	/* 绑定sEMG UDP套接字 */
	SOCKADDR_IN s_sockInfo;
	int s_bErr;
	s_sockInfo.sin_family = AF_INET;
	s_sockInfo.sin_port = htons(s_Port);
	s_sockInfo.sin_addr.s_addr = inet_addr(myAddress);
	s_bErr = bind(s_sock, (SOCKADDR *)&s_sockInfo, sizeof(s_sockInfo));
	if (s_bErr != 0) {
		printf("bind sEMG socket failed with error: %d\n", WSAGetLastError());
		closesocket(s_sock);
		WSACleanup();
		return 1;
	}
	else {
		printf("bind sEMG socket successed\n");
	}


	/* 将接收到的两种数据一起输出成txt文件形式 */
	FILE * m_HEXfp, * s_HEXfp;
	m_HEXfp = fopen("c:\\Users\\MSI-PC\\Desktop\\Visual data analysis for WIFI\\UDP_together\\m_HEXdata_UDP.txt", "w");
	s_HEXfp = fopen("c:\\Users\\MSI-PC\\Desktop\\Visual data analysis for WIFI\\UDP_together\\s_HEXdata_UDP.txt", "w");
	uint8_t m_RecvBuf[m_RecvNum];
	uint8_t s_RecvBuf[s_RecvNum];
	uint16_t DecBuf[2];
	uint8_t SendWait[2] = { 1,1 };  //发送等待采集信号
	uint8_t SendStart[3] = { 1,0.0 };  //发送启动采集信号
	uint8_t m_RecvReady[2];  //接收准备采集信号
	uint8_t s_RecvReady[2];  //接收准备采集信号
	uint32_t m_TimeStamp;
	uint32_t s_TimeStamp;
	memset(m_RecvBuf, 0, m_RecvNum);
	memset(s_RecvBuf, 0, s_RecvNum);
	int nSockInfoLen = sizeof(SOCKADDR);

	while (recvfrom(m_sock, (char *)m_RecvReady, 2, 0, (SOCKADDR *)&m_sockInfo, &nSockInfoLen) == 0);
	SendStart[1] = 1;
	printf("received MMocap ready message\n");
	int mwErr = sendto(m_sock, (char *)SendWait, 2, 0, (SOCKADDR *)&m_sockInfo, sizeof(SOCKADDR));
	if (mwErr == 0) {
		printf("sent failed!\n");
		return 1;
	}
	else {
		printf("sent wait message successful\n");
	}

	while (recvfrom(s_sock, (char *)s_RecvReady, 2, 0, (SOCKADDR *)&s_sockInfo, &nSockInfoLen) == 0);
	SendStart[2] = 2;
	printf("received sEMG ready message\n");
	int swErr = sendto(s_sock, (char *)SendWait, 2, 0, (SOCKADDR *)&s_sockInfo, sizeof(SOCKADDR));
	if (swErr == 0) {
		printf("sent failed!\n");
		return 1;
	}
	else {
		printf("sent wait message successful\n");
	}


	int msErr = sendto(m_sock, (char *)SendStart, 3, 0, (SOCKADDR *)&m_sockInfo, sizeof(SOCKADDR));
	if (msErr == 0) {
		printf("sent failed!\n");
		return 1;
	}
	else {
		printf("sent MMocap start message successful\n");
	}

	int ssErr = sendto(s_sock, (char *)SendStart, 3, 0, (SOCKADDR *)&s_sockInfo, sizeof(SOCKADDR));
	if (ssErr == 0) {
		printf("sent sEMG start message failed!\n");
		return 1;
	}
	else {
		printf("sent sEMG start message successful\n");
	}


	while (1) {
		int s_reErr = recvfrom(s_sock, (char *)s_RecvBuf, s_RecvNum, 0, (SOCKADDR *)&s_sockInfo, &nSockInfoLen);
		if (s_reErr == 0) {
			printf("sEMG data received failed!\n");
			return 1;
		}
		for (int s = 0; s < s_RecvNum - 4; s++) {
			fprintf(s_HEXfp, "%x ", s_RecvBuf[s]);
		}
		s_TimeStamp = (s_RecvBuf[s_RecvNum - 4] << 24) | (s_RecvBuf[s_RecvNum - 3] << 16) | (s_RecvBuf[s_RecvNum - 2] << 8) | s_RecvBuf[s_RecvNum - 1];
		fprintf(s_HEXfp, "%d\n", s_TimeStamp);

		int m_reErr = recvfrom(m_sock, (char *)m_RecvBuf, m_RecvNum, 0, (SOCKADDR *)&m_sockInfo, &nSockInfoLen);
		if (m_reErr == 0) {
			printf("MMocap data received failed!\n");
			return 1;
		}
		for (int m = 0; m < m_RecvNum - 4; m++) {
			fprintf(m_HEXfp, "%x ", m_RecvBuf[m]);
		}
		m_TimeStamp = (m_RecvBuf[m_RecvNum - 4] << 24) | (m_RecvBuf[m_RecvNum - 3] << 16) | (m_RecvBuf[m_RecvNum - 2] << 8) | m_RecvBuf[m_RecvNum - 1];
		fprintf(m_HEXfp, "%d\n", m_TimeStamp);


		//clock_t m_start_time = clock();
		
		/*clock_t m_end_time = clock();
		printf("MMocap time:%f\n", static_cast<double>(m_end_time - m_start_time) / CLOCKS_PER_SEC * 1000);*/

		//clock_t s_start_time = clock();

		

		//clock_t s_end_time = clock();
		//printf("sEMG time:%f\n", static_cast<double>(s_end_time - s_start_time) / CLOCKS_PER_SEC * 1000);
		/*printf("Total time:%f\n", static_cast<double>(s_end_time - m_start_time) / CLOCKS_PER_SEC * 1000);
		printf("Total time:%f\n", difftime(s_end_time , m_start_time));*/
	}

	fclose(m_HEXfp);
	fclose(s_HEXfp);
	closesocket(m_sock);
	closesocket(s_sock);
	WSACleanup();
	return 0;
}