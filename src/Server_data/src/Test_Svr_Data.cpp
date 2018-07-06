#include "Service_data.h"
#include <thread>
#include <string>
#include <iostream>
int num[1024];
Client_interpreter client(4096,4096,2,1024);
void func_add(int index )
{
    //注册index
    std::string msg = std::to_string(index);
    client.add_client(index,msg);
    msg += "|" + msg;
    msg += "|" + msg;
    msg += "|" + msg;
    msg += "|" + msg;
    msg += "|" + msg;

    while ( true )
    {
        client.add_msg (index,msg);
        usleep(50);
    }
}
void func_get(int index)
{
    std::list < std::pair < TClient_info,std::list <std::string> > > tmp;
    while ( true )
    {
        tmp = client.get_msg ();
        for ( auto & p : tmp )
        {
            num[index] += (p.second).size();
        }
    } 
}
int main ()
{
    std::thread t1[1024];
    std::thread t2[1024];
    
    int num1 = 100;
    int num2 = 10;
    for ( int iLoop = 0; iLoop < num1; iLoop ++ )
    {
        t1[iLoop] = std::thread(func_add,iLoop);
    }

    for ( int iLoop = 0; iLoop < num2; iLoop ++ )
    {
        t2[iLoop] = std::thread(func_get,iLoop);
    }

    int tmp;

    while (true)
    {
        tmp = 0;
        for ( int iLoop = 0; iLoop < num2; iLoop ++ )
        {
            tmp += num[iLoop];
            num[iLoop] = 0;
        }
        sleep(1);
        std::cout << "recv " << tmp << std::endl;
    }

    t2[0].join();

    return 0;
}
