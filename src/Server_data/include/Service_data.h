#pragma once
#include <map>
#include <vector>
#include "Service_package.h"
#include "read_write_lock.h"

//固定长度的客户单连接资源
//提供资源分配功能 可以一定程度减少 new delete
//使用 取出 与 放回的机制 增加资源的复用度
class Service_data
{
private:
    int m_fixed_len;
    int index;
    std::mutex m_lock;
    PService_package  m_data[];
public:
    PService_data( int fixed_len )
    {
        if ( fixed_len > 0 )
        {
            m_fixed_len = fixed_len;
        }
        else
        {
            m_fixed_len = 2048; 
        }
        m_data = new PService_package[m_fixed_len];
        index = 0;
        //分配出资源
        for ( int iLoop = 0 ; iLoop < m_fixed_len; iLoop ++ )
        {
            m_data[iLoop] = new Service_package;
        }
    } 

    PService_data()
    {
        m_fixed_len = 2048; 
        m_data = new PService_package[m_fixed_len];
        index = 0;
        //分配出资源
        for ( int iLoop = 0 ; iLoop < m_fixed_len; iLoop ++ )
        {
            m_data[iLoop] = new Service_package;
        }
    }

    ~PService_data()
    {
        for ( auto &p : m_data )
        {
            delete p;
        }
        delete [] m_data;
    }

    PService_package get_one() 
    {
        PService_package tmp;
        m_lock.lock();
        if ( index + 1 < m_fixed_len )
        {
            tmp = m_data [ index ];
            index++;
            m_lock.unlock();
        }
        else
        {
            m_lock.unlock();
            tmp = new Service_package;
        }
        return tmp;
    }

    void put_back( PService_package client )
    {
        m_lock.lock();
        if ( client == NULL )
        {
            m_lock.unlock();
            return;
        }
        if ( index > 0 )
        {
           index--; 
           m_data [ index ] = client;
            m_lock.unlock();
        }
        else
        {
            m_lock.unlock();
            delete client;
        }
    }
};

class Client_interpreter
{
private:
    MyList <PService_package> m_data;
    Service_data m_client_rc;
    read_write_lock  m_map_lock;
    int m_mode;
    int m_max_size;
#ifdef LINUX_PROJ
    typedef int TSOCKECT;
#else
    typedef SOCKET TSOCKECT;
#endif
    std::map < TSOCKET, PService_package > m_map;
    BOOL IsInMap ( TSOCKET sck )
    {
        if ( m_map.find(sck) != m_client.end() ) 
        {
            return true;
        }
        return false;
    }

    PService_package Get_Client ( TSOCKET sck )
    {
        std::map < TSOCKET, PService_package > :: iterator it;
        it = m_map.find(sck);
        if ( it != m_map.end() )
        {
            return it->second;
        }
        else
        {
            return NULL;
        }
    }

public:
    //max_list_size               消息队列初始化长度
    //max_client_size             客户端资源池固定保留资源个数
    //mode                        数据包缓存模式（0 无缓存限制 1 按大小缓存 2 按记录条数缓存）
    //max_size                    数据包缓存最大数量（超过该值 则丢弃数据包）
    Client_interpreter(int max_list_size, int max_client_size,int mode, int max_size) : m_data(max_list_size),m_client(max_client_size)
    {
        m_mode = mode;
        m_max_size = max_size;
    }

    BOOL add_client ( TSOCKET sck , std::string ip)
    {
        PService_package tmp;
        m_map_lock.write_lock();
        if ( ( tmp = Get_Client ( sck ) ) != NULL )
        {
            tmp->reset ( m_mode,max_size,sck,ip );
        }
        else
        {
            if( ( tmp = m_client.get_one() ) == NULL )
            {
                std::cout << "Get Client Source Error!" std::endl;
                m_map_lock.write_unlock();
                return false;
            }
            else
            {
                tmp->reset ( m_mode,max_size,sck,ip );
                m_map[sck] = tmp;
            }
        }
        m_map_lock.write_unlock();
        return true;
    }

    BOOL delete_client(TSOCKET sck)
    {

        PService_package tmp;
        m_map_lock.write_lock();
        if ( ( tmp = Get_Client ( sck ) ) != NULL )
        {
            m_client.put_back(tmp);
        }
        m_map_lock.write_unlock();
        return true;
    }

    // -2 包缓存达到上限 -1 队列满
    int add_msg ( int sck, std::string msg )
    {
        int ret =  0;
        PService_package tmp;
        m_map_lock.read_lock();
        if ( ( tmp = Get_Client ( sck ) ) != NULL )
        {
            tmp->lock();
            if ( ( ret = tmp->add_msg(msg) ) < 0 )
            {
                tmp->unlock();
                m_map_lock.read_unlock();
                return ret;
            }
            else
            {
                if ( !tmp -> m_in_list )
                {
                    if ( add_msg_with_notify_one ( tmp ) )
                    {
                        tmp -> m_in_list == true;
                    }
                    else
                    {
                        return -1;
                    }
                }
            }
        }
        tmp->unlock();
        m_map_lock.read_unlock();
        return ret;
    }

    std::list < std::pair < TClient_info,std::list <std::string> > > get_msg ()
    {
        std::list < std::pair < TClient_info,std::list <std::string> > > res;
        std::pair <int, std::list < PService_package > > tmp;
        m_map_lock.read_lock();
        if ( ( tmp = m_data.get_all_with_wait_notify() ).first > 0 )
        {
            for ( auto & p : tmp.second )
            {
                p->lock();
                res.push_back(p->getMessage());
                p->unlock();
            }
        }
        m_map_lock.read_unlock();

        return res;
    }
};
