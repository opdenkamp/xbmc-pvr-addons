#pragma once

#include "platform/util/StdString.h"

class MythSignal
{
  friend class MythEventHandler;
public:
    MythSignal();
    CStdString  AdapterStatus();     /*!< @brief (optional) status of the adapter that's being used */
    int    SNR();                       /*!< @brief (optional) signal/noise ratio */
    int    Signal();                    /*!< @brief (optional) signal strength */
    long   BER();                       /*!< @brief (optional) bit error rate */
    long   UNC();                       /*!< @brief (optional) uncorrected blocks */
    int    ID();                        /*!< @brief (optional) Recorder ID */    
private:
    CStdString  m_AdapterStatus;     /*!< @brief (optional) status of the adapter that's being used */
    int    m_SNR;                       /*!< @brief (optional) signal/noise ratio */
    int    m_Signal;                    /*!< @brief (optional) signal strength */
    long   m_BER;                       /*!< @brief (optional) bit error rate */
    long   m_UNC;                       /*!< @brief (optional) uncorrected blocks */
    int    m_ID;                        /*!< @brief (optional) Recorder ID */    
};