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

#include "VNSIAdmin.h"
#include "responsepacket.h"
#include "requestpacket.h"
#include "vnsicommand.h"
#include <queue>
#include <stdio.h>

#if defined(HAVE_GL)
#include <GL/gl.h>
#endif

#define CONTROL_RENDER_ADDON             9
#define CONTROL_MENU                    10
#define CONTROL_OSD_BUTTON              13

#define ACTION_NONE                    0
#define ACTION_MOVE_LEFT               1
#define ACTION_MOVE_RIGHT              2
#define ACTION_MOVE_UP                 3
#define ACTION_MOVE_DOWN               4
#define ACTION_SELECT_ITEM             7
#define ACTION_PREVIOUS_MENU          10
#define ACTION_SHOW_INFO              11

#define REMOTE_0                    58  // remote keys 0-9. are used by multiple windows
#define REMOTE_1                    59  // for example in videoFullScreen.xml window id=2005 you can
#define REMOTE_2                    60  // enter time (mmss) to jump to particular point in the movie
#define REMOTE_3                    61
#define REMOTE_4                    62  // with spincontrols you can enter 3digit number to quickly set
#define REMOTE_5                    63  // spincontrol to desired value
#define REMOTE_6                    64
#define REMOTE_7                    65
#define REMOTE_8                    66
#define REMOTE_9                    67
#define ACTION_NAV_BACK             92

#define ACTION_TELETEXT_RED           215 // Teletext Color buttons to control TopText
#define ACTION_TELETEXT_GREEN         216 //    "       "      "    "     "       "
#define ACTION_TELETEXT_YELLOW        217 //    "       "      "    "     "       "
#define ACTION_TELETEXT_BLUE          218 //    "       "      "    "     "       "


using namespace ADDON;


class cOSDTexture
{
public:
  cOSDTexture(int bpp, int x0, int y0, int x1, int y1);
  virtual ~cOSDTexture();
  void SetPalette(int numColors, uint32_t *colors);
  void SetBlock(int x0, int y0, int x1, int y1, int stride, void *data, int len);
  void Clear();
  void GetSize(int &width, int &height);
  void GetOrigin(int &x0, int &y0) { x0 = m_x0; y0 = m_y0;};
  bool IsDirty(int &x0, int &y0, int &x1, int &y1);
  void *GetBuffer() {return (void*)m_buffer;};
protected:
  int m_x0, m_x1, m_y0, m_y1;
  int m_dirtyX0, m_dirtyX1, m_dirtyY0, m_dirtyY1;
  int m_bpp;
  int m_numColors;
  uint32_t m_palette[256];
  uint8_t *m_buffer;
  bool m_dirty;
};

cOSDTexture::cOSDTexture(int bpp, int x0, int y0, int x1, int y1)
{
  m_bpp = bpp;
  m_x0 = x0;
  m_x1 = x1;
  m_y0 = y0;
  m_y1 = y1;
  m_buffer = new uint8_t[(x1-x0+1)*(y1-y0+1)*sizeof(uint32_t)];
  memset(m_buffer,0, (x1-x0+1)*(y1-y0+1)*sizeof(uint32_t));
  m_dirtyX0 = m_dirtyY0 = 0;
  m_dirtyX1 = x1 - x0;
  m_dirtyY1 = y1 - y0;
  m_dirty = false;
}

cOSDTexture::~cOSDTexture()
{
  if (m_buffer)
  {
    delete [] m_buffer;
    m_buffer = 0;
  }
}

void cOSDTexture::Clear()
{
  memset(m_buffer,0, (m_x1-m_x0+1)*(m_y1-m_y0+1)*sizeof(uint32_t));
  m_dirtyX0 = m_dirtyY0 = 0;
  m_dirtyX1 = m_x1 - m_x0;
  m_dirtyY1 = m_y1 - m_y0;
  m_dirty = false;
}

