#pragma once
#include <iostream>
#include <mutex>
#include <condition_variable>
//������ д��ռ
//д���� ����дʱ ���������� �ȴ����ڶ���ɺ� ����д
class read_write_lock
{
#ifndef LINUX_PROJ
private:
	std::mutex m_lock;
	std::mutex m_read_lock;
	std::mutex m_write_lock;
	int m_reader = 0;
	int m_write_flg = 0;
public:
    void write_lock()
    {
		m_write_lock.lock();
		m_write_flg = 1;
		while ( m_reader != 0 )
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
    }
    void write_unlock()
    {
		m_write_flg = 0;
		m_write_lock.unlock();
    }
    void read_lock()
    {
		if (m_write_flg == 1)
		{
			m_write_lock.lock();
			m_write_lock.unlock();
		}
		m_read_lock.lock();
		m_reader++;
		m_read_lock.unlock();
    }
    void read_unlock()
    {
		m_read_lock.lock();
		m_reader--;
		m_read_lock.unlock();
    }
#else
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
#endif
};
