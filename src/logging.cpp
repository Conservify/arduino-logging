#include "alogging/sprintf.h"
#include "logging.h"
#include "platform.h"
#include "assert.h"

#ifdef __cplusplus
extern "C" {
#endif

extern size_t platform_write_fn(const LogMessage *m, const char *line);

static uint32_t always_zero();

static log_message_uptime_fn_t log_uptime_fn = millis;
static log_message_time_fn_t log_time_fn = always_zero;
static log_message_write_fn_t write_fn = platform_write_fn;

static log_message_hook_fn_t log_hook_fn = nullptr;
static void *log_hook_arg = nullptr;
static bool log_hook_enabled = false;
static uint32_t log_counter = 0;
static uint8_t log_level = (uint8_t)LogLevels::INFO;

static const char *log_format = "%06" PRIu32 " %-25s ";
static bool log_show_counter = false;

static uint32_t always_zero() {
    return 0;
}

void log_configure_writer(log_message_write_fn_t new_fn) {
    write_fn = new_fn;
}

void log_configure_formatting(const char *format, bool enable_counter) {
    log_format = format;
    log_show_counter = enable_counter;
}

void log_configure_hook_register(log_message_hook_fn_t hook, void *arg) {
    log_hook_fn = hook;
    log_hook_arg = arg;
    log_hook_enabled = true;
}

void log_configure_hook(bool enabled) {
    log_hook_enabled = enabled;
}

void log_configure_level(LogLevels level) {
    log_level = (uint8_t)level;
}

uint8_t log_get_level(void) {
    return log_level;
}

bool log_is_trace(void) {
    return log_level >= (uint8_t)LogLevels::TRACE;
}

bool log_is_debug(void) {
    return log_level >= (uint8_t)LogLevels::DEBUG;
}

void log_configure_time(log_message_uptime_fn_t uptime_fn, log_message_time_fn_t time_fn) {
    log_uptime_fn = uptime_fn == nullptr ? always_zero : uptime_fn;
    log_time_fn = time_fn == nullptr ? always_zero : time_fn;
}

void log_raw(const LogMessage *m) {
    char formatted[ArduinoLoggingLineMax * 2];
    auto remaining = sizeof(formatted) - 3;
    auto pos = 0;

    if (log_show_counter) {
        pos = alogging_snprintf(formatted, remaining, log_format, m->uptime, m->number, m->facility);
    }
    else {
        pos = alogging_snprintf(formatted, remaining, log_format, m->uptime, m->facility);
    }

    // We size or local formatted buffer such that we should never have an
    // issue, test just in case.
    remaining -= pos;

    auto length = strlen(m->message);
    auto copying = length > remaining ? remaining : length;
    memcpy(formatted + pos, m->message, copying);
    pos += copying;

    if (formatted[pos - 1] != '\r' && formatted[pos - 1] != '\n') {
        #if defined(ARDUINO_LOGGING_INCLUDE_CR)
        formatted[pos++] = '\r';
        formatted[pos++] = '\n';
        #else
        formatted[pos++] = '\n';
        #endif // defined(ARDUINO_LOGGING_INCLUDE_CR)
    }

    formatted[pos] = 0;

    if (write_fn != nullptr) {
        write_fn(m, formatted);
    }

    if (log_hook_fn != nullptr) {
        if (log_hook_enabled) {
            log_hook_enabled = false;
            log_hook_fn(m, formatted, log_hook_arg);
            log_hook_enabled = true;
        }
    }
}

void valogf(LogLevels level, const char *facility, const char *f, va_list args) {
    if ((uint8_t)level < log_get_level()) {
        return;
    }

    char message[ArduinoLoggingLineMax];
    alogging_vsnprintf(message, ArduinoLoggingLineMax, f, args);

    LogMessage m;
    m.uptime = log_uptime_fn();
    m.number = log_counter++;
    m.time = log_time_fn();
    m.level = (uint8_t)level;
    m.facility = facility;
    m.message = message;

    log_raw(&m);
}

void alogf(LogLevels level, const char *facility, const char *f, ...) {
    va_list args;
    va_start(args, f);
    valogf(level, facility, f, args);
    va_end(args);
}

void logerrorf(const char *facility, const char *f, ...) {
    va_list args;
    va_start(args, f);
    valogf(LogLevels::ERROR, facility, f, args);
    va_end(args);
}

void logtracef(const char *facility, const char *f, ...) {
    va_list args;
    va_start(args, f);
    valogf(LogLevels::TRACE, facility, f, args);
    va_end(args);
}

void loginfof(const char *facility, const char *f, ...) {
    va_list args;
    va_start(args, f);
    valogf(LogLevels::INFO, facility, f, args);
    va_end(args);
}

void loginfofv(const char *facility, const char *f, va_list args) {
    valogf(LogLevels::INFO, facility, f, args);
}

#ifdef __cplusplus
}
#endif

