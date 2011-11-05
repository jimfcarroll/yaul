/*
 * Copyright (C) 2011
 */

#pragma once

#include <stdio.h>

#define LOG_LEVEL_NONE         -1 // nothing at all is logged
#define LOG_LEVEL_NORMAL        0 // shows notice, error, severe and fatal
#define LOG_LEVEL_DEBUG         1 // shows all
#define LOG_LEVEL_DEBUG_FREEMEM 2 // shows all + shows freemem on screen
#define LOG_LEVEL_DEBUG_SAMBA   3 // shows all + freemem on screen + samba debugging
#define LOG_LEVEL_MAX           LOG_LEVEL_DEBUG_SAMBA

// ones we use in the code
#define LOGDEBUG   0
#define LOGINFO    1
#define LOGNOTICE  2
#define LOGWARNING 3
#define LOGERROR   4
#define LOGSEVERE  5
#define LOGFATAL   6
#define LOGNONE    7

#ifdef __GNUC__
#define ATTRIB_LOG_FORMAT __attribute__((format(printf,3,4)))
#else
#define ATTRIB_LOG_FORMAT
#endif

namespace YaulCommons
{
  class ILogger
  {
  public:
    void Log(int loglevel, const char *format, ... ) ATTRIB_LOG_FORMAT;

    virtual void log(int loglevel, const char* message) = 0;
  };

  /**
   * Because the logger is usually set as a static member of the class it's used
   * in, this template will help define a Setter class that will set the value of
   * the logger for any particular class static member called 'logger.'
   *
   * An instance of this class can be used in the DI container functionality.
   *
   * To see an example of where this is used, see the generic Exception class 
   * that's part of this library.
   */
  template <class C> class LogSetter
  {
  public:
    inline ~LogSetter() { C::logger = NULL; }

    inline void setLogger(YaulCommons::ILogger* logger) 
    { 
      C::logger = logger;
    }
  };
}

#undef ATTRIB_LOG_FORMAT
