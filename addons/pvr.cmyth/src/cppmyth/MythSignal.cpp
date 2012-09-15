#include "MythSignal.h"

/*
 *            MythSignal
 */
  
    MythSignal::MythSignal() :m_AdapterStatus(),m_SNR(0),m_Signal(0),m_BER(0),m_UNC(0),m_ID(0) {}

    CStdString  MythSignal::AdapterStatus(){return m_AdapterStatus;}     /*!< @brief (optional) status of the adapter that's being used */
    int    MythSignal::SNR(){return m_SNR;}                       /*!< @brief (optional) signal/noise ratio */
    int    MythSignal::Signal(){return m_Signal;}                    /*!< @brief (optional) signal strength */
    long   MythSignal::BER(){return m_BER;}                       /*!< @brief (optional) bit error rate */
    long   MythSignal::UNC(){return m_UNC;}                       /*!< @brief (optional) uncorrected blocks */
    int    MythSignal::ID(){return m_ID;}

