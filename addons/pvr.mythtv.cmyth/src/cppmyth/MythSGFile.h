#pragma once

#include "MythPointer.h"
#include "MythConnection.h"
#include <boost/shared_ptr.hpp>

extern "C" {
#include <cmyth/cmyth.h>
};

template < class T > class MythPointer;

class MythSGFile
{
public:
  MythSGFile(cmyth_storagegroup_file_t myth_storagegroup_file);
  MythSGFile();
  CStdString Filename();
  unsigned long long Size();
  unsigned long LastModified();
  bool IsNull();
private:
  boost::shared_ptr< MythPointer< cmyth_storagegroup_file_t > > m_storagegroup_file_t;
};
