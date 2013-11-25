#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "VNSIData.h"

class CProvider
{
public:
  CProvider();
  CProvider(std::string name, int caid);
  bool operator==(const CProvider &rhs);
  std::string m_name;
  int m_caid;
  bool m_whitelist;
};

class CChannel
{
public:
  void SetCaids(char *caids);
  unsigned int m_id;
  unsigned int m_number;
  std::string m_name;
  std::string m_provider;
  bool m_radio;
  std::vector<int> m_caids;
  bool m_blacklist;
};

class CVNSIChannels
{
public:
  CVNSIChannels();
  void CreateProviders();
  void LoadProviderWhitelist();
  void LoadChannelBlacklist();
  void ExtractProviderWhitelist();
  void ExtractChannelBlacklist();
  bool IsWhitelist(CChannel &channel);
  std::vector<CChannel> m_channels;
  std::map<int, int> m_channelsMap;
  std::vector<CProvider> m_providers;
  std::vector<CProvider> m_providerWhitelist;
  std::vector<int> m_channelBlacklist;
  bool m_loaded;
  bool m_radio;

  enum
  {
    NONE,
    PROVIDER,
    CHANNEL
  }m_mode;
};
