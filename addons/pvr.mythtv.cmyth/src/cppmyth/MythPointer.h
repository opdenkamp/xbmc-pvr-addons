#pragma once

#include "libcmyth.h"
#include "../../../../lib/platform/threads/threads.h"

extern CHelper_libcmyth *CMYTH;

using namespace PLATFORM;

template <class T> class MythPointer
{
public:
  ~MythPointer()
  {
    CMYTH->RefRelease(m_mythpointer);
    m_mythpointer=0;
  }
  MythPointer()
  {
    m_mythpointer=0;
  }
  operator T()
  {
    return m_mythpointer;
  }
  MythPointer & operator=(const T mythpointer)
  {
        m_mythpointer=mythpointer;
        return *this;
  }
protected:
  T m_mythpointer;
};

template <class T> class MythPointerThreadSafe : public MythPointer<T>, public CMutex
{
public:
  operator T()
  {
    return this->m_mythpointer;
  }

  MythPointerThreadSafe & operator=(const T mythpointer)
  {
        this->m_mythpointer=mythpointer;
        return *this;
  }
};
