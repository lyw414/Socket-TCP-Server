#pragma once
#pragma once
#include <thread>
#include <mutex>
#include "MyList.h"
#include <map>
#include <iostream>
#include <vector>
#include <string.h>

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
//完成键
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

typedef struct Svr_res
{
#ifdef LINUX_PROJ
	int m_socket;
#else
	SOCKET m_socket;
#endif
	std::string m_ip;
	std::string m_msg;
	int m_errcode;
	//1 链正常 0 断链 -1 socket异常主动断链
	int m_status;
};

/*
* @brief   tcp多连接原型 使用三条队列 数据接收队列、数据发送队列、客户端状态队列
*          分别维护接收数据、发送数据、客户端建链断链，对外暴露数据读写、状态获取接口
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
    Client_interpreter m_connet_msg;

    void Free_Client ( TSOCKET sck )
    {
        m_connet_msg.delete_client(sck); 
        m_recv_msg.delete_client(sck); 
        m_send_msg.delete_client(sck); 
    }

    bool IsSckUsefull ( TSOCKET sck )
    {
        //以m_connect_msg 队列的资源为准
        return m_connet_msg.IsSCKUsefull(sck); 
    }
    void Add_Client ( TSOCKET sck,std::string ip )
    {
        m_connet_msg.add_client(sck,ip); 
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
	//服务状态 0 终止服务 1 运行服务
	int m_svr_status = 0;
	//ip socket map 互斥量
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
		//设置套接字非阻塞
		if ((m_listen = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0)) < 0)
		{
			std::cout << "socket error!" << std::endl;
			return;
		}

		//设置重用
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

		//注册epoll
		m_epoll_events = new struct epoll_event[m_max_count];
		if (m_epoll_events == NULL)
		{
			std::cout << "new epoll_event error!" << std::endl;
		}
		m_epoll_fd = epoll_create(m_max_count);
		//登记监听 listen句柄
		struct epoll_event ev;
		ev.data.fd = m_listen;
		ev.events = EPOLLIN | EPOLLET;
		epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, m_listen, &ev);

		//创建接收线程、发送线程
		for (int iLoop = 0; iLoop < m_recv_thread_total; iLoop++)
		{
			m_recv_thread.push_back(std::thread(recv_thread, this));
		}
		for (int iLoop = 0; iLoop < m_send_thread_total; iLoop++)
		{
			m_send_thread.push_back(std::thread(send_thread, this));
		}
		//控制线程进入循环
		int nfds = 0;
		int conn = 0;
		while (m_svr_status == 1)
		{
			//等待注册句柄状态变更 2s检查一次服务运行状态
			if ((nfds = epoll_wait(m_epoll_fd, m_epoll_events, m_max_count, 2000)) <= 0)
			{
				//添加超时检测
				continue;
			}
			//唤醒后 遍历就行句柄 
			for (int iLoop = 0; iLoop < nfds; iLoop++)
			{
				//唤醒句柄为 m_listen 表明存在建链请求
				if (m_epoll_events[iLoop].data.fd == m_listen)
				{
					//接收连接
					if ((conn = accept(m_listen, (struct sockaddr *)&cli_addr, &cli_addr_size)) < 0)
					{
						std::cout << "accept error!" << std::endl;
						continue;
					}
					//设置连接为非阻塞
					fcntl(conn, F_SETFL, fcntl(conn, F_GETFL, 0) | O_NONBLOCK);
					//登记连接
					ev.data.fd = conn;
					ev.events = EPOLLIN | EPOLLET;
					epoll_ctl(m_epoll_fd, EPOLL_CTL_ADD, conn, &ev);
					//获取客户端IP 并注册
                    Add_Client ( conn,std::string ( inet_ntoa ( cli_addr.sin_addr ) ) );
					//添加连接就绪消息
                    m_connect_msg.add_msg ( conn,"1" );
				}
				else if (m_epoll_events[iLoop].events & EPOLLIN)
				{
					//将句柄丢给工作线程处理
					m_recv_ready_fd.add_msg_with_notify_one(m_epoll_events[iLoop].data.fd);
				}
				/*
				* 由于不需要接收后触发写操作 不监听 EPOLLOUT状态
				else if ( events[i].events & EPOLLOUT )
				{

				}
				*/
			}

		} //end while
		close(m_epoll_fd);
		close(m_listen);
        //释放连接资源
        Free_All_Client()
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
			if (tmp.first == 0 && tmp.second > 0)
			{
			    //未登记 则断开连接 等待建链后登记信息
                if ( !IsSckUsefull( tmp.second ) )
				{
					ev.data.fd = tmp.second;
					ev.events = EPOLLERR;
					std::cout << "unkown connect handle,disconnect " << tmp.second << std::endl;
					shutdown(tmp.second, 0x02);
					epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, tmp.second, &ev);
					continue;
				}
				//循环接收消息
				while (true)
				{
					len = 0;
					msg = "";
					memset(buf, 0x00, sizeof(buf));
					if ((len = recv(tmp.second, buf, sizeof(buf), 0)) <= 0)
					{
						//断链请求
						if (len == 0)
						{
							//发送断链事件 在获取到断链请求时 归还资源
							m_connect_msg.add_msg ( tmp.second,"0" );
                            Free_Client ( tmp.second );
							//注销epoll监听
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
							//无可接收数据
							break;
						}
						else
						{
							//未知错误 关闭连接
							m_connect_msg.add_msg ( tmp.second,"0" );
                            Free_Client ( tmp.second );
							//注销epoll监听
							ev.data.fd = tmp.second;
							ev.events = EPOLLERR;
							std::cout << "recv error errno [" << errno << "]! close connect" << std::endl;
							//关闭连接
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
				}//while 接收消息结束

				 //接收到消息 将消息放置于 接收消息队列中
				if (msg.length() == 0)
				{
					continue;
				}
				else
				{
					//发送接收到消息时间
					m_recv_msg.add_msg(tmp.second,msg);
				}
			}
			else
			{
				//等待接收消息通知
				//超时 检查服务器状态 继续等待
				continue;
			}
		}
	}

	void send_thread_run()
	{
		//循环等待发送通知
		struct epoll_event ev;
        std::list < std::pair < TClient_info,std::list <std::string> > > tmp:
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
                //逐条发送消息
                for ( auto &msg : p.second )
                {
			        ret = send(p.first.m_socket, msg.c_str(), msg.length(), MSG_NOSIGNAL);
                    if ( ret < 0 )
                    {
                        //主动断链
                        if ( m_send_msg.get_client_ip ( p.first.m_socket ) != p.first.m_ip )
                        {
                            break;
                        }
                        else
                        {
                            //套接字异常
							m_connect_msg.add_msg ( tmp.second,"0" );
                            Free_Client ( p.first.m_socket );
                            ev.data.fd = p.first.m_socket;
							ev.events = EPOLLERR;
							std::cout << "send error errno [" << errno << "]! close connect" << std::endl;
							//关闭连接
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
		//保证投递个数为 m_post_accept_total 
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
		//消耗一个accept投递
		//接收连接		
		SOCKADDR_IN* ClientAddr = NULL;
		SOCKADDR_IN* LocalAddr = NULL;
		int remoteLen = sizeof(SOCKADDR_IN), localLen = sizeof(SOCKADDR_IN);
		//将接收到的连接信息转换出来
		m_pFucGetAcceptExSockAddrs(poper_data->m_databuf.buf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, (LPSOCKADDR*)&LocalAddr, &localLen, (LPSOCKADDR*)&ClientAddr, &remoteLen);
		//为新接入客户端分配 一个操作数据结构 完成键在投递accept时已经分配
		PCOM_KEY p_cli_com_key = new TCOM_KEY;
		p_cli_com_key->m_socket = poper_data->m_socket;
		p_cli_com_key->m_ip = std::string(inet_ntoa(ClientAddr->sin_addr));

		//将套接字绑定至完成端口
		if (false == CreateIoCompletionPort((HANDLE)poper_data->m_socket, m_IOCPSocket, (ULONG_PTR)p_cli_com_key, 0))
		{
			//关闭这个连接
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
		//投递数量减少1 并重新投递	
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
		//查看套接字是否注册	
		if (!IsInIPMAP(pcom_key->m_socket, pcom_key->m_ip))
		{
			//未知连接 关闭后 
			std::cout << "Socket Not In Map, Refree Resource" << std::endl;
			do_close(pcom_key, poper_data, 0);
		}
        //发送消息
		m_recv_msg.add_msg(pcom_key->m_socket,std::string(poper_data->m_databuf.buf, poper_data->m_overlapped.InternalHigh));
		//清空操作数据
		memset(poper_data->m_buffer, 0x00, sizeof(poper_data->m_buffer));
		memset(&(poper_data->m_overlapped), 0x00, sizeof(poper_data->m_overlapped));
		return poper_data->m_overlapped.InternalHigh;
	}

	//inotify_flg 1 通知 0 不通知
	bool do_close(PCOM_KEY  pcom_key, POPER_DATA  poper_data, int inotify_flg = 1)
	{
		Svr_res tmp;
		
		closesocket(pcom_key->m_socket);
        std::string ip =  m_connect_msg.get_client_ip( pcom_key->m_socket );
        TSOCKET sck = pcom_key->m_socket;
		if ( ip  == pcom_key->m_ip )
		{
			//回收IO资源
			delete pcom_key;
			delete poper_data;
            Free_Client (  pcom_key->m_socket );
		}
        if (inotify_flg == 1)
		{
			m_connect_msg.add(sck,"0");
		}

		//重新投递一个连接
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
		//创建完成端口
		if (NULL == (m_IOCPSocket = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0)))
		{
			std::cout << "create IOCP Error" << std::endl;
			return false;
		}
		//创建服务器监听
		m_listen = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (m_listen == INVALID_SOCKET)
		{
			std::cout << "WSASocket Error" << std::endl;
			return false;
		}
		m_ServiceAddr.sin_family = AF_INET;
		m_ServiceAddr.sin_addr.S_un.S_addr = INADDR_ANY;
		m_ServiceAddr.sin_port = htons((u_short)m_port);

		//将m_listen绑定至完成端口
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
		//监听
		if (SOCKET_ERROR == listen(m_listen, SOMAXCONN))
		{
			std::cout << "listen Error" << std::endl;
			return false;
		}
		// 使用AcceptEx函数，因为这个是属于WinSock2规范之外的微软另外提供的扩展函数
		// 所以需要额外获取一下函数的指针，
		// 获取AcceptEx函数指针
		// AcceptEx 和 GetAcceptExSockaddrs 的GUID，用于导出函数指针

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
		//启动工作线程 选择外部配置的方式 不选用内核数 * 2 的最优配置
		for (int iLoop = 0; iLoop < m_recv_thread_total; iLoop++)
		{
			m_recv_thread.push_back(std::thread(recv_thread, this));
		}
		for (int iLoop = 0; iLoop < m_send_thread_total; iLoop++)
		{
			m_send_thread.push_back(std::thread(send_thread, this));
		}
		// 为AcceptEx 准备参数，然后投递AcceptEx I/O请求
		post_accept();
	}
	void recv_thread_run()
	{
		Svr_res tmp;
		PCOM_KEY pcom_key = NULL;
		OVERLAPPED *poverlapped = NULL;
		POPER_DATA poper_data = NULL;
		DWORD dwIOSize = -1;
		BOOL res = false;
		while (m_svr_status == 1)
		{
			res = GetQueuedCompletionStatus(m_IOCPSocket, &dwIOSize, (PULONG_PTR)&pcom_key, &poverlapped, -1);
			if (poverlapped != NULL)
			{
				poper_data = CONTAINING_RECORD(poverlapped, TOPER_DATA, m_overlapped);
			}
			if (!res)
			{
				if (poper_data == NULL)
				{
					if (WAIT_TIMEOUT == WSAGetLastError())
					{
						continue;
					}
					else
					{
						std::cout << "GetQueuedCompletionStatus Error!" << std::endl;
					}
				}
				else
				{
					//接收到异常断链请求（如客户端宕机 未使用 closescoket）
					if (poper_data->m_type == RECV_MSG)
					{
						//接收到异常的建链请求 释放套接字资源 并重新投递请求
						do_close(pcom_key, poper_data, 1);
						std::cout << "Recv Disconnect From Client! Release Client Resource" << std::endl;
					}
					else if (poper_data->m_type == ACCEPT_CONN)
					{
						//接收到异常的建链请求 释放套接字资源 并重新投递请求
						closesocket(poper_data->m_socket);
						delete poper_data;
						//重新投递一个连接
						m_post_accept_lock.lock();
						m_post_accept_num--;
						m_post_accept_lock.unlock();
						post_accept();
						std::cout << "Accept Error! Release Client Resourse" << std::endl;
					}
					else
					{
						std::cout << "GetQueuedCompletionStatus Error! Release Client Resourse" << std::endl;
					}
				}
			}
			else
			{
				//IOCP正常接收到数据
				if (dwIOSize != 0)
				{
					//接收到请求 包括 接收、建链（建链附带接收数据）
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
					//查看完成键是否为空
					if (pcom_key == NULL)
					{
						//未知情况 不清楚什么时候完成键是空的
						std::cout << "pcom_key is NULL" << std::endl;
					}
					else if (poper_data != NULL)
					{
						//为管理请求 操作为接收、发送时为正常断链请求；操作为建链时 为建链请求没有附件消息
						if (poper_data->m_type == RECV_MSG || poper_data->m_type == SEND_MSG)
						{
							//常规断链
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
						//未知错误	完成键为NULL 操作数 非NULL
						std::cout << "complate_key and operater_date both NULL" << std::endl;
					}
				}
			}
		}
	}

	void send_thread_run()
	{
        struct epoll_event ev;
        std::list < std::pair < TClient_info,std::list <std::string> > > tmp:
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
                //逐条发送消息
                for ( auto &msg : p.second )
                {
			        ret = send(p.first.m_socket, msg.c_str(), msg.length(), MSG_NOSIGNAL);
                    if ( ret < 0 )
                    {
                        //主动断链
                        if ( m_send_msg.get_client_ip ( p.first.m_socket ) != p.first.m_ip )
                        {
                            break;
                        }
                        else
                        {
                            //套接字异常
							m_connect_msg.add_msg ( tmp.second,"0" );
                            Free_Client ( p.first.m_socket );
                            ev.data.fd = p.first.m_socket;
							ev.events = EPOLLERR;
							std::cout << "send error errno [" << errno << "]! close connect" << std::endl;
							//关闭连接
							shutdown(p.first.m_socket, 0x02);
							epoll_ctl(m_epoll_fd, EPOLL_CTL_DEL, p.first.m_socket, &ev);
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
	//启动服务 并创建socket服务线程
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

	//断开连接 并停止服务器监听
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

	//清空当前连接
	bool restart_svr_thread()
	{
		return true;
	}

	std::list < std::pair < TClient_info,std::list <std::string> > > recv_msg(int timeout = -1)
	{
        std::list < std::pair < TClient_info,std::list <std::string> > > tmp;
		std::pair <int, std::list<Svr_res>> tmp;
		while (true)
		{
			tmp = m_recv_msg.get_msg(timeout);
		    return tmp.second;
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
		std::pair <int, std::list<Svr_res>> tmp;
		while (true)
		{
			tmp = m_conn_msg.get_msg(timeout);
		    return tmp.second;
		}
	}
};
