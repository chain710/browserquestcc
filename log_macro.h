#ifndef _LOG_MACRO_H_
#define _LOG_MACRO_H_

#include <log4cplus/logger.h>
#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>
#include <log4cplus/consoleappender.h>
#include <log4cplus/layout.h>
#include <log4cplus/helpers/pointer.h>
#include <log4cplus/ndc.h>
#include <log4cplus/hierarchy.h>

#define L_LOG_INST (log4cplus::Logger::getInstance(LOG4CPLUS_TEXT("browserquest")))

#define L_TRACE(logFmt, ...) LOG4CPLUS_TRACE_FMT(L_LOG_INST, logFmt, __VA_ARGS__)
#define L_DEBUG(logFmt, ...) LOG4CPLUS_DEBUG_FMT(L_LOG_INST, logFmt, __VA_ARGS__)
#define L_INFO(logFmt, ...) LOG4CPLUS_INFO_FMT(L_LOG_INST, logFmt, __VA_ARGS__)
#define L_WARN(logFmt, ...) LOG4CPLUS_WARN_FMT(L_LOG_INST, logFmt, __VA_ARGS__)
#define L_ERROR(logFmt, ...) LOG4CPLUS_ERROR_FMT(L_LOG_INST, logFmt, __VA_ARGS__)
#define L_FATAL(logFmt, ...) LOG4CPLUS_FATAL_FMT(L_LOG_INST, logFmt, __VA_ARGS__)

#endif
