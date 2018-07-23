#pragma once
#include <fstream>
#include <string>
#include <mutex>
#include "read_write_lock.h"
//具备的功能
//线程安全
class File_IO
{
private:
    std::fstream m_file_handle;
    // 0 清空 1 追加
    int m_mode;
    std::string m_filename;
    read_write_lock m_lock;
    int m_read_flg = 0;
public:
    File_IO ( );
    bool open_file( const std::string & filename, const int & mode );
    bool write_file( std::string data );
    std::fstream * read_begin();
    std::string read_file_line ( std::fstream * read_handle );
    void read_end( std::fstream * read_handle);
    void close();
};
