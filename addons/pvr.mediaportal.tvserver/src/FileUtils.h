#pragma once

#include <string>

namespace OS
{
  class CFile
  {
  public:
    static bool Exists(const std::string& strFileName, long* errCode = NULL);
  };
};
