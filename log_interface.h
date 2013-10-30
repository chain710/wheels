#ifndef _LOG_INTERFACE_H_
#define _LOG_INTERFACE_H_

// ideas comes from google protobuf

namespace wheels
{
    typedef void (*_write_log)(const char* filename, int line, const char* fmt, ...);
    struct log_handler_t
    {
        log_handler_t();
        log_handler_t(_write_log generic_log, void* ud);
        _write_log fatal;
        _write_log error;
        _write_log info;
        _write_log debug;
        _write_log trace;
        void* data;
    };

    void set_log_handler(const log_handler_t& newhandler);
    log_handler_t& logger();
}

#define W_TRACE(logFmt, ...) wheels::logger().trace(__FILE__, __LINE__, logFmt, __VA_ARGS__)
#define W_DEBUG(logFmt, ...) wheels::logger().debug(__FILE__, __LINE__, logFmt, __VA_ARGS__)
#define W_INFO(logFmt, ...) wheels::logger().info(__FILE__, __LINE__, logFmt, __VA_ARGS__)
#define W_ERROR(logFmt, ...) wheels::logger().error(__FILE__, __LINE__, logFmt, __VA_ARGS__)
#define W_FATAL(logFmt, ...) wheels::logger().fatal(__FILE__, __LINE__, logFmt, __VA_ARGS__)

#endif
