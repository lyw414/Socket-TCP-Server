#pragma once
#include <iostream>
#include <mutex>
#ifndef LINUX_PROJ 
#include <shared_mutex>
#endif
//读并发 写独占
//写优先 发生写时 读操作阻塞 等待正在读完成后 立即写
class read_write_lock
{
#ifdef LINUX_PROJ
private:
    int m_reader = 0;
    int m_writer_flg = 0;
    std::mutex m_read_lock; 

    //鍐欎紭鍏堟爣璇?缁堟璇绘搷浣?
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
        m_writer_flg = 0;
        m_write_lock.unlock();
        m_operator_lock.unlock();
    }
    void read_lock()
    {
        //浣跨敤flg鍑忓皯涓婇攣娆℃暟
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
            //鍗犳湁鎿嶄綔
            m_write_lock.unlock();
        }
        m_read_lock.unlock();

    }
#else
private:
	std::shared_timed_mutex m_operator_lock;
	//std::shared_mutex operator_lock;
public:
    void write_lock()
    {
        m_operator_lock.lock();
    }
    void write_unlock()
    {
        m_operator_lock.unlock();
    }
    void read_lock()
    {
		m_operator_lock.lock_shared();
    }
    void read_unlock()
    {
		m_operator_lock.unlock_shared();
    }

#endif
};
