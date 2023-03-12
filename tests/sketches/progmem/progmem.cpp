#define SKETCH_SERIAL_OUTPUT

#if defined SKETCH_SERIAL_OUTPUT

    /**
     * Print a formatted string to serial output.
     *
     * @param [in] FMTSTR Format string
     * @param [in] ...    Format arguments
     *
     * The format string will be stored in program memory in order
     * to save space.
     */
    #define SERIAL_PRINTF(FMTSTR, ...)                                                             \
        [&]() {                                                                                    \
            static_assert(sizeof(FMTSTR) <= 255, "Format string cannot be greater than 255");      \
            /* Save format string into program */                                                  \
            /* memory to save flash space */                                                       \
            static const char __fmtpm [] PROGMEM = FMTSTR;                                         \
            char buf [64]                        = {};                                             \
            snprintf_P(buf, sizeof(buf), __fmtpm, ##__VA_ARGS__);                                  \
            Serial.print(buf);                                                                     \
        }()

    #define SERIAL_PRINTLNF(FMTSTR, ...)                                                           \
        SERIAL_PRINTF(FMTSTR, ##__VA_ARGS__);                                                      \
        Serial.println("");                                                                        \
        Serial.flush()

    #define SERIAL_PRINT_U16_AS_MB(U16SPAN)                                                        \
        [](tdsl::u16char_view v) {                                                                 \
            for (const auto ch : v) {                                                              \
                Serial.print(static_cast<char>(ch));                                               \
            }                                                                                      \
        }(U16SPAN)

    #if defined SKETCH_TDSL_DEBUG_LOG
        #define TDSL_DEBUG_PRINT(FMTSTR, ...)       SERIAL_PRINTF(FMTSTR, ##__VA_ARGS__)
        #define TDSL_DEBUG_PRINTLN(FMTSTR, ...)     SERIAL_PRINTLNF(FMTSTR, ##__VA_ARGS__)
        #define TDSL_DEBUG_PRINT_U16_AS_MB(U16SPAN) SERIAL_PRINT_U16_AS_MB(U16SPAN)
    #endif

#else

    #define SERIAL_PRINTF(FMTSTR, ...)
    #define SERIAL_PRINTLNF(FMTSTR, ...)
    #define SERIAL_PRINT_U16_AS_MB(U16SPAN)
#endif

#include <tdslite.h>

/**
 * Initialize serial port for logging
 */
inline void initSerialPort() {
#if defined SKETCH_SERIAL_OUTPUT
    Serial.begin(112500);
    while (!Serial)
        ;
#endif
}

const char test [] PROGMEM =
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut"
    " labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco "
    "laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in "
    "voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat "
    "cupidatat "
    "non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";

void setup() {
    initSerialPort();

    Serial.println("Iterate and print:");
    for (auto ch : tdsl::progmem_string_view{test}) {
        Serial.print(ch);
    }
    Serial.println("");
    Serial.println("Random access:");

    Serial.print(tdsl::progmem_string_view{test} [4]);
    Serial.print(tdsl::progmem_string_view{test} [96]);

    SERIAL_PRINTLNF("... setup complete ...");
}

void loop() {

    delay(250);
}