void cOSDTexture::SetBlock(int x0, int y0, int x1, int y1, int stride, void *data, int len)
{
  int line = y0;
  int col;
  int color;
  int width = m_x1 - m_x0 + 1;
  uint8_t *dataPtr = (uint8_t*)data;
  int pos = 0;
  uint32_t *buffer = (uint32_t*)m_buffer;
  while (line <= y1)
  {
    int lastPos = pos;
    col = x0;
    int offset = line*width;
    while (col <= x1)
    {
      if (pos >= len)
      {
        XBMC->Log(LOG_ERROR, "cOSDTexture::SetBlock: reached unexpected end of buffer");
        return;
      }
      color = dataPtr[pos];
      if (m_bpp == 8)
      {
        buffer[offset+col] = m_palette[color];
      }
      else if (m_bpp == 4)
      {
        buffer[offset+col] = m_palette[color & 0x0F];
      }
      else if (m_bpp == 2)
      {
        buffer[offset+col] = m_palette[color & 0x03];
      }
      else if (m_bpp == 1)
      {
        buffer[offset+col] = m_palette[color & 0x01];
      }
      pos++;
      col++;
    }
    line++;
    pos = lastPos + stride;
  }
  if (x0 < m_dirtyX0) m_dirtyX0 = x0;
  if (x1 > m_dirtyX1) m_dirtyX1 = x1;
  if (y0 < m_dirtyY0) m_dirtyY0 = y0;
  if (y1 > m_dirtyY1) m_dirtyY1 = y1;
  m_dirty = true;
}

void cOSDTexture::SetPalette(int numColors, uint32_t *colors)
{
  m_numColors = numColors;
  for (int i=0; i<m_numColors; i++)
  {
    // convert from ARGB to RGBA
    m_palette[i] = (colors[i] & 0x00FF0000) | (colors[i] & 0x0000FF00) | (colors[i] & 0x000000FF) | (colors[i] & 0xFF000000);
  }
}

void cOSDTexture::GetSize(int &width, int &height)
{
  width = m_x1 - m_x0 + 1;
  height = m_y1 - m_y0 + 1;
}

bool cOSDTexture::IsDirty(int &x0, int &y0, int &x1, int &y1)
{
  bool ret = m_dirty;
  x0 = m_dirtyX0;
  x1 = m_dirtyX1;
  y0 = m_dirtyY0;
  y1 = m_dirtyY1;
  m_dirty = false;
  return ret;
}
//-----------------------------------------------------------------------------
#define MAX_TEXTURES 10

class cOSDRender
{
public:
  cOSDRender();
  virtual ~cOSDRender();
  void SetOSDSize(int width, int height) {m_osdWidth = width; m_osdHeight = height;};
  void SetControlSize(int width, int height) {m_controlWidth = width; m_controlHeight = height;};
  void AddTexture(int wndId, int bpp, int x0, int y0, int x1, int y1, int reset);
  void SetPalette(int wndId, int numColors, uint32_t *colors);
  void SetBlock(int wndId, int x0, int y0, int x1, int y1, int stride, void *data, int len);
  void Clear(int wndId);
  virtual void DisposeTexture(int wndId);
  virtual void FreeResources();
  virtual void Render() {};
protected:
  cOSDTexture *m_osdTextures[MAX_TEXTURES];
  std::queue<cOSDTexture*> m_disposedTextures;
  int m_osdWidth, m_osdHeight;
  int m_controlWidth, m_controlHeight;
};

cOSDRender::cOSDRender()
{
  for (int i = 0; i < MAX_TEXTURES; i++)
    m_osdTextures[i] = 0;
}

cOSDRender::~cOSDRender()
{
  for (int i = 0; i < MAX_TEXTURES; i++)
  {
    DisposeTexture(i);
  }
  FreeResources();
}

void cOSDRender::DisposeTexture(int wndId)
{
  if (m_osdTextures[wndId])
  {
    m_disposedTextures.push(m_osdTextures[wndId]);
    m_osdTextures[wndId] = 0;
  }
}

void cOSDRender::FreeResources()
{
  while (!m_disposedTextures.empty())
  {
    delete m_disposedTextures.front();
    m_disposedTextures.pop();
  }
}

