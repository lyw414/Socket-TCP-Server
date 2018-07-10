#pragma once
#include <list>
#include <condition_variable>
#include <mutex>
#include <iostream>
#include "ArrayList.h"
#ifdef LINUX_PROJ
#include <unistd.h>
#endif

/*
* @brief  �̰߳�ȫ ���пɶ�֪ͨ
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
	ArrayList < T > m_list;
	int m_max_size = -1;
public:
    MyList(int fixed_size ) : m_list(fixed_size) {}
    MyList() : m_list() {}
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
		if (max == -1 || (max * 2) > m_list.max_size() || m_list.size() > max)
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

	/*  brief ��ȡ����ͷ����Ա������������Ϣ
	*  return pair.first Errcode -2 ����Ϣ  0 �ɹ�
	*         pair.second           ͷ����Ա����
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

	/*  @brief  �ȴ�֪ͨ���ȡͷ����Ϣ������������Ϣ
	*  @param[in] timeout           �ȴ���ʱʱ�� -1��ʶһֱ�ȴ�ֱ��ȡ����Ϣ
	*  return pair.first Errcode -1 �ȴ���ʱ  0 �ɹ�
	*         pair.second           ͷ����Ա����
	*/
	std::pair < int, T> get_front_with_wait_notify(int timeout = -1)
	{
		std::pair < int, T> tmp;
		//�鿴�����Ƿ�����Դ����������Դ��ȡ����Դ �������ȴ�
		//��ȡ��Դ�� ȡ����Ϣ
		if (!m_list.empty())
		{
			m_msg_lock.lock();
			tmp = m_list.pop_front();
			if (tmp.first != -1)
			{
				m_msg_lock.unlock();
				return tmp;
			}
			m_msg_lock.unlock();
		}
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
					//��ȡ��Դ�� ȡ����Ϣ
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
			//��ȡ��Դ�� ȡ����Ϣ
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

	/*  @brief  �ȴ�֪ͨ���ȡ�б������
	*  return pair.first Errcode    -2 ��ϢΪ��  0 �ɹ�
	*         pair.second           listȫ������
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

	/*  @brief  �ȴ�֪ͨ���ȡ
	*  @param[in] timeout           �ȴ���ʱʱ�� -1��ʶһֱ�ȴ�ֱ��ȡ����Ϣ
	*  return pair.first Errcode -1 �ȴ���ʱ  0 �ɹ�
	*         pair.second           ͷ����Ա����
	*/
	std::pair <int, std::list < T > > get_all_with_wait_notify(int timeout = -1)
	{
		std::pair <int, std::list < T > > tmp;
		//�鿴�����Ƿ�����Դ����������Դ��ȡ����Դ �������ȴ�
		//��ȡ��Դ�� ȡ����Ϣ
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
					//��ȡ��Դ�� ȡ����Ϣ
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
			//��ȡ��Դ�� ȡ����Ϣ
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
