/*
 *      Copyright (C) 2014 Jean-Luc Barriere
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef MYTHPROTOMONITOR_H
#define	MYTHPROTOMONITOR_H

#include "mythprotobase.h"
#include "mythprotorecorder.h"

#define PROTO_MONITOR_RCVBUF      64000

namespace Myth
{

  class ProtoMonitor : public ProtoBase
  {
  public:
    ProtoMonitor(const std::string& server, unsigned port);
    ProtoMonitor(const std::string& server, unsigned port, bool blockShutdown);

    virtual bool Open();
    virtual void Close();
    virtual bool IsOpen();

    ProtoRecorderPtr GetNextFreeRecorder(int rnum)
    {
      return GetNextFreeRecorder75(rnum);
    }
    ProtoRecorderPtr GetRecorderFromNum(int rnum)
    {
      return GetRecorderFromNum75(rnum);
    }
    bool QueryFreeSpaceSummary(int64_t *total, int64_t *used)
    {
      return QueryFreeSpaceSummary75(total, used);
    }
    std::string GetSetting(const std::string& hostname, const std::string& setting)
    {
      return GetSetting75(hostname, setting);
    }
    bool SetSetting(const std::string& hostname, const std::string& setting, const std::string& value)
    {
      return SetSetting75(hostname, setting, value);
    }
    bool QueryGenpixmap(const Program& program)
    {
      return QueryGenpixmap75(program);
    }
    bool DeleteRecording(const Program& program, bool force = false, bool forget = false)
    {
      return DeleteRecording75(program, force, forget);
    }
    bool UndeleteRecording(const Program& program)
    {
      return UndeleteRecording75(program);
    }
    bool StopRecording(const Program& program)
    {
      return StopRecording75(program);
    }
    bool CancelNextRecording(int rnum, bool cancel)
    {
      return CancelNextRecording75(rnum, cancel);
    }
    StorageGroupFilePtr QuerySGFile(const std::string& hostname, const std::string& sgname, const std::string& filename)
    {
      return QuerySGFile75(hostname, sgname, filename);
    }
    MarkListPtr GetCutList(const Program& program, int unit = 0)
    {
      //if (m_protoVersion >= ??) return GetCutList__(program, unit);
      return GetCutList75(program, unit);
    }
    MarkListPtr GetCommBreakList(const Program& program, int unit = 0)
    {
      //if (m_protoVersion >= ??) return GetCommBreakList__(program, unit);
      return GetCommBreakList75(program, unit);
    }
    bool BlockShutdown()
    {
      m_blockShutdown = true;
      return BlockShutdown75();
    }
    bool AllowShutdown()
    {
      m_blockShutdown = false;
      return AllowShutdown75();
    }

  private:
    bool m_blockShutdown;

    bool Announce75();
    ProtoRecorderPtr GetNextFreeRecorder75(int rnum);
    ProtoRecorderPtr GetRecorderFromNum75(int rnum);
    bool QueryFreeSpaceSummary75(int64_t *total, int64_t *used);
    std::string GetSetting75(const std::string& hostname, const std::string& setting);
    bool SetSetting75(const std::string& hostname, const std::string& setting, const std::string& value);
    bool QueryGenpixmap75(const Program& program);
    bool DeleteRecording75(const Program& program, bool force, bool forget);
    bool UndeleteRecording75(const Program& program);
    bool StopRecording75(const Program& program);
    bool CancelNextRecording75(int rnum, bool cancel);
    StorageGroupFilePtr QuerySGFile75(const std::string& hostname, const std::string& sgname, const std::string& filename);
    MarkListPtr GetCutList75(const Program& program, int unit);
    MarkListPtr GetCutList__(const Program& program, int unit);
    MarkListPtr GetCommBreakList75(const Program& program, int unit);
    MarkListPtr GetCommBreakList__(const Program& program, int unit);
    bool BlockShutdown75();
    bool AllowShutdown75();

    // Not implemented
    //int64_t GetBookmark75(Program& program);
    //bool SetBookmark75(Program& program, int64_t bookmark);
    //bool UpdateSchedules75();
    //bool UpdateSchedules75(uint32_t recordid);
  };

}

#endif	/* MYTHPROTOMONITOR_H */
