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

#include "VNSIChannels.h"
#include <algorithm>

CProvider::CProvider()
  :m_name(""), m_caid(0), m_whitelist(false)
{

}

CProvider::CProvider(std::string name, int caid)
  :m_name(name), m_caid(caid), m_whitelist(false)
{
};

bool CProvider::operator==(const CProvider &rhs)
{
  if (rhs.m_caid != m_caid)
    return false;
  if (rhs.m_name.compare(m_name) != 0)
    return false;
  return true;
}

void CChannel::SetCaids(char *caids)
{
  m_caids.clear();
  std::string strCaids = caids;
  size_t pos = strCaids.find("caids:");
  if(pos == strCaids.npos)
    return;

  strCaids.erase(0,6);
  std::string token;
  int caid;
  char *pend;
  while ((pos = strCaids.find(";")) != strCaids.npos)
  {
    token = strCaids.substr(0, pos);
    caid = strtol(token.c_str(), &pend, 10);
    m_caids.push_back(caid);
    strCaids.erase(0, pos+1);
  }
  if (strCaids.length() > 1)
  {
    caid = strtol(strCaids.c_str(), &pend, 10);
    m_caids.push_back(caid);
  }
}

CVNSIChannels::CVNSIChannels()
{
  m_loaded = false;
  m_mode = NONE;
  m_radio = false;
}

void CVNSIChannels::CreateProviders()
{
  std::vector<CChannel>::iterator c_it;
  std::vector<CProvider>::iterator p_it;
  CProvider provider;
  m_providers.clear();
  for (c_it=m_channels.begin(); c_it!=m_channels.end(); ++c_it)
  {
    provider.m_name = c_it->m_provider;
    for(unsigned int i=0; i<c_it->m_caids.size(); i++)
    {
      provider.m_caid = c_it->m_caids[i];
      p_it = std::find(m_providers.begin(), m_providers.end(), provider);
      if (p_it == m_providers.end())
      {
        m_providers.push_back(provider);
      }
    }
    if (c_it->m_caids.size() == 0)
    {
      provider.m_caid = 0;
      p_it = std::find(m_providers.begin(), m_providers.end(), provider);
      if (p_it == m_providers.end())
      {
        m_providers.push_back(provider);
      }
    }
  }
}

void CVNSIChannels::LoadProviderWhitelist()
{
  std::vector<CProvider>::iterator p_it;

  bool select = m_providerWhitelist.empty();
  for(p_it=m_providers.begin(); p_it!=m_providers.end(); ++p_it)
  {
    p_it->m_whitelist = select;
  }

  std::vector<CProvider>::iterator w_it;
  for(w_it=m_providerWhitelist.begin(); w_it!=m_providerWhitelist.end(); ++w_it)
  {
    p_it = std::find(m_providers.begin(), m_providers.end(), *w_it);
    if(p_it != m_providers.end())
    {
      p_it->m_whitelist = true;
    }
  }
}

void CVNSIChannels::LoadChannelBlacklist()
{
  std::map<int, int>::iterator it;
  for(unsigned int i=0; i<m_channelBlacklist.size(); i++)
  {
    it = m_channelsMap.find(m_channelBlacklist[i]);
    if(it!=m_channelsMap.end())
    {
      int idx = it->second;
      m_channels[idx].m_blacklist = true;
    }
  }
}

void CVNSIChannels::ExtractProviderWhitelist()
{
  std::vector<CProvider>::iterator it;
  m_providerWhitelist.clear();
  for(it=m_providers.begin(); it!=m_providers.end(); ++it)
  {
    if(it->m_whitelist)
      m_providerWhitelist.push_back(*it);
  }
  if(m_providerWhitelist.size() == m_providers.size())
  {
    m_providerWhitelist.clear();
  }
  else if (m_providerWhitelist.size() == 0)
  {
    m_providerWhitelist.clear();
    CProvider provider;
    provider.m_name = "no whitelist";
    provider.m_caid = 0;
    m_providerWhitelist.push_back(provider);
  }
}

void CVNSIChannels::ExtractChannelBlacklist()
{
  m_channelBlacklist.clear();
  for(unsigned int i=0; i<m_channels.size(); i++)
  {
    if(m_channels[i].m_blacklist)
      m_channelBlacklist.push_back(m_channels[i].m_id);
  }
}

bool CVNSIChannels::IsWhitelist(CChannel &channel)
{
  CProvider provider;
  std::vector<CProvider>::iterator p_it;
  provider.m_name = channel.m_provider;
  if (channel.m_caids.empty())
  {
    provider.m_caid = 0;
    p_it = std::find(m_providers.begin(), m_providers.end(), provider);
    if(p_it!=m_providers.end() && p_it->m_whitelist)
      return true;
  }
  for(unsigned int i=0; i<channel.m_caids.size(); i++)
  {
    provider.m_caid = channel.m_caids[i];
    p_it = std::find(m_providers.begin(), m_providers.end(), provider);
    if(p_it!=m_providers.end() && p_it->m_whitelist)
      return true;
  }
  return false;
}
