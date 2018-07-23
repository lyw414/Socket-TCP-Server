#include "File_IO.h"
File_IO::File_IO()
{
    m_mode = 0;
    m_filename = "";

}
bool File_IO :: open_file( const std::string & filename, const int & mode )
{
    m_lock.write_lock();
    if ( m_filename != "" )
    {
        m_file_handle.close();
        m_filename = {};
    }

    m_filename = filename; 
    m_mode = mode;
    if ( m_filename == "" )
    {
        return false;
    }
    if ( m_mode != 1 )
    {
        m_mode = 0;
    }
    if ( m_mode == 0 )
    {
        m_file_handle.open ( m_filename, std::ios::in|std::ios::out );
    }
    else
    {
        m_file_handle.open ( m_filename, std::ios::in|std::ios::out|std::ios::app );
    } 
    m_lock.write_unlock();
    return true;
}

bool File_IO :: write_file( std::string data )
{
    m_lock.write_lock();
    m_file_handle << data;
    m_file_handle.flush();
    m_lock.write_unlock();
    return true;
}

std::fstream * File_IO :: read_begin()
{
    return new std::fstream ( m_filename ,std::ios::in);
}


std::string File_IO :: read_file_line ( std::fstream * read_handle )
{
    std::string tmp;
    m_lock.read_lock();
    (*read_handle) >> tmp;
    m_lock.read_unlock();
    return tmp;
}

void File_IO :: read_end ( std::fstream * read_handle)
{
    read_handle->close();
    delete read_handle;
}


void File_IO :: close()
{
    m_lock.write_lock();
    if ( m_filename != "")
    {
        m_file_handle.close();
        m_filename = "";
    }
    m_lock.write_unlock();
}
