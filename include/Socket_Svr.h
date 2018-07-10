#pragma once
#pragma once
#include <thread>
#include <mutex>
#include "MyList.h"
#include <map>
#include <iostream>
#include <vector>
#include <string.h>
#include "Service_data.h"

#ifdef LINUX_PROJ
#include <sys/epoll.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>
#else
#include "stdafx.h"
#endif


#ifndef LINUX_PROJ
//��ɼ�
typedef struct _complate_key
{
	SOCKET m_socket;
	std::string m_ip;
}TCOM_KEY, *PCOM_KEY;
typedef enum _Operator_Data
{
	ACCEPT_CONN,
	RECV_MSG,
	SEND_MSG
} TOperatorType;

typedef struct _IO_Operater_Data
{
	OVERLAPPED m_overlapped;
	WSABUF m_databuf;
	CHAR m_buffer[1024];
	TOperatorType m_type;
	SOCKET m_socket;
	DWORD m_len;
}TOPER_DATA, *POPER_DATA;


#endif
/*
struct Svr_res
{
#ifdef LINUX_PROJ
	int m_socket;
#else
	SOCKET m_socket;
#endif
	std::string m_ip;
	std::string m_msg;
	int m_errcode;
	//1 ������ 0 ���� -1 socket�쳣��������
	int m_status;
};
*/
/*
* @brief   tcp������ԭ�� ʹ���������� ���ݽ��ն��С����ݷ��Ͷ��С��ͻ���״̬����
*          �ֱ�ά���������ݡ��������ݡ��ͻ��˽������������Ⱪ¶���ݶ�д��״̬��ȡ�ӿ�
*/

class Socket_Svr
{
private:
#ifdef LINUX_PROJ
    typedef int TSOCKET;
#else
    typedef SOCKET TSOCKET;
#endif
    Client_interpreter m_recv_msg;
    Client_interpreter m_send_msg;
    Client_interpreter m_connect_msg;

    void Free_Client ( TSOCKET sck )
    {
        m_connect_msg.delete_client(sck); 
        m_recv_msg.delete_client(sck); 
        m_send_msg.delete_client(sck); 
    }

    bool IsSckUsefull ( TSOCKET sck )
    {
        //��m_connect_msg ���е���ԴΪ׼
        return m_connect_msg.IsSCKUsefull(sck); 
    }
    void Add_Client ( TSOCKET sck,std::string ip )
    {
        m_connect_msg.add_client(sck,ip); 
        m_recv_msg.add_client(sck,ip); 
        m_send_msg.add_client(sck,ip); 
    }

    void Free_All_Client()
    {
        m_connect_msg.close_all();
        m_recv_msg.delete_all();
        m_send_msg.delete_all();
    }
	int m_wait_second;
	int m_port;
	//����״̬ 0 ��ֹ���� 1 ���з���
	int m_svr_status = 0;
	//ip socket map ������
	std::mutex m_lock;
	std::mutex m_svr_status_lock;
	int m_recv_thread_num;
	int m_send_thread_num;
	int m_send_thread_total;
	int m_recv_thread_total;

	std::thread m_control;
	std::vector < std::thread > m_recv_thread;
	std::vector < std::thread > m_send_thread;
	static void _create_svr_thread(void * p)
	{
		Socket_Svr * socket_svr = (Socket_Svr *)p;
		socket_svr->create_svr_thread_run();
	}

	static void recv_thread(void * p)
	{
		Socket_Svr *socket_svr = (Socket_Svr *)p;
		socket_svr->recv_thread_run();
	}

	static void send_thread(void * p)
	{
		Socket_Svr *socket_svr = (Socket_Svr *)p;
		socket_svr->send_thread_run();
	}

#ifdef LINUX_PROJ
	int m_max_count;
	int m_epoll_fd;
	int m_listen;
	struct epoll_event * m_epoll_events;
	MyList <int> m_recv_ready_fd;
	struct sockaddr_in server_addr_in;
	struct sockaddr_in cli_addr;
	unsigned int cli_addr_size = 0;

