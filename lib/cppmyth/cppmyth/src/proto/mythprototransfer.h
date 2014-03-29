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

#ifndef MYTHPROTOTRANSFER_H
#define	MYTHPROTOTRANSFER_H

#include "mythprotobase.h"

#define PROTO_TRANSFER_RCVBUF     64000

namespace Myth
{

  class ProtoTransfer;
  typedef MYTH_SHARED_PTR<ProtoTransfer> ProtoTransferPtr;

  class ProtoTransfer : public ProtoBase
  {
  public:
    ProtoTransfer(const std::string& server, unsigned port);

    void Close();
    bool Open(const std::string& pathname, const std::string& sgname);
    uint32_t GetFileId() const;
    std::string GetPathName() const;
    std::string GetStorageGroupName() const;

    size_t ReadData(void *buffer, size_t n);

    int64_t fileSize;
    int64_t filePosition;

  private:
    uint32_t m_fileId;
    std::string m_pathName;
    std::string m_storageGroupName;

    bool Open();
    bool Announce75(const std::string& pathname, const std::string& sgname);
  };

}

#endif	/* MYTHPROTOTRANSFER_H */
