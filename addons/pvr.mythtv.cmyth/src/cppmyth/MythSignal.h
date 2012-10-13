#pragma once
/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "platform/util/StdString.h"

class MythSignal
{
public:
  friend class MythEventHandler;

  MythSignal();

  CStdString AdapterStatus() const; /*!< @brief (optional) status of the adapter that's being used */
  int  SNR() const;                 /*!< @brief (optional) signal/noise ratio */
  int  Signal() const;              /*!< @brief (optional) signal strength */
  long BER() const;                 /*!< @brief (optional) bit error rate */
  long UNC() const;                 /*!< @brief (optional) uncorrected blocks */
  int  ID() const;                  /*!< @brief (optional) Recorder ID */

private:
  CStdString m_AdapterStatus; /*!< @brief (optional) status of the adapter that's being used */
  int  m_SNR;                 /*!< @brief (optional) signal/noise ratio */
  int  m_Signal;              /*!< @brief (optional) signal strength */
  long m_BER;                 /*!< @brief (optional) bit error rate */
  long m_UNC;                 /*!< @brief (optional) uncorrected blocks */
  int  m_ID;                  /*!< @brief (optional) Recorder ID */
};