	void  create_svr_thread_run()
	{

		memset(&server_addr_in, 0x00, sizeof(server_addr_in));
		server_addr_in.sin_family = AF_INET;
		server_addr_in.sin_port = htons(m_port);
		server_addr_in.sin_addr.s_addr = htonl(INADDR_ANY);
		//�����׽��ַ�����
		if ((m_listen = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
		{
			std::cout << "socket error!" << std::endl;
			return;
		}

		//��������
		int reuse = 1;
		setsockopt(m_listen, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

		if (bind(m_listen, (struct sockaddr*)&server_addr_in, sizeof(server_addr_in)) == -1)
		{
			std::cout << "bind error!" << std::endl;
			return;
		}
		if (listen(m_listen, 5) < 0)
		{
			std::cout << "listen error!" << std::endl;
		}

		//ע��epoll
		m_epoll_events = new struct epoll_event[m_max_count];
		if (m_epoll_events == NULL)
		{
			std::cout << "new epoll_event error!" << std::endl;
		}
		m_epoll_fd = epoll_create(m_max_count);
		//�ǼǼ��� listen���
		struct epoll_event ev;
		ev.data.fd = m_listen;
		ev.events = EPOLLIN | EPOLLET;
		epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_listen, &ev);

		//���������̡߳������߳�
		for (int iLoop = 0; iLoop < m_recv_thread_total; iLoop++)
		{
			m_recv_thread.push_back(std::thread(recv_thread, this));
		}
		for (int iLoop = 0; iLoop < m_send_thread_total; iLoop++)
		{
			m_send_thread.push_back(std::thread(send_thread, this));
		}
		//�����߳̽���ѭ��
		int nfds = 0;
		int conn = 0;
		while (m_svr_status == 1)
		{
			//�ȴ�ע����״̬��� 2s���һ�η�������״̬
			if ((nfds = epoll_wait(m_epoll_fd, m_epoll_events, m_max_count, 2000)) <= 0)
			{
				//��ӳ�ʱ���
				continue;
			}
			//���Ѻ� �������о�� 
			for (int iLoop = 0; iLoop < nfds; iLoop++)
			{
				//���Ѿ��Ϊ m_listen �������ڽ�������
				if (m_epoll_events[iLoop].data.fd == m_listen)
				{
					//��������
					if ((conn = accept(m_listen, (struct sockaddr *)&cli_addr, &cli_addr_size)) < 0)
					{
						std::cout << "accept error!" << std::endl;
						continue;
					}

					//��������Ϊ������
					fcntl(conn, F_SETFL, fcntl(conn, F_GETFL, 0) | O_NONBLOCK);
					//�Ǽ�����
					ev.data.fd = conn;
					ev.events = EPOLLIN | EPOLLET;
					epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, conn, &ev);
					//��ȡ�ͻ���IP ��ע��
                    Add_Client ( conn,std::string ( inet_ntoa ( cli_addr.sin_addr ) ) );
					//������Ӿ�����Ϣ
                    m_connect_msg.add_msg ( conn,"1" );
				}
				else if (m_epoll_events[iLoop].events & EPOLLIN)
				{
					//��������������̴߳���
					m_recv_ready_fd.add_msg_with_notify_one(m_epoll_events[iLoop].data.fd);
				}
				/*
				* ���ڲ���Ҫ���պ󴥷�д���� ������ EPOLLOUT״̬
				else if ( events[i].events & EPOLLOUT )
				{

				}
				*/
			}

		} //end while
		close(m_epoll_fd);
		close(m_listen);
        //�ͷ�������Դ
        Free_All_Client();
		delete m_epoll_events;
	}
	void recv_thread_run()
	{
		std::pair < int, int > tmp;
		std::string msg;
		struct epoll_event ev;
		char buf[1024] = { 0 };
		int len = 0;

		while (m_svr_status == 1)
		{
			tmp = m_recv_ready_fd.get_front_with_wait_notify(2);

			if (tmp.first > 0 && tmp.second > 0)
			{
			    //δ�Ǽ� ��Ͽ����� �ȴ�������Ǽ���Ϣ
                if ( !IsSckUsefull( tmp.second ) )
				{
					ev.data.fd = tmp.second;
					ev.events = EPOLLERR;
					std::cout << "unkown connect handle,disconnect " << tmp.second << std::endl;
					shutdown(tmp.second, 0x02);
					epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, tmp.second, &ev);
					continue;
				}
				//ѭ��������Ϣ
				while (true)
				{
					len = 0;
					msg = "";
					memset(buf, 0x00, sizeof(buf));
					if ((len = recv(tmp.second, buf, sizeof(buf), 0)) <= 0)
					{
						//��������
						if (len == 0)
						{
							//���Ͷ����¼� �ڻ�ȡ����������ʱ �黹��Դ
							m_connect_msg.add_msg ( tmp.second,"0" );
                            Free_Client ( tmp.second );
							//ע��epoll����
							ev.data.fd = tmp.second;
							ev.events = EPOLLERR;
							//shutdown(tmp.second,0x02)
							close(tmp.second);
							epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, tmp.second, &ev);
							std::cout << " connect handle disconnect" << std::endl;
							break;
						}
						else if (len == -1 && (errno == EINTR || errno == EWOULDBLOCK || errno == EAGAIN))
						{
							//�޿ɽ�������
							break;
						}
						else
						{
							//δ֪���� �ر�����
							m_connect_msg.add_msg ( tmp.second,"0" );
                            Free_Client ( tmp.second );
							//ע��epoll����
							ev.data.fd = tmp.second;
							ev.events = EPOLLERR;
							std::cout << "recv error errno [" << errno << "]! close connect" << std::endl;
							//�ر�����
							shutdown(tmp.second, 0x02);
							epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, tmp.second, &ev);
							break;
						}
					}
					else if (len == sizeof(buf))
					{
						msg += std::string(buf, sizeof(buf));
						continue;
					}
					else
					{
						msg += std::string(buf, sizeof(buf));
						break;
					}
				}//while ������Ϣ����

				 //���յ���Ϣ ����Ϣ������ ������Ϣ������
				if (msg.length() == 0)
				{
					continue;
				}
				else
				{
					//���ͽ��յ���Ϣʱ��
					m_recv_msg.add_msg(tmp.second,msg);
				}
			}
			else
			{
				//�ȴ�������Ϣ֪ͨ
				//��ʱ ��������״̬ �����ȴ�
				continue;
			}
		}
	}

	void send_thread_run()
	{
		//ѭ���ȴ�����֪ͨ
		struct epoll_event ev;
        std::list < std::pair < TClient_info,std::list <std::string> > > tmp;
		int ret = 0;
		while (m_svr_status == 1)
		{
            tmp = m_send_msg.get_msg (2);
			for ( auto & p : tmp )
			{
                if ( m_send_msg.get_client_ip ( p.first.m_socket ) != p.first.m_ip )
                {
                    continue;
                }
                //����������Ϣ
                for ( auto &msg : p.second )
                {
			        ret = send(p.first.m_socket, msg.c_str(), msg.length(), MSG_NOSIGNAL);
                    if ( ret < 0 )
                    {
                        //��������
                        if ( m_send_msg.get_client_ip ( p.first.m_socket ) != p.first.m_ip )
                        {
                            break;
                        }
                        else
                        {
                            //�׽����쳣
							m_connect_msg.add_msg ( p.first.m_socket ,"0" );
                            Free_Client ( p.first.m_socket );
                            ev.data.fd = p.first.m_socket;
							ev.events = EPOLLERR;
							std::cout << "send error errno [" << errno << "]! close connect" << std::endl;
							//�ر�����
							shutdown(p.first.m_socket, 0x02);
							epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, p.first.m_socket, &ev);
                        }
                    }
                }
            }
        }
    }
