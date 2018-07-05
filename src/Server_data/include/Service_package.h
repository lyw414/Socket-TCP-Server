#pragma once
#include <mutex>
#include <list>
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
    typedef int TSOCKECT;
#else
    typedef SOCKET TSOCKECT;
#endif
    std::string m_ip;
    TSOCKECT m_socket;
    // 1 按最大长度缓存 2 按最大条数缓存 0 不缓存
    int m_mode;
    std::string m_msg;
    std::list < std::string> m_list;
    int m_max_len;
    int m_max_num;
    std::mutex m_lock;

public:
    void set_info ( TSOCKET sck, std::string  ip )
    {
        m_lock.lock();
        m_socket = sck;
        m_ip = ip;
        m_lock.unlock();
    }

    BOOL m_in_list;
    Service_package ( int m_mode, int max_size )
    {
        m_in_list = false;
        sck = -1;
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
            m_mode == 1;
            m_max_len = 10240;
            m_max_num = 0;
        }
    }
    BOOL reset (int mode, int max_size,TSOCKET sck,std::string ip)
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
            m_mode == 1;
            m_max_len = 10240;
            m_max_num = 0;
        }
        m_in_list = false;
        m_socket = sck;
        m_ip = ip;
        m_msg = "";
        m_list.clear();
        m_lock.lock();
        return true;
    }
    
    void lock()
    {
        m_lock.lock();
    }

    void unlock()
    {
        m_lock.unlock();
    }
    int add_msg ( std::string msg )
    {
        if ( m_mode == 1 )
        {
            if ( m_msg.length() < m_max_len )
            {
                m_msg += msg; 
            }
            else
            {
                return -2;
            }
        }
        else
        {
            if ( m_mode == 2 )
            {
                if ( m_list.size() > m_max_num )
                {
                    return -2;
                }
            }
            m_list.push_back(msg);
        }
        return 0;
    }

    std::pair < TClient_info,std::list <std::string> >  getMessage ( )
    {
        std::pair < TClient_info,std::list <std::string > > tmp;
        tmp.first.m_socket = m_socket;
        tmp.first.m_ip = m_ip;
        m_in_list = false;
        if ( m_mode == 1 )
        {
            //消息存在于字符串
            tmp.second.push_back(m_msg);
            m_msg = "";
        }
        else
        {
            //消息存在于列表
            tmp.second = m_list;
            m_list.clear();
        }
        return tmp;
    }
};
typedef Service_package * PService_package;

