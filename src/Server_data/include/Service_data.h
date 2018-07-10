#pragma once
#include <map>
#include <vector>
#include "Service_package.h"
#include "read_write_lock.h"
#include "MyList.h"

//�̶����ȵĿͻ���������Դ
//�ṩ��Դ���书�� ����һ���̶ȼ��� new delete
//ʹ�� ȡ�� �� �ŻصĻ��� ������Դ�ĸ��ö�
class Service_data
{
private:
    int m_fixed_len;
    int index;
    std::mutex m_lock;
    PService_package * m_data;
    int m_mode;
    int m_max_size;
public:
    Service_data( int fixed_len, int mode, int max_size )
    {

        m_mode = mode;
        m_max_size = max_size;
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
        //�������Դ
        for ( int iLoop = 0 ; iLoop < m_fixed_len; iLoop ++ )
        {
            m_data[iLoop] = new Service_package(m_mode,m_max_size);
        }

    } 

    Service_data()
    {
        m_fixed_len = 2048; 
        m_mode = 1;
        m_max_size = 10240;
        m_data = new PService_package[m_fixed_len];
        index = 0;
        //�������Դ
        for ( int iLoop = 0 ; iLoop < m_fixed_len; iLoop ++ )
        {
            m_data[iLoop] = new Service_package(m_mode,m_max_size);

        }
    }

    ~Service_data()
    {
        for ( int iLoop = index; iLoop < m_fixed_len; iLoop++ )
        {
            delete m_data[iLoop];
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
            tmp = new Service_package ( m_mode, m_max_size);
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

//ʹ�ö��� ʵ���� ���� ���� ����
class Client_interpreter
{
private:
    read_write_lock  m_map_lock;
    int m_mode;
    int m_max_size;
#ifdef LINUX_PROJ
    typedef int TSOCKET;
#else
    typedef SOCKET TSOCKET;
#endif
    MyList <std::pair <TSOCKET,PService_package>> m_data;
    Service_data m_client_rc;
    std::map < TSOCKET, PService_package > m_map;
    bool IsInMap ( TSOCKET sck )
    {
        if ( m_map.find(sck) != m_map.end() ) 
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
    //max_list_size               ��Ϣ���г�ʼ������
    //max_client_size             �ͻ�����Դ�ع̶�������Դ����
    //mode                        ���ݰ�����ģʽ��0 �޻������� 1 ����С���� 2 ����¼�������棩
    //max_size                    ���ݰ��������������������ֵ �������ݰ���
    Client_interpreter(int max_list_size, int max_client_size,int mode, int max_size) : 
                                    m_data(max_list_size),
                                    m_client_rc(max_client_size,mode,max_size)
    {
        m_mode = mode;
        m_max_size = max_size;
    }

    Client_interpreter() : 
                         m_data(1024),
                         m_client_rc(1024,1,10240)
    {
        m_mode = 2;
        m_max_size = 4096;
    }
    
    bool add_client ( TSOCKET sck , std::string ip)
    {
        PService_package tmp;
        m_map_lock.write_lock();
        if ( ( tmp = Get_Client ( sck ) ) != NULL )
        {
            tmp->reset ( m_mode,m_max_size,sck,ip );
        }
        else
        {
            if( ( tmp = m_client_rc.get_one() ) == NULL )
            {
                std::cout << "Get Client Source Error!" << std::endl;
                m_map_lock.write_unlock();
                return false;
            }
            else
            {
                tmp->reset ( m_mode,m_max_size,sck,ip );
                m_map[sck] = tmp;
            }
        }
        m_map_lock.write_unlock();
        return true;
    }

    bool delete_client(TSOCKET sck)
    {
        PService_package tmp;
        m_map_lock.write_lock();
        if ( ( tmp = Get_Client ( sck ) ) != NULL )
        {
			//����map�� �������ʹ����
            m_map.erase(sck);
			if ( !(tmp->isUsed()) )
            {
                m_client_rc.put_back(tmp);
				std::cout << "delete" << std::endl;
            }
			else
			{
				tmp->setUnused();
			}
        }
        m_map_lock.write_unlock();
        return true;
    }


    bool close_all ()
    {
        m_map_lock.write_lock();
        for ( auto & p : m_map )
        {
#ifdef LINUX_PROJ
            close(p.first);
#else
			closesocket(p.first);
#endif
            m_map.erase(p.first);
            if ( ! p.second -> isUsed() )
            {
                m_client_rc.put_back(p.second);
            }
            else
            {
				p.second->setUnused();
            }
        }
        m_map_lock.write_unlock();
        return true;
    }

    bool delete_all ()
    {
        m_map_lock.write_lock();
        for ( auto & p : m_map )
        {
            m_map.erase(p.first);
            if ( ! p.second -> isUsed() )
            {
                m_client_rc.put_back(p.second);
            }
            else
            {
				p.second->setUnused();
            }
        }
        m_map_lock.write_unlock();
        return true;
    }

    bool IsSCKUsefull ( TSOCKET sck )
    {
        bool tmp = false;
        m_map_lock.read_lock();
        tmp = IsInMap ( sck );
        m_map_lock.read_unlock();
        return tmp;
    }
    
    std::string get_client_ip ( TSOCKET sck )
    {
        PService_package tmp;
        m_map_lock.read_lock();
        if ( ( tmp = Get_Client ( sck ) ) != NULL )
        {
            m_map_lock.read_unlock();
            return tmp -> m_ip;
        }
        m_map_lock.read_unlock();
        return "";
    }

    // -2 ������ﵽ���� -1 ������
    int add_msg ( TSOCKET sck, std::string msg,size_t cache = 0)
    {
        int ret =  0;
        PService_package tmp;
        m_map_lock.read_lock();
        if ( ( tmp = Get_Client ( sck ) ) != NULL )
        {
			// < 0 �����쳣 0 δ�ﵽ�������� 1 �ﵽ����
            if ( ( ret = tmp->add_msg(msg,cache) ) < 0 )
            {
                m_map_lock.read_unlock();
                return ret;
            }
            else if (ret == 1)
            {
                if ( !tmp -> isUsed() )
                {
					tmp->setUsed();
					m_data.add_msg_with_notify_one(std::pair<TSOCKET, PService_package>(sck, tmp));
                }
            }
        }
        m_map_lock.read_unlock();
        return ret;
    }

    std::list < std::pair < TClient_info,std::list <std::string> > > get_msg (int timeout = -1)
    {
        std::list < std::pair < TClient_info,std::list <std::string> > > res;
        std::pair < int, std::list < std::pair < TSOCKET, PService_package > > > tmp;
        if ( ( tmp = m_data.get_all_with_wait_notify( timeout) ).first > 0 )
        {
            m_map_lock.read_lock();
            for ( auto & p : tmp.second )
            {
                res.push_back(p.second->getMessage());
                //�鿴��Դ�Ѿ��黹
                if ( !p.second-> isUsed() )
                {
                    m_client_rc.put_back( p.second );
                    continue;
                }
                else
                {
					p.second->setUnused();
                }
            }
            m_map_lock.read_unlock();
        }
        return res;
    }
};
