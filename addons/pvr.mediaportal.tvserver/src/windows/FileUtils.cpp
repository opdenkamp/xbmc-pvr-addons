#include "../FileUtils.h"
#include "platform/os.h"
#include <string>
#include "platform/util/StdString.h"
#include "../utils.h"

namespace OS
{
  bool CFile::Exists(const std::string& strFileName, long* errCode)
  {
    CStdString strWinFile = ToWindowsPath(strFileName);
    CStdStringW strWFile = UTF8Util::ConvertUTF8ToUTF16(strWinFile.c_str());
    DWORD dwAttr = GetFileAttributesW(strWFile.c_str());

    if(dwAttr != 0xffffffff)
    {
      return true;
    }

    if (errCode)
      *errCode = GetLastError();

    return false;
  }
}
