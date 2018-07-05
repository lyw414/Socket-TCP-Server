#pragma once
#include <iostream>
#include <mutex>
//读并发 写独占
//写优先 发生写时 读操作阻塞 等待正在读完成后 立即写
class read_write_lock
{
private:
    int m_reader = 0;
    int m_writer_flg = 0;
    std::mutex m_read_lock; 
    //写优先标识 终止读操作
    std::mutex m_operator_lock; 
    std::mutex m_write_lock; 
public:
    void write_lock()
    {
        m_operator_lock.lock();
        m_writer_flg = 1;
        m_write_lock.lock();

    }
    void write_unlock()
    {
        m_write_lock.unlock();
        m_writer_flg = 0;
        m_operator_lock.unlock();
    }
    void read_lock()
    {
        //使用flg减少上锁次数
        if ( m_writer_flg == 1 )
        {
            m_operator_lock.lock();
            m_operator_lock.unlock();
        }
        m_read_lock.lock();
        m_reader++;
        if ( m_reader == 1 )
        {
            m_write_lock.lock();
        }
        m_read_lock.unlock();

    }

    void read_unlock()
    {
        m_read_lock.lock();
        m_reader--;
        if ( m_reader == 0 )
        {
            //占有操作
            m_write_lock.unlock();
        }
        m_read_lock.unlock();

    }

};
