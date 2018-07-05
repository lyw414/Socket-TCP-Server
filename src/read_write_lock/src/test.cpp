#include <thread>
#include <unistd.h>
#include "read_write_lock.h"
read_write_lock ss;
void func_read(int index )
{
    while ( true )
    {
        ss.read_lock();
        std::cout << "begin read" << std::endl;
        std::cout << "reader "<< index  << std::endl;
        std::cout << "end read" << std::endl;
        ss.read_unlock();
        usleep (500000);
    }
}
void func_write()
{
    while ( true )
    {
        ss.write_lock();
        std::cout << "begin write " << std::endl;
        std::cout << "11111111111" << std::endl;
        std::cout << "end write " << std::endl;
        ss.write_unlock();
        sleep(1);
    }
}

int main ( int argc, char *argv[] )
{
    std::thread t[5];
    for ( int iLoop = 0; iLoop < 5 ; iLoop ++ )
    {
        t[iLoop] = std::thread (func_read,iLoop);
    }
    std::thread t1[5];
    for ( int iLoop = 0; iLoop < 5 ; iLoop ++ )
    {
        t1[iLoop] = std::thread (func_write);
    }
    t1[0].join();
    return 0;
}
