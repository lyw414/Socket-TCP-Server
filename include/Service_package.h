#pragma once
#include <mutex>
#include <list>
#include <iostream>
#include "ArrayList.h"
typedef struct client_info
{
#ifdef LINUX_PROJ
    typedef int TSOCKECT;
#else
    typedef SOCKET TSOCKECT;
#endif
    TSOCKECT m_socket;
    std::string m_ip;
}TClient_info, *PClient_info;

//数据包
//汇聚数据 配合队列使用 减少队列的操作
//增加提升流量的方式的压榨IOCP/EPOLL效率 
//减少资源操作次数 降低CPU使用率

class Service_package
{
private:
#ifdef LINUX_PROJ
    typedef int TSOCKET;
#else
    typedef SOCKET TSOCKET;
#endif
    // 1 按最大长度缓存 2 按最大条数缓存 0 不缓存
    int m_mode;
    std::string m_msg;
    size_t m_max_num;
    size_t m_max_len;
    //记录每则消息的长度
	std::list < size_t > m_list;
	//ArrayList < size_t > m_list = ArrayList < size_t >(4096);
    std::mutex m_lock;
    bool m_in_list;
public:
    std::string m_ip;
    TSOCKET m_socket;

    void set_info ( TSOCKET sck, std::string  ip )
    {
        m_lock.lock();
        m_socket = sck;
        m_ip = ip;
        m_lock.unlock();
    }
    Service_package ( int mode, int max_size )
    {
        m_in_list = false;
        m_socket = -1;
        m_ip = "";
        m_mode = mode;
        if ( m_mode == 0 )
        {
            m_max_len = 0;
            m_max_num = 0;
        } 
        else if ( m_mode == 1 )
        {

            m_max_len = max_size;
            m_max_num = 0;
        }
        else if ( m_mode == 2 )
        {

            m_max_len = 0;
            m_max_num = max_size;
        }
        else
        {
            m_mode = 1;
            m_max_len = 10240;
            m_max_num = 0;
        }
    }
    bool reset (int mode, int max_size,TSOCKET sck,std::string ip)
    {
        m_lock.lock();
        m_mode = mode;
        if ( m_mode == 0 )
        {
            m_max_len = 0;
            m_max_num = 0;
        } 
        else if ( m_mode == 1 )
        {

            m_max_len = max_size;
            m_max_num = 0;
        }
        else if ( m_mode == 2 )
        {

            m_max_len = 0;
            m_max_num = max_size;
        }
        else
        {
            m_mode = 1;
            m_max_len = 10240;
            m_max_num = 0;
        }
        m_in_list = false;
        m_socket = sck;
        m_ip = ip;
        m_msg = "";
        m_list.clear();
        m_lock.unlock();
        return true;
    }
    
	bool isUsed()
	{
		bool tmp;
		m_lock.lock();
		tmp = m_in_list;
		m_lock.unlock();
		return tmp;
	}
	void setUsed()
	{
		m_lock.lock();
		m_in_list = true;
		m_lock.unlock();
	}

	void setUnused()
	{
		m_lock.lock();
		m_in_list = false;
		m_lock.unlock();
	}

    int add_msg ( std::string msg, size_t cache = 0)
    {
		m_lock.lock();
        if ( m_mode == 1 )
        {
            if ( m_msg.length() > m_max_len )
            {
				m_lock.unlock();
                return -2;
            }
        }
        else
        {
            if ( m_mode == 2 )
            {
                if ( m_list.size() > m_max_num )
                {
					m_lock.unlock();
                    return -2;
                }
            }

        }
        m_msg += msg; 
        m_list.push_back(msg.length());
		m_lock.unlock();
		if (m_list.size() < cache)
		{
			return 0;
		}
		return 1;
    }

    std::pair < TClient_info,std::list <std::string> >  getMessage ( )
    {
        std::pair < TClient_info,std::list <std::string > > tmp;
		m_lock.lock();
        tmp.first.m_socket = m_socket;
        tmp.first.m_ip = m_ip;
        size_t index_begin = 0;
			
        for ( auto & p : m_list ) 
        {
            if ( index_begin + p <= m_msg.length() )
            {
                tmp.second.push_back(m_msg.substr(index_begin,p));
                index_begin += p;
            }
            else
            {
                std::cout << "Error Msg" << std::endl;
                break;
            }
        }
        m_list.clear();
        m_msg = "";
		m_lock.unlock();
        return tmp;
    }
};
typedef Service_package * PService_package;
