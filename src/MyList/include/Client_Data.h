#pragma once
#include <list>
template < typename T >
class Client_Data
{
private:
	int m_real_size;
	int m_size;
	T * m_data;
	int m_head;
	int m_end;
public:
	Client_Data(int size)
	{
		if (size < 0)
		{
			m_size = 1024;
		}
		else
		{
			m_size = size;
		}
		m_real_size = size + 2;
		m_data = new T[m_real_size];
		m_head = 0;
		m_end = 1;
	}

	Client_Data()
	{
		m_size = 1024;
		m_real_size = m_size + 2;
		m_data = new T[m_real_size];
		m_head = 0;
		m_end = 1;
	}
	~Client_Data()
	{
		if (m_data != NULL)
		{
			delete[] m_data;
			m_data = NULL;
		}
	}

	int size()
	{
		if (m_head > m_end)
		{
			return m_end + m_real_size - m_head - 1;
		}
		else
		{
			return m_end - m_head - 1;
		}
	}
	bool empty()
	{
		if ((m_head + 1) % m_real_size == m_end)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool full()
	{
		if ((m_end + 1) % m_real_size == m_head)
		{
			return true;
		}
		else
		{
			return false;
		}
	}

	bool push_back(T msg)
	{
		if (full())
		{
			return false;
		}
		else
		{
			*(m_data + m_end) = msg;
			m_end = (m_end + 1) % m_real_size;
			return true;
		}
	}

	std::pair < int, T > pop_front()
	{
		if (empty())
		{
			return std::pair < int, T >(-1, NULL);
		}
		else
		{
			m_head = (m_head + 1) % m_real_size;
			return std::pair < int, T >(1, *(m_data + m_head));
		}
	}
	std::list < T >  get_all()
	{
		std::list < T > tmp;
		if (!empty())
		{
			int next = 0;
			while ((next = (m_head + 1) % m_real_size) != m_end)
			{
				tmp.push_back(*(m_data + next));
				m_head = next;
			}
		}
		return tmp;
	}
};
