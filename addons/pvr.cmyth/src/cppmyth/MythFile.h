#pragma once

#include "MythPointer.h"
#include "MythConnection.h"
#include "../libcmyth.h"
#include <boost/shared_ptr.hpp>

template < class T > class MythPointer;

class MythFile 
{
public:
  MythFile();
  MythFile(cmyth_file_t myth_file,MythConnection conn);
  bool IsNull();
  int Read(void* buffer,long long length);
  long long Seek(long long offset, int whence);
  unsigned long long Duration();
  unsigned long long CurrentPosition();
  void UpdateDuration(unsigned long long length);
 private:
  boost::shared_ptr< MythPointer< cmyth_file_t > > m_file_t; 
  MythConnection m_conn;
};