#else
	std::map <SOCKET, std::string> m_socket_ip_map;
	SOCKET m_listen;
	HANDLE m_IOCPSocket;
	int m_post_accept_total;
	int m_post_accept_num;
	std::mutex m_post_accept_lock;
	SOCKADDR_IN m_ServiceAddr;
	LPFN_ACCEPTEX m_pFucAcceptEx;
	LPFN_GETACCEPTEXSOCKADDRS m_pFucGetAcceptExSockAddrs;

	bool post_accept()
	{
		m_post_accept_lock.lock();
		//��֤Ͷ�ݸ���Ϊ m_post_accept_total 
		while (m_post_accept_num < m_post_accept_total)
		{
			DWORD dwbytes;
			POPER_DATA p_oper_data;
			//p_oper_data = (POPER_DATA)GlobalAlloc(GPTR, sizeof(TOPER_DATA));
			p_oper_data = new TOPER_DATA;
			memset(&p_oper_data->m_overlapped, 0x00, sizeof(OVERLAPPED));
			p_oper_data->m_databuf.buf = p_oper_data->m_buffer;
			p_oper_data->m_databuf.len = 1024;
			p_oper_data->m_type = ACCEPT_CONN;
			p_oper_data->m_socket = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			if (p_oper_data->m_socket == SOCKET_ERROR)
			{
				std::cout << "acceptex WSASocket Error" << std::endl;
				m_post_accept_lock.unlock();
				return false;
			}
			BOOL rc = m_pFucAcceptEx(m_listen, p_oper_data->m_socket,
				p_oper_data->m_buffer, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
				&dwbytes, &(p_oper_data->m_overlapped));
			m_post_accept_num++;
		}
		m_post_accept_lock.unlock();
		return true;
	}
	int do_accept(TCOM_KEY * pcom_key, TOPER_DATA * poper_data)
	{
		//����һ��acceptͶ��
		//��������		
		SOCKADDR_IN* ClientAddr = NULL;
		SOCKADDR_IN* LocalAddr = NULL;
		int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);
		//�����յ���������Ϣת������
		m_pFucGetAcceptExSockAddrs(poper_data->m_databuf.buf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&LocalAddr, &localLen, (LPSOCKADDR*)&ClientAddr, &remoteLen);
		//Ϊ�½���ͻ��˷��� һ���������ݽṹ ��ɼ���Ͷ��acceptʱ�Ѿ�����
		PCOM_KEY p_cli_com_key = new TCOM_KEY;
		p_cli_com_key->m_socket = poper_data->m_socket;
		p_cli_com_key->m_ip = std::string(inet_ntoa(ClientAddr->sin_addr));

		//���׽��ְ�����ɶ˿�
		if (false == CreateIoCompletionPort((HANDLE)poper_data->m_socket, m_IOCPSocket, (ULONG_PTR)p_cli_com_key, 0))
		{
			//�ر��������
			std::cout << "bind client TO IOCP Error,Need Client Reconnect" << std::endl;
			closesocket(p_cli_com_key->m_socket);
			delete p_cli_com_key;
			delete poper_data;
		}
		else
		{
            Add_Client ( p_cli_com_key->m_socket, p_cli_com_key->m_ip );
			m_connect_msg.add_msg(p_cli_com_key->m_socket,"1");
			post_recv(p_cli_com_key, poper_data);
		}
		//Ͷ����������1 ������Ͷ��	
		m_post_accept_lock.lock();
		m_post_accept_num--;
		m_post_accept_lock.unlock();
		post_accept();
		return true;
	}

	BOOL post_recv(PCOM_KEY  pcom_key, POPER_DATA  poper_data)
	{
		DWORD dwFlags = 0;
		DWORD dwBytes = 0;
		poper_data->m_type = RECV_MSG;
		int iRet = WSARecv(poper_data->m_socket, &(poper_data->m_databuf), 1, &dwBytes, &dwFlags, &(poper_data->m_overlapped), NULL);
		if (iRet == SOCKET_ERROR && WSAGetLastError() == WSA_IO_PENDING)
		{
			return false;
		}
		return true;
	}

	int do_recv(PCOM_KEY  pcom_key, POPER_DATA  poper_data)
	{
		//�鿴�׽����Ƿ�ע��	
		if (!IsSckUsefull(pcom_key->m_socket) )
		{
			//δ֪���� �رպ� 
			std::cout << "Socket Not In Map, Refree Resource" << std::endl;
			do_close(pcom_key, poper_data, 0);
		}
        //������Ϣ
		m_recv_msg.add_msg(pcom_key->m_socket,std::string(poper_data->m_databuf.buf, poper_data->m_overlapped.InternalHigh),500);
		//��ղ�������
		memset(poper_data->m_buffer, 0x00, sizeof(poper_data->m_buffer));
		memset(&(poper_data->m_overlapped), 0x00, sizeof(poper_data->m_overlapped));
		return poper_data->m_overlapped.InternalHigh;
	}

	//inotify_flg 1 ֪ͨ 0 ��֪ͨ
	bool do_close(PCOM_KEY  pcom_key, POPER_DATA  poper_data, int inotify_flg = 1)
	{
		closesocket(pcom_key->m_socket);
        std::string ip =  m_connect_msg.get_client_ip( pcom_key->m_socket );
        TSOCKET sck = pcom_key->m_socket;
		if ( ip  == pcom_key->m_ip )
		{
			//����IO��Դ
			delete pcom_key;
			delete poper_data;
            Free_Client (  pcom_key->m_socket );
		}
        if (inotify_flg == 1)
		{
			m_connect_msg.add_msg(sck,"0");
		}
		//����Ͷ��һ������
		m_post_accept_lock.lock();
		m_post_accept_num--;
		m_post_accept_lock.unlock();
		post_accept();

		return true;
	}

	bool create_svr_thread_run()
	{
		WSADATA wsaData;
		int iRet = 0;
		iRet = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (iRet != NO_ERROR)
		{
			std::cout << "WSAStartup Error" << std::endl;
			return false;
		}
		//������ɶ˿�
		if (NULL == (m_IOCPSocket = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)))
		{
			std::cout << "create IOCP Error" << std::endl;
			return false;
		}
		//��������������
		m_listen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (m_listen == INVALID_SOCKET)
		{
			std::cout << "WSASocket Error" << std::endl;
			return false;
		}
		m_ServiceAddr.sin_family = AF_INET;
		m_ServiceAddr.sin_addr.S_un.S_addr = INADDR_ANY;
		m_ServiceAddr.sin_port = htons((u_short)m_port);

		//��m_listen������ɶ˿�
		PCOM_KEY pListen_com_key = new TCOM_KEY;
		if (NULL == CreateIoCompletionPort((HANDLE)m_listen, m_IOCPSocket, (DWORD)pListen_com_key, 0))
		{
			std::cout << "bind m_listen TO IOCP Error" << std::endl;
			return false;
		}
		//bind
		if (SOCKET_ERROR == bind(m_listen, (struct sockaddr *)&m_ServiceAddr, sizeof(m_ServiceAddr)))
		{
			std::cout << "bind Error" << std::endl;
			return false;
		}
		//����
		if (SOCKET_ERROR == listen(m_listen, SOMAXCONN))
		{
			std::cout << "listen Error" << std::endl;
			return false;
		}
		// ʹ��AcceptEx��������Ϊ���������WinSock2�淶֮���΢�������ṩ����չ����
		// ������Ҫ�����ȡһ�º�����ָ�룬
		// ��ȡAcceptEx����ָ��
		// AcceptEx �� GetAcceptExSockaddrs ��GUID�����ڵ�������ָ��

		GUID GuidAcceptEx = WSAID_ACCEPTEX;
		GUID GuidGetAcceptExSockAddrs = WSAID_GETACCEPTEXSOCKADDRS;

		DWORD dwBytes = 0;
		if (SOCKET_ERROR == WSAIoctl(
			m_listen,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidAcceptEx,
			sizeof(GuidAcceptEx),
			&m_pFucAcceptEx,
			sizeof(m_pFucAcceptEx),
			&dwBytes,
			NULL,
			NULL))
		{
			std::cout << "Get AcceptEx Function Pointer Error" << std::endl;
			return false;
		}

		if (SOCKET_ERROR == WSAIoctl(
			m_listen,
			SIO_GET_EXTENSION_FUNCTION_POINTER,
			&GuidGetAcceptExSockAddrs,
			sizeof(GuidGetAcceptExSockAddrs),
			&m_pFucGetAcceptExSockAddrs,
			sizeof(m_pFucGetAcceptExSockAddrs),
			&dwBytes,
			NULL,
			NULL))
		{
			std::cout << "Get GetAcceptExSockAddrs Function Pointer Error" << std::endl;
			return false;
		}
		//���������߳� ѡ���ⲿ���õķ�ʽ ��ѡ���ں��� * 2 ����������
		for (int iLoop = 0; iLoop < m_recv_thread_total; iLoop++)
		{
			m_recv_thread.push_back(std::thread(recv_thread, this));
		}
		for (int iLoop = 0; iLoop < m_send_thread_total; iLoop++)
		{
			m_send_thread.push_back(std::thread(send_thread, this));
		}
		// ΪAcceptEx ׼��������Ȼ��Ͷ��AcceptEx I/O����
		post_accept();
		return true;
	}
	void recv_thread_run()
	{
		PCOM_KEY pcom_key = NULL;
		OVERLAPPED *poverlapped = NULL;
		POPER_DATA poper_data = NULL;
		DWORD dwIOSize = -1;
		BOOL res = false;
		while (m_svr_status == 1)
		{
			res = false;
			res = GetQueuedCompletionStatus(m_IOCPSocket, &dwIOSize, (PULONG_PTR)&pcom_key, &poverlapped,1000);
			if (poverlapped != NULL)
			{
				poper_data = CONTAINING_RECORD(poverlapped, TOPER_DATA, m_overlapped);
			}
			if (!res)
			{
				if (WAIT_TIMEOUT == WSAGetLastError())
				{
					continue;
				}

				if (poper_data == NULL)
				{
					std::cout << "GetQueuedCompletionStatus Error!" << std::endl;
				}
				else
				{
					//���յ��쳣����������ͻ���崻� δʹ�� closescoket��
					if (poper_data->m_type == RECV_MSG)
					{
						//���յ��쳣�Ľ������� �ͷ��׽�����Դ ������Ͷ������
						do_close(pcom_key, poper_data, 1);
						std::cout << "Recv Disconnect From Client! Release Client Resource" << std::endl;
					}
					else if (poper_data->m_type == ACCEPT_CONN)
					{
						//���յ��쳣�Ľ������� �ͷ��׽�����Դ ������Ͷ������
						closesocket(poper_data->m_socket);
						std::cout << "Error "<<poper_data->m_socket << std::endl;
						delete poper_data;
						//����Ͷ��һ������
						m_post_accept_lock.lock();
						m_post_accept_num--;
						m_post_accept_lock.unlock();
						post_accept();
						std::cout << "Accept Error! Release Client Resourse" << std::endl;
					}
					else
					{
						if (WAIT_TIMEOUT == WSAGetLastError())
						{
							continue;
						}
						//���յ��쳣�Ľ������� �ͷ��׽�����Դ ������Ͷ������
						std::cout << "GetQueuedCompletionStatus Error! Release Client Resourse" << std::endl;
					}
				}
			}
			else
			{
				//IOCP�������յ�����
				if (dwIOSize != 0)
				{
					//���յ����� ���� ���ա����������������������ݣ�
					if (poper_data->m_type == RECV_MSG)
					{
						do_recv(pcom_key, poper_data);
						post_recv(pcom_key, poper_data);
					}
					else if (poper_data->m_type == ACCEPT_CONN)
					{
						do_accept(pcom_key, poper_data);
						std::cout << "Recv connet" << std::endl;
					}
					else
					{
						std::cout << "TYPE Error!" << std::endl;
					}
				}
				else
				{
					//�鿴��ɼ��Ƿ�Ϊ��
					if (pcom_key == NULL)
					{
						//δ֪��� �����ʲôʱ����ɼ��ǿյ�
						std::cout << "pcom_key is NULL" << std::endl;
					}
					else if (poper_data != NULL)
					{
						//Ϊ�������� ����Ϊ���ա�����ʱΪ�����������󣻲���Ϊ����ʱ Ϊ��������û�и�����Ϣ
						if (poper_data->m_type == RECV_MSG || poper_data->m_type == SEND_MSG)
						{
							//�������
							do_close(pcom_key, poper_data, 1);
							std::cout << "client disconneted!" << std::endl;
						}
						else if (poper_data->m_type == ACCEPT_CONN)
						{
							do_accept(pcom_key, poper_data);
						}
					}
					else
					{
						//δ֪����	��ɼ�ΪNULL ������ ��NULL
						std::cout << "complate_key and operater_date both NULL" << std::endl;
					}
				}
			}
		}
	}

	void send_thread_run()
	{
		std::list < std::pair < TClient_info, std::list <std::string> > > tmp;
		int ret = 0;
		while (m_svr_status == 1)
		{
            tmp = m_send_msg.get_msg (2);
			for ( auto & p : tmp )
			{
                if ( m_send_msg.get_client_ip ( p.first.m_socket ) != p.first.m_ip )
                {
                    continue;
                }
                //����������Ϣ
                for ( auto &msg : p.second )
                {
			        ret = send(p.first.m_socket, msg.c_str(), msg.length(), 0);
                    if ( ret < 0 )
                    {
                        //��������
                        if ( m_send_msg.get_client_ip ( p.first.m_socket ) != p.first.m_ip )
                        {
                            break;
                        }
                        else
                        {
                            //�׽����쳣
							m_connect_msg.add_msg ( p.first.m_socket,"0" );
                            Free_Client ( p.first.m_socket );
							std::cout << "send error errno [" << errno << "]! close connect" << std::endl;
							//�ر�����
							shutdown(p.first.m_socket, 0x02);
                        }
                    }
                }
            }
        }

	}

