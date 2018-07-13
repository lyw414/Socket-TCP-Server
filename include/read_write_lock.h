#pragma once
#include <iostream>
#include <mutex>
#ifndef LINUX_PROJ 
#include <shared_mutex>
#endif
//������ д��ռ
//д���� ����дʱ ���������� �ȴ����ڶ���ɺ� ����д
class read_write_lock
{
#ifdef LINUX_PROJ
private:
    int m_reader = 0;
    int m_writer_flg = 0;
    std::mutex m_read_lock; 

    //写优先标�?终止读操�?
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
