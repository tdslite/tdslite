#include "ApplicationMonitor.h"
#include <avr/eeprom.h>

/*
Function called when the watchdog interrupt fires. The function is naked so that
we don't get program stated pushed onto the stack. Consequently the top two
values on the stack will be the program counter when the interrupt fired. We're
going to save that in the eeprom then let the second watchdog event reset the
micro. We never return from this function.
*/
uint8_t * upStack;
extern "C" {
void appMon_asm_gate(void) __attribute__((used));

void appMon_asm_gate(void) {
    ApplicationMonitor.WatchdogInterruptHandler(upStack);
}
}

ISR(WDT_vect, ISR_NAKED) {
    // Setup a pointer to the program counter. It goes in a register so we
    // don't mess up the stack.
    upStack = (uint8_t *) SP;

    // The stack pointer on the AVR micro points to the next available location
    // so we want to go back one location to get the first byte of the address
    // pushed onto the stack when the interrupt was triggered. There will be
    // PROGRAM_COUNTER_SIZE bytes there.
    ++upStack;

    // Newer versions of GCC don't like when naked functions call regular functions
    // so we use call an assembly gate function instead
    __asm__ __volatile__("call appMon_asm_gate \n");
}

namespace Watchdog {
    /*
    Initialize the application monitor. There should only be a single instance
    of the application monitor in the whole program.
    nBaseAddress: The address in the eeprom where crash data should be stored.
    nMaxEntries: The maximum number of crash entries that should be stored in the
    eeprom. Storage of eeprom data will take up sizeof(CApplicationMonitorHeader) +
    nMaxEntries * sizeof(CCrashReport) bytes in the eeprom.
    */
    CApplicationMonitor::CApplicationMonitor(int nBaseAddress, int nMaxEntries) :
        c_nBaseAddress(nBaseAddress), c_nMaxEntries(nMaxEntries) {
        m_CrashReport.m_uData = 0;
    }

    /*
    Enable the watchdog timer & have it trigger an interrupt before
    resetting the micro. When the interrupt fires, we save the program counter
    to the eeprom.
    */
    void CApplicationMonitor::EnableWatchdog(CApplicationMonitor::ETimeout Timeout) {
        wdt_enable(Timeout);
        WDTCSR |= _BV(WDIE);
    }

    void CApplicationMonitor::DisableWatchdog() {
        wdt_disable();
    }

    /*
    Lets the watchdog timer know the program is still alive. Call this before
    the watchdog timeout ellapses to prevent program being aborted.
    */
    void CApplicationMonitor::IAmAlive() const {
        wdt_reset();
    }

    void CApplicationMonitor::Dump(Print & rDestination, bool bOnlyIfPresent) const {
        CApplicationMonitorHeader Header;
        CCrashReport Report;
        uint8_t uReport;
        uint32_t uAddress;

        LoadHeader(Header);
        if (!bOnlyIfPresent || Header.m_uSavedReports != 0) {
            rDestination.println(F("Application Monitor"));
            rDestination.println(F("-------------------"));
            PrintValue(rDestination, F("Saved reports: "), Header.m_uSavedReports, DEC, true);
            PrintValue(rDestination, F("Next report: "), Header.m_uNextReport, DEC, true);

            for (uReport = 0; uReport < Header.m_uSavedReports; ++uReport) {
                LoadReport(uReport, Report);

                rDestination.print(uReport);
                uAddress = 0;
                memcpy(&uAddress, Report.m_auAddress, PROGRAM_COUNTER_SIZE);
                PrintValue(rDestination, F(": word-address=0x"), uAddress, HEX, false);
                PrintValue(rDestination, F(": byte-address=0x"), uAddress * 2, HEX, false);
                PrintValue(rDestination, F(", data=0x"), Report.m_uData, HEX, true);
            }
        }
    }

    void CApplicationMonitor::PrintValue(Print & rDestination, const __FlashStringHelper * pLabel,
                                         uint32_t uValue, uint8_t uRadix, bool bNewLine) const {
        rDestination.print(pLabel);
        rDestination.print(uValue, uRadix);
        if (bNewLine)
            rDestination.println();
    }

    void CApplicationMonitor::WatchdogInterruptHandler(uint8_t * puProgramAddress) {
        CApplicationMonitorHeader Header;

        LoadHeader(Header);
        memcpy(m_CrashReport.m_auAddress, puProgramAddress, PROGRAM_COUNTER_SIZE);
        SaveCurrentReport(Header.m_uNextReport);

        // Update header for next time.
        ++Header.m_uNextReport;
        if (Header.m_uNextReport >= c_nMaxEntries)
            Header.m_uNextReport = 0;
        else
            ++Header.m_uSavedReports;
        SaveHeader(Header);

        // Wait for next watchdog time out to reset system.
        // If the watch dog timeout is too short, it doesn't
        // give the program much time to reset it before the
        // next timeout. So we can be a bit generous here.
        wdt_enable(WDTO_120MS);
        while (true)
            ;
    }

    void CApplicationMonitor::LoadHeader(CApplicationMonitorHeader & rReportHeader) const {
        ReadBlock(c_nBaseAddress, &rReportHeader, sizeof(rReportHeader));

        // Ensure the report structure is valid.
        if (rReportHeader.m_uSavedReports == 0xff) // eeprom is 0xff when uninitialized
            rReportHeader.m_uSavedReports = 0;
        else if (rReportHeader.m_uSavedReports > c_nMaxEntries)
            rReportHeader.m_uSavedReports = c_nMaxEntries;

        if (rReportHeader.m_uNextReport >= c_nMaxEntries)
            rReportHeader.m_uNextReport = 0;
    }

    void CApplicationMonitor::SaveHeader(const CApplicationMonitorHeader & rReportHeader) const {
        WriteBlock(c_nBaseAddress, &rReportHeader, sizeof(rReportHeader));
    }

    void CApplicationMonitor::SaveCurrentReport(int nReportSlot) const {
        WriteBlock(GetAddressForReport(nReportSlot), &m_CrashReport, sizeof(m_CrashReport));
    }

    void CApplicationMonitor::LoadReport(int nReport, CCrashReport & rState) const {
        ReadBlock(GetAddressForReport(nReport), &rState, sizeof(rState));

        // The return address is reversed when we read it off the stack. Correct that.
        // by reversing the byte order. Assuming PROGRAM_COUNTER_SIZE is 2 or 3.
        uint8_t uTemp;
        uTemp                  = rState.m_auAddress [0];
        rState.m_auAddress [0] = rState.m_auAddress [PROGRAM_COUNTER_SIZE - 1];
        rState.m_auAddress [PROGRAM_COUNTER_SIZE - 1] = uTemp;
    }

    int CApplicationMonitor::GetAddressForReport(int nReport) const {
        int nAddress;

        nAddress = c_nBaseAddress + sizeof(CApplicationMonitorHeader);
        if (nReport < c_nMaxEntries)
            nAddress += nReport * sizeof(m_CrashReport);
        return nAddress;
    }

    void CApplicationMonitor::ReadBlock(int nBaseAddress, void * pData, uint8_t uSize) const {
        uint8_t * puData = (uint8_t *) pData;
        while (uSize--)
            *puData++ = eeprom_read_byte((const uint8_t *) nBaseAddress++);
    }

    void CApplicationMonitor::WriteBlock(int nBaseAddress, const void * pData,
                                         uint8_t uSize) const {
        const uint8_t * puData = (const uint8_t *) pData;
        while (uSize--)
            eeprom_write_byte((uint8_t *) nBaseAddress++, *puData++);
    }
} // namespace Watchdog