void cOSDRender::AddTexture(int wndId, int bpp, int x0, int y0, int x1, int y1, int reset)
{
  if (reset)
    DisposeTexture(wndId);
  if (!m_osdTextures[wndId])
    m_osdTextures[wndId] = new cOSDTexture(bpp, x0, y0, x1, y1);
}

void cOSDRender::Clear(int wndId)
{
  if (m_osdTextures[wndId])
    m_osdTextures[wndId]->Clear();
}

void cOSDRender::SetPalette(int wndId, int numColors, uint32_t *colors)
{
  if (m_osdTextures[wndId])
    m_osdTextures[wndId]->SetPalette(numColors, colors);
}

void cOSDRender::SetBlock(int wndId, int x0, int y0, int x1, int y1, int stride, void *data, int len)
{
  if (m_osdTextures[wndId])
    m_osdTextures[wndId]->SetBlock(x0, y0, x1, y1, stride, data, len);
}

#if defined(HAVE_GL)
class cOSDRenderGL : public cOSDRender
{
public:
  cOSDRenderGL();
  virtual ~cOSDRenderGL();
  virtual void DisposeTexture(int wndId);
  virtual void FreeResources();
  virtual void Render();
protected:
  GLuint m_hwTextures[MAX_TEXTURES];
  std::queue<GLuint> m_disposedHwTextures;
};

cOSDRenderGL::cOSDRenderGL()
{
  for (int i = 0; i < MAX_TEXTURES; i++)
    m_hwTextures[i] = 0;
}

cOSDRenderGL::~cOSDRenderGL()
{
  for (int i = 0; i < MAX_TEXTURES; i++)
  {
    DisposeTexture(i);
  }
  FreeResources();
}

void cOSDRenderGL::DisposeTexture(int wndId)
{
  if (m_hwTextures[wndId])
  {
    m_disposedHwTextures.push(m_hwTextures[wndId]);
    m_hwTextures[wndId] = 0;
  }
  cOSDRender::DisposeTexture(wndId);
}

void cOSDRenderGL::FreeResources()
{
  while (!m_disposedHwTextures.empty())
  {
    if (glIsTexture(m_disposedHwTextures.front()))
    {
      glFinish();
      glDeleteTextures(1, &m_disposedHwTextures.front());
      m_disposedHwTextures.pop();
    }
  }
  cOSDRender::FreeResources();
}

