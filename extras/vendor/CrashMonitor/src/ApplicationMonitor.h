/* ************************************************************************************************
** Class to log watchdog lockup location to eeprom.
** Configures the watchdog to fire an interrupt before resetting the micro. When the interrupt
** fires, the program counter and 4 bytes of user data are written to the eeprom. The data can
** be examined when the micro restarts.
** ************************************************************************************************
 */

#pragma once
#include <Arduino.h>
#include <avr/wdt.h>

namespace Watchdog {
#if defined(__AVR_ATmega2560__)
    #define PROGRAM_COUNTER_SIZE 3 /* bytes*/
#else
    #define PROGRAM_COUNTER_SIZE 2 /* bytes*/
#endif

    struct CApplicationMonitorHeader {
        // The number of reports saved in the eeprom.
        uint8_t m_uSavedReports;

        // the location for the next report to be saved
        uint8_t m_uNextReport;

    } __attribute__((__packed__));

    struct CCrashReport {
        // Address of code executing when watchdog interrupt fired.
        // On the 328 & 644 this is just a word pointer. For the mega,
        // we need 3 bytes. We can use an array to make it easy.
        uint8_t m_auAddress [PROGRAM_COUNTER_SIZE];

        // User data.
        uint32_t m_uData;

    } __attribute__((__packed__));

    class CApplicationMonitor { // The address in the eeprom where crash data is saved. The
        // first byte is the number of records saved, followed by
        // the location for the next report to be saved, followed by
        // the indivual CApplicationState records.
        const int c_nBaseAddress;

        // The maximum number of crash entries stored in the eeprom.
        const int c_nMaxEntries;

        enum EConstants
        {
            DEFAULT_ENTRIES = 10
        };

        CCrashReport m_CrashReport;

    public:
        CApplicationMonitor(int nBaseAddress = 500, int nMaxEntries = DEFAULT_ENTRIES);
        void Dump(Print & rDestination, bool bOnlyIfPresent = true) const;

        enum ETimeout
        {
            Timeout_15ms  = WDTO_15MS,
            Timeout_30ms  = WDTO_30MS,
            Timeout_60ms  = WDTO_60MS,
            Timeout_120ms = WDTO_120MS,
            Timeout_250ms = WDTO_250MS,
            Timeout_500ms = WDTO_500MS,
            Timeout_1s    = WDTO_1S,
            Timeout_2s    = WDTO_2S,
#if defined(WDTO_4S)
            Timeout_4s = WDTO_4S,
#endif
#if defined(WDTO_8S)
            Timeout_8s = WDTO_8S,
#endif
        };

        void EnableWatchdog(ETimeout Timeout);
        void DisableWatchdog();

        void IAmAlive() const;

        void SetData(uint32_t uData) {
            m_CrashReport.m_uData = uData;
        }

        uint32_t GetData() const {
            return m_CrashReport.m_uData;
        }

        void WatchdogInterruptHandler(uint8_t * puProgramAddress);

    private:
        void SaveHeader(const CApplicationMonitorHeader & rReportHeader) const;
        void LoadHeader(CApplicationMonitorHeader & rReportHeader) const;

        void SaveCurrentReport(int nReportSlot) const;
        void LoadReport(int nReport, CCrashReport & rState) const;
        int GetAddressForReport(int nReport) const;

        void ReadBlock(int nBaseAddress, void * pData, uint8_t uSize) const;
        void WriteBlock(int nBaseAddress, const void * pData, uint8_t uSize) const;

        void PrintValue(Print & rDestination, const __FlashStringHelper * pLabel, uint32_t uValue,
                        uint8_t uRadix, bool bNewLine) const;
    };

} // namespace Watchdog

extern Watchdog::CApplicationMonitor ApplicationMonitor;