#include "MyList.h"
#include "Socket_Svr.h"
#include <iostream>
#include <thread>
#include <unistd.h>
#include <map>

MyList < int > mylist;
Socket_Svr socket_svr(2048,8,2);
int m_recv_num[10000];

int m_cli[4096];
std::string m_ip[4096];
void func_send(int index )
{
    std::string msg = "sender " + std::to_string ( index );
    while ( true )
    {
        if ( m_cli[index] > 0 )
        {
            socket_svr.send_msg ( m_cli[index],msg );
            sleep ( 1 );
        }
        else
        {
            sleep ( 1 );
        }
    }
}

void func_recv(int index)
{
    while ( true )
    {
        std::list < std::pair < TClient_info,std::list <std::string> > > tmp;
        if ( m_cli[index] > 0 )
        {
            tmp = socket_svr.recv_msg();
            m_recv_num[index] += tmp.size();
        }
        else
        {
            sleep ( 1 );
        }
    }
}

int find_cli(int sck)
{
    for ( int iLoop = 0; iLoop < 4096; iLoop ++ )
    {
        if ( m_cli[iLoop] == sck )
        {
            return iLoop;
        }
    }
    return -1;
}

int find_new ()
{
    for ( int iLoop = 0; iLoop < 4096; iLoop ++ )
    {
        if ( m_cli[iLoop] <= 0)
        {
            return iLoop;
        }
    }
    return -1;
}

void func_sck()
{
    int index = 0;
    std::list < std::pair < TClient_info,std::list <std::string> > > tmp;
    int i = 0;
    int j = 0;
    while ( true )
    {
        tmp = socket_svr.recv_connect_msg ();
        std::string m_status;
        for ( auto & p : tmp )
        {
            m_status = p.second.back();
            if ( m_status == "1" )
            {
                index = find_cli ( p.first.m_socket );
                if ( index != -1 )
                {
                    std::cout << "socket repeated" << std::endl;
                }
                else
                {
                   index = find_new();
                   m_cli[index] = p.first.m_socket;
                   i++;
                   //std::cout << "recv connected :" << i << " sokcet : " << p.first.m_socket << std::endl;
                }
            }
            else
            {
                index = find_cli ( p.first.m_socket );
                j++;
                std::cout << "recv disconnect " << j <<" sokcet : "<<p.first.m_socket<< std::endl;
                if ( index == -1 )
                {
                }
                else
                {
                   m_cli[index] = -1;
                }
            }
        }
    }
}

int main( int argc,char *argv[] )
{ 
    std::thread t1[1024];
    std::thread t2[1024];
    std::thread t3[1024];
    
    int num = 10;
    int num2 = 1;
    int num1 = 1;
    socket_svr.create_svr_thread(8988);

    for ( int iLoop = 0; iLoop < num2; iLoop++ )
    {
        t1[iLoop] = std::thread ( func_send,iLoop);
    }

    for ( int iLoop = 0; iLoop < num; iLoop++ )
    {
        t2[iLoop] = std::thread ( func_recv,iLoop);
    }

    for ( int iLoop = 0; iLoop < num1; iLoop++ )
    {
        t3[iLoop] = std::thread (func_sck);
    }
    int total;

    while ( true )
    {
        total = 0;
        for ( int iLoop = 0; iLoop < num; iLoop++ )
        {
            total += m_recv_num [iLoop];
            m_recv_num [iLoop] = 0;
        }
        sleep (1);
        std::cout << total << std::endl;
    }

    for ( int iLoop = 0; iLoop < num2; iLoop ++ )
    {
        t1[iLoop].join();
    }

    for ( int iLoop = 0; iLoop < num; iLoop ++ )
    {
        t2[iLoop].join();
    }

    for ( int iLoop = 0; iLoop < num1; iLoop ++ )
    {
        t3[iLoop].join();
    }
    
    socket_svr.stop_svr_thread();
    return 0;
}