void cOSDRenderGL::Render()
{
  glMatrixMode (GL_MODELVIEW);
  glPushMatrix ();
  glLoadIdentity ();
  glMatrixMode (GL_PROJECTION);
  glPushMatrix ();
  glLoadIdentity ();
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glColor4f(1.0f, 1.0f, 1.0f, 0.75f);

  for (int i = 0; i < MAX_TEXTURES; i++)
  {
    int width, height, offsetX, offsetY;
    int x0,x1,y0,y1;
    bool dirty;

    if (m_osdTextures[i] == 0)
      continue;

    m_osdTextures[i]->GetSize(width, height);
    m_osdTextures[i]->GetOrigin(offsetX, offsetY);
    dirty = m_osdTextures[i]->IsDirty(x0,y0,x1,y1);

    // create gl texture
    if (dirty && !glIsTexture(m_hwTextures[i]))
    {
      glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
      glEnable(GL_TEXTURE_2D);
      glGenTextures(1, &m_hwTextures[i]);
      glBindTexture(GL_TEXTURE_2D, m_hwTextures[i]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_osdTextures[i]->GetBuffer());
      glPopClientAttrib();
    }
    // update texture
    else if (dirty)
    {
      glPushClientAttrib(GL_CLIENT_PIXEL_STORE_BIT);
      glEnable(GL_TEXTURE_2D);
      glBindTexture(GL_TEXTURE_2D, m_hwTextures[i]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
      glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, width);
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, x0);
      glPixelStorei(GL_UNPACK_SKIP_ROWS, y0);
      glTexSubImage2D(GL_TEXTURE_2D, 0, x0, y0, x1-x0+1, y1-y0+1, GL_RGBA, GL_UNSIGNED_BYTE, m_osdTextures[i]->GetBuffer());
      glPopClientAttrib();
    }

    // render texture

    // calculate ndc for OSD texture
    float destX0 = (float)offsetX*2/m_osdWidth -1;
    float destX1 = (float)(offsetX+width)*2/m_osdWidth -1;
    float destY0 = (float)offsetY*2/m_osdHeight -1;
    float destY1 = (float)(offsetY+height)*2/m_osdHeight -1;
    float aspectControl = (float)m_controlWidth/m_controlHeight;
    float aspectOSD = (float)m_osdWidth/m_osdHeight;
    if (aspectOSD > aspectControl)
    {
      destY0 *= aspectControl/aspectOSD;
      destY1 *= aspectControl/aspectOSD;
    }
    else if (aspectOSD < aspectControl)
    {
      destX0 *= aspectOSD/aspectControl;
      destX1 *= aspectOSD/aspectControl;
    }

    // y inveted
    destY0 *= -1;
    destY1 *= -1;

    glEnable(GL_TEXTURE_2D);
    glActiveTextureARB(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hwTextures[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0, 0.0);  glVertex2f(destX0, destY0);
    glTexCoord2f(1.0, 0.0);  glVertex2f(destX1, destY0);
    glTexCoord2f(1.0, 1.0);  glVertex2f(destX1, destY1);
    glTexCoord2f(0.0, 1.0);  glVertex2f(destX0, destY1);
    glEnd();
    glBindTexture (GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
  }

  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}
#endif

//-----------------------------------------------------------------------------
cVNSIAdmin::cVNSIAdmin()
{
}

cVNSIAdmin::~cVNSIAdmin()
{

}

bool cVNSIAdmin::Open(const std::string& hostname, int port, const char* name)
{

  if(!cVNSIData::Open(hostname, port, name))
    return false;

  if(!cVNSIData::Login())
    return false;

  m_bIsOsdControl = false;
#if defined(HAVE_GL)
  m_osdRender = new cOSDRenderGL();
#else
  m_osdRender = new cOSDRender();
#endif

  if (!ConnectOSD())
    return false;

  // Load the Window as Dialog
  m_window = GUI->Window_create("Admin.xml", "Confluence", false, true);
  m_window->m_cbhdl   = this;
  m_window->CBOnInit  = OnInitCB;
  m_window->CBOnFocus = OnFocusCB;
  m_window->CBOnClick = OnClickCB;
  m_window->CBOnAction= OnActionCB;
  m_window->DoModal();

  GUI->Control_releaseRendering(m_renderControl);
  GUI->Window_destroy(m_window);
  Close();

  return true;
}

bool cVNSIAdmin::OnClick(int controlId)
{
  XBMC->Log(LOG_ERROR,"--------------- id: %d", controlId);
  return false;
}

bool cVNSIAdmin::OnFocus(int controlId)
{
  if (controlId == CONTROL_OSD_BUTTON)
  {
    m_window->SetControlLabel(CONTROL_OSD_BUTTON, XBMC->GetLocalizedString(30102));
    m_window->MarkDirtyRegion();
    m_bIsOsdControl = true;
    return true;
  }
  else if (m_bIsOsdControl)
  {
    m_window->SetControlLabel(CONTROL_OSD_BUTTON, XBMC->GetLocalizedString(30103));
    m_window->MarkDirtyRegion();
    m_bIsOsdControl = false;
    return true;
  }

  return false;
}

bool cVNSIAdmin::OnInit()
{
  m_renderControl = GUI->Control_getRendering(m_window, CONTROL_RENDER_ADDON);
  m_renderControl->m_cbhdl   = this;
  m_renderControl->CBCreate = CreateCB;
  m_renderControl->CBRender = RenderCB;
  m_renderControl->CBStop = StopCB;
  m_renderControl->CBDirty = DirtyCB;
  m_renderControl->Init();

  cRequestPacket vrp;
  if (!vrp.init(VNSI_OSD_HITKEY))
  {
    XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
    return false;
  }
  vrp.add_U32(0);
  cVNSISession::TransmitMessage(&vrp);

  return true;
}

bool cVNSIAdmin::OnAction(int actionId)
{
  if (m_window->GetFocusId() != CONTROL_OSD_BUTTON && m_bIsOsdControl)
  {
    m_bIsOsdControl = false;
    m_window->SetControlLabel(CONTROL_OSD_BUTTON, XBMC->GetLocalizedString(30103));
    m_window->MarkDirtyRegion();
  }
  else if (m_window->GetFocusId() == CONTROL_OSD_BUTTON)
  {
    if (actionId == ACTION_SHOW_INFO)
    {
      m_window->SetFocusId(CONTROL_MENU);
      return true;
    }
    else if(IsVdrAction(actionId))
    {
      // send all actions to vdr
      cRequestPacket vrp;
      if (!vrp.init(VNSI_OSD_HITKEY))
      {
        XBMC->Log(LOG_ERROR, "%s - Can't init cRequestPacket", __FUNCTION__);
        return false;
      }
      vrp.add_U32(actionId);
      cVNSISession::TransmitMessage(&vrp);
      return true;
    }
  }

  if (actionId == ADDON_ACTION_CLOSE_DIALOG ||
      actionId == ADDON_ACTION_PREVIOUS_MENU ||
      actionId == ACTION_NAV_BACK)
  {
    m_window->Close();
    return true;
  }

  if (actionId == ACTION_SELECT_ITEM)
  {
    int controlID = m_window->GetFocusId();
    if (controlID == CONTROL_MENU)
    {
      if (strncmp(m_window->GetProperty("menu"), "osd", 3) == 0)
      {
        m_window->MarkDirtyRegion();
      }
    }
  }

  return false;
}

bool cVNSIAdmin::IsVdrAction(int action)
{
  if (action == ACTION_MOVE_LEFT ||
      action == ACTION_MOVE_RIGHT ||
      action == ACTION_MOVE_UP ||
      action == ACTION_MOVE_DOWN ||
      action == ACTION_SELECT_ITEM ||
      action == ACTION_PREVIOUS_MENU ||
      action == REMOTE_0 ||
      action == REMOTE_1 ||
      action == REMOTE_2 ||
      action == REMOTE_3 ||
      action == REMOTE_4 ||
      action == REMOTE_5 ||
      action == REMOTE_6 ||
      action == REMOTE_7 ||
      action == REMOTE_8 ||
      action == REMOTE_9 ||
      action == ACTION_NAV_BACK ||
      action == ACTION_TELETEXT_RED ||
      action == ACTION_TELETEXT_GREEN ||
      action == ACTION_TELETEXT_YELLOW ||
      action == ACTION_TELETEXT_BLUE)
    return true;
  else
    return false;
}

bool cVNSIAdmin::Create(int x, int y, int w, int h)
{
  if (m_osdRender)
  {
    m_osdRender->SetControlSize(w,h);
  }
  return true;
}

void cVNSIAdmin::Render()
{
  m_osdMutex.Lock();
  if (m_osdRender)
  {
    m_osdRender->Render();
    m_osdRender->FreeResources();
  }
  m_bIsOsdDirty = false;
  m_osdMutex.Unlock();
}

void cVNSIAdmin::Stop()
{
  m_osdMutex.Lock();
  if (m_osdRender)
  {
    delete m_osdRender;
    m_osdRender = NULL;
  }
  m_osdMutex.Unlock();
}

bool cVNSIAdmin::Dirty()
{
  return m_bIsOsdDirty;
}

bool cVNSIAdmin::OnInitCB(GUIHANDLE cbhdl)
{
  cVNSIAdmin* osd = static_cast<cVNSIAdmin*>(cbhdl);
  return osd->OnInit();
}

bool cVNSIAdmin::OnClickCB(GUIHANDLE cbhdl, int controlId)
{
  cVNSIAdmin* osd = static_cast<cVNSIAdmin*>(cbhdl);
  return osd->OnClick(controlId);
}

bool cVNSIAdmin::OnFocusCB(GUIHANDLE cbhdl, int controlId)
{
  cVNSIAdmin* osd = static_cast<cVNSIAdmin*>(cbhdl);
  return osd->OnFocus(controlId);
}

bool cVNSIAdmin::OnActionCB(GUIHANDLE cbhdl, int actionId)
{
  cVNSIAdmin* osd = static_cast<cVNSIAdmin*>(cbhdl);
  return osd->OnAction(actionId);
}

bool cVNSIAdmin::CreateCB(GUIHANDLE cbhdl, int x, int y, int w, int h, void *device)
{
  cVNSIAdmin* osd = static_cast<cVNSIAdmin*>(cbhdl);
  return osd->Create(x, y, w, h);
}

void cVNSIAdmin::RenderCB(GUIHANDLE cbhdl)
{
  cVNSIAdmin* osd = static_cast<cVNSIAdmin*>(cbhdl);
  osd->Render();
}

void cVNSIAdmin::StopCB(GUIHANDLE cbhdl)
{
  cVNSIAdmin* osd = static_cast<cVNSIAdmin*>(cbhdl);
  osd->Stop();
}

bool cVNSIAdmin::DirtyCB(GUIHANDLE cbhdl)
{
  cVNSIAdmin* osd = static_cast<cVNSIAdmin*>(cbhdl);
  return osd->Dirty();
}

bool cVNSIAdmin::OnResponsePacket(cResponsePacket* resp)
{
  if (resp->getChannelID() == VNSI_CHANNEL_OSD)
  {
    uint32_t wnd, color, x0, y0, x1, y1, len;
    uint8_t *data;
    resp->getOSDData(wnd, color, x0, y0, x1, y1);
    if (resp->getOpCodeID() == VNSI_OSD_OPEN)
    {
      data = resp->getUserData();
      len = resp->getUserDataLength();
      m_osdMutex.Lock();
      if (m_osdRender)
        m_osdRender->AddTexture(wnd, color, x0, y0, x1, y1, data[0]);
      m_osdMutex.Unlock();
      free(data);
    }
    else if (resp->getOpCodeID() == VNSI_OSD_SETPALETTE)
    {
      data = resp->getUserData();
      len = resp->getUserDataLength();
      m_osdMutex.Lock();
      if (m_osdRender)
        m_osdRender->SetPalette(wnd, x0, (uint32_t*)data);
      m_osdMutex.Unlock();
      free(data);
    }
    else if (resp->getOpCodeID() == VNSI_OSD_SETBLOCK)
    {
      data = resp->getUserData();
      len = resp->getUserDataLength();
      m_osdMutex.Lock();
      if (m_osdRender)
      {
        m_osdRender->SetBlock(wnd, x0, y0, x1, y1, color, data, len);
        m_bIsOsdDirty = true;
      }
      m_osdMutex.Unlock();
      free(data);
    }
    else if (resp->getOpCodeID() == VNSI_OSD_CLEAR)
    {
      m_osdMutex.Lock();
      if (m_osdRender)
        m_osdRender->Clear(wnd);
      m_bIsOsdDirty = true;
      m_osdMutex.Unlock();
    }
    else if (resp->getOpCodeID() == VNSI_OSD_CLOSE)
    {
      m_osdMutex.Lock();
      if (m_osdRender)
        m_osdRender->DisposeTexture(wnd);
      m_bIsOsdDirty = true;
      m_osdMutex.Unlock();
    }
    else if (resp->getOpCodeID() == VNSI_OSD_MOVEWINDOW)
    {
    }
    else
      return false;
  }
  else
    return false;

  return true;
}

bool cVNSIAdmin::ConnectOSD()
{
  cRequestPacket vrp;
  if (!vrp.init(VNSI_OSD_CONNECT))
    return false;

  cResponsePacket* vresp = ReadResult(&vrp);
  if (vresp == NULL || vresp->noResponse())
  {
    delete vresp;
    return false;
  }
  uint32_t osdWidth = vresp->extract_U32();
  uint32_t osdHeight = vresp->extract_U32();
  if (m_osdRender)
    m_osdRender->SetOSDSize(osdWidth, osdHeight);
  delete vresp;

  return true;
}
