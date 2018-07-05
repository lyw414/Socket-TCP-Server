#pragma once
#include <list>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include "Client_Data.h"
#ifdef LINUX_PROJ
#include <unistd.h>
#endif

/*
* @brief  线程安全 带有可读通知
*
*
*/
template < typename T >
class MyList
{
private:
	std::condition_variable m_cv;
	std::mutex m_cv_lock;
	std::mutex m_msg_lock;
	//std::list < T > m_list;
	Client_Data < T > m_list;
	int m_max_size = -1;
public:
	int size()
	{
		int tmp = 0;
		m_msg_lock.lock();
		tmp = m_list.size();
		m_msg_lock.unlock();
		return tmp;
	}
	bool set_max_size(int max_size)
	{
		m_max_size = max_size;
		return true;
	}
	bool add_msg(T msg)
	{
		m_msg_lock.lock();
		m_list.push_back(msg);
		m_msg_lock.unlock();
		return true;
	}

	bool add_msg_with_notify_one(T msg, int max = -1)
	{
		m_msg_lock.lock();
		m_list.push_back(msg);
		if (max == -1 || max * 2 > m_list.max_size() || m_list.size() > max)
		{
			m_msg_lock.unlock();
			m_cv.notify_one();
			return true;
		}
		m_msg_lock.unlock();
		return true;
	}

	bool add_msg_with_notify_all(T msg)
	{
		m_msg_lock.lock();
		m_list.push_back(msg);
		m_msg_lock.unlock();
		m_cv.notify_all();
		return true;
	}

	/*  brief 获取队列头部成员，并弹出该消息
	*  return pair.first Errcode -2 无消息  0 成功
	*         pair.second           头部成员内容
	*/
	std::pair < int, T> get_front()
	{
		std::pair < int, T > tmp;
		m_msg_lock.lock();
		if (m_list.empty())
		{
			tmp.first = -2;
			m_msg_lock.unlock();
			return tmp;
		}
		tmp = m_list.pop_front();
		m_msg_lock.unlock();
		return tmp;
	}

	/*  @brief  等待通知后读取头部消息，并弹出该消息
	*  @param[in] timeout           等待超时时间 -1标识一直等待直至取到消息
	*  return pair.first Errcode -1 等待超时  0 成功
	*         pair.second           头部成员内容
	*/
	std::pair < int, T> get_front_with_wait_notify(int timeout = -1)
	{
		std::pair < int, T> tmp;
		//查看队列是否有资源，若存在资源则取出资源 否则进入等待
		//获取资源锁 取出消息
		m_msg_lock.lock();
		if (!m_list.empty())
		{
			tmp = m_list.pop_front();
			m_msg_lock.unlock();
			return tmp;
		}
		m_msg_lock.unlock();
		std::unique_lock < std::mutex > lck(m_cv_lock);
		while (true)
		{
			if (timeout == -1)
			{
				m_cv.wait(lck);
			}
			else
			{
				if (m_cv.wait_for(lck, std::chrono::seconds(timeout)) == std::cv_status::timeout)
				{
					//获取资源锁 取出消息
					m_msg_lock.lock();
					if (m_list.empty())
					{
						tmp.first = -1;
						m_msg_lock.unlock();
						return tmp;
					}
					tmp = m_list.pop_front();
					m_msg_lock.unlock();
					return tmp;
				}
			}
			//获取资源锁 取出消息
			m_msg_lock.lock();
			if (m_list.empty())
			{
				m_msg_lock.unlock();
				continue;
			}
			tmp = m_list.pop_front();
			m_msg_lock.unlock();
			return tmp;
		}
	}

	/*  @brief  等待通知后读取列表，并清空
	*  return pair.first Errcode    -2 消息为空  0 成功
	*         pair.second           list全部内容
	*/
	std::pair < int, std::list < T > > get_all()
	{
		std::pair < int, std::list < T > > tmp;
		m_list.lock();
		if (m_list.empty())
		{
			tmp.first = -2;
			m_msg_lock.unlock();
			return tmp;
		}
		tmp.first = 1;
		tmp.second = m_list.get_all();
		m_msg_lock.unlock();
		return tmp;
	}

	/*  @brief  等待通知后读取
	*  @param[in] timeout           等待超时时间 -1标识一直等待直至取到消息
	*  return pair.first Errcode -1 等待超时  0 成功
	*         pair.second           头部成员内容
	*/
	std::pair <int, std::list < T > > get_all_with_wait_notify(int timeout = -1)
	{
		std::pair <int, std::list < T > > tmp;
		//查看队列是否有资源，若存在资源则取出资源 否则进入等待
		//获取资源锁 取出消息
		/*
		m_msg_lock.lock();
		if (!m_list.empty())
		{
		tmp.first = 1;
		tmp.second = m_list.get_all();
		m_msg_lock.unlock();
		return tmp;
		}
		m_msg_lock.unlock();
		*/
		std::unique_lock < std::mutex > lck(m_cv_lock);
		while (true)
		{
			if (timeout == -1)
			{
				m_cv.wait(lck);
			}
			else
			{
				if (m_cv.wait_for(lck, std::chrono::seconds(timeout)) == std::cv_status::timeout)
				{
					//获取资源锁 取出消息
					m_msg_lock.lock();
					if (m_list.empty())
					{
						tmp.first = -1;
						m_msg_lock.unlock();
						return tmp;
					}
					tmp.first = 1;
					tmp.second = m_list.get_all();
					m_msg_lock.unlock();
					return tmp;
				}
			}
			//获取资源锁 取出消息
			m_msg_lock.lock();
			if (m_list.empty())
			{
				m_msg_lock.unlock();
				continue;
			}
			tmp.first = 1;
			tmp.second = m_list.get_all();
			m_msg_lock.unlock();
			return tmp;
		}
	}
};
