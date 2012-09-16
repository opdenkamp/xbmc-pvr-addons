#include "MythSGFile.h"
#include "../client.h"

using namespace ADDON;

 MythSGFile::MythSGFile()
   :m_storagegroup_file_t(new MythPointer<cmyth_storagegroup_file_t>())
 {
 }

  MythSGFile::MythSGFile(cmyth_storagegroup_file_t myth_storagegroup_file)
    : m_storagegroup_file_t(new MythPointer<cmyth_storagegroup_file_t>())
 {
   *m_storagegroup_file_t=myth_storagegroup_file;
 }

  CStdString MythSGFile::Filename()
  {
    char* name = cmyth_storagegroup_file_get_filename(*m_storagegroup_file_t);
    CStdString retval(name);
    ref_release(name);
    name = 0;
    return retval;
  }

  unsigned long long MythSGFile::Size()
  {
    unsigned long long retval = cmyth_storagegroup_file_get_size(*m_storagegroup_file_t);    
    return retval;
  }

  unsigned long MythSGFile::LastModified()
  {
    unsigned long retval = cmyth_storagegroup_file_get_lastmodified(*m_storagegroup_file_t);    
    return retval;
  }

  bool  MythSGFile::IsNull()
  {
    if(m_storagegroup_file_t==NULL)
      return true;
    return *m_storagegroup_file_t==NULL;
  }
