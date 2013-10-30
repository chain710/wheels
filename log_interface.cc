#include <log_interface.h>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>

void def_log_handler(const char* filename, int line, const char* fmt, ...)
{
    va_list ap;
    fprintf(stderr, "wheels %s:%d] ", filename, line);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fprintf(stderr, "\n");
    fflush(stderr);  // Needed on MSVC.
}

static wheels::log_handler_t log_handler_ = wheels::log_handler_t(def_log_handler, NULL);

wheels::log_handler_t::log_handler_t( _write_log generic_log, void* ud ):
    fatal(generic_log),
    error(generic_log),
    info(generic_log),
    debug(generic_log),
    trace(generic_log),
    data(ud)
{
}

wheels::log_handler_t::log_handler_t():
    fatal(NULL),
    error(NULL),
    info(NULL),
    debug(NULL),
    trace(NULL)
{

}

void wheels::set_log_handler( const log_handler_t& newhandler )
{
    log_handler_ = newhandler;
}

wheels::log_handler_t& wheels::logger()
{
    return log_handler_;
}