#endif
public:
#ifdef LINUX_PROJ
	Socket_Svr(int mac_count = 2048, int recv_thread_num = 4, int send_thread_num = 2)
	{
		m_svr_status = 0;
		m_listen = -1;
		m_max_count = mac_count;
		m_recv_thread_total = recv_thread_num;
		m_send_thread_total = send_thread_num;
		m_epoll_events = NULL;
		m_wait_second = 2;
	}

	~Socket_Svr()
	{
		if (m_listen > 0)
		{
			close(m_listen);
		}
		if (m_epoll_fd > 0)
		{
			close(m_epoll_fd);
		}
	}
#else
	Socket_Svr(int post_accept_total = 50, int recv_thread_total = 4, int send_thread_total = 2)
	{
		m_post_accept_total = post_accept_total;
		m_svr_status = 0;
		m_listen = 0;
		m_recv_thread_total = recv_thread_total;
		m_send_thread_total = send_thread_total;
		m_wait_second = 2;
	}
	~Socket_Svr()
	{
	}
#endif
	//�������� ������socket�����߳�
	bool create_svr_thread(int port)
	{
		m_svr_status_lock.lock();
		if (m_svr_status == 0)
		{
			m_svr_status = 1;
			m_port = port;
#ifdef LINUX_PROJ
			m_control = std::thread(_create_svr_thread, this);
#else
			if (!create_svr_thread_run())
			{
				m_svr_status_lock.unlock();
				return false;
			}
#endif
			m_svr_status_lock.unlock();
			return true;
		}
		else
		{
			m_svr_status_lock.unlock();
			std::cout << "Svr is running!" << std::endl;
			return false;
		}
	}

	//�Ͽ����� ��ֹͣ����������
	bool stop_svr_thread()
	{
		m_svr_status_lock.lock();
		if (m_svr_status == 0)
		{
			m_svr_status_lock.unlock();
			return true;
		}
		m_svr_status = 0;
		m_lock.unlock();
#ifdef LINUX_PROJ
		m_control.join();
#endif
		for (auto &p : m_recv_thread)
		{
			p.join();
		}
		m_recv_thread.clear();

		for (auto &p : m_send_thread)
		{
			p.join();
		}
		m_send_thread.clear();
		m_svr_status_lock.unlock();
		return true;
	}

	//��յ�ǰ����
	bool restart_svr_thread()
	{
		return true;
	}

	std::list < std::pair < TClient_info,std::list <std::string> > > recv_msg(int timeout = -1)
	{
        std::list < std::pair < TClient_info,std::list <std::string> > > tmp;
		while (true)
		{
			tmp = m_recv_msg.get_msg(timeout);
		    return tmp;
		}
	}

	bool send_msg ( TSOCKET conn, std::string msg)
	{
		m_send_msg.add_msg(conn,msg);
		return true;
	}

    std::list < std::pair < TClient_info,std::list <std::string> > > recv_connect_msg (int timeout = -1)
	{
        std::list < std::pair < TClient_info,std::list <std::string> > > tmp;
		while (true)
		{
			tmp = m_connect_msg.get_msg(timeout);
		    return tmp;
		}
	}
};
