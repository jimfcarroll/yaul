/*
 * Copyright (C) 2011
 */

#include "ilog.h"
#include "StdString.h"

namespace YaulCommons
{
  void ILogger::Log(int loglevel, const char *format, ... )
  {
    CStdString strData;

    strData.reserve(16384);
    va_list va;
    va_start(va, format);
    strData.FormatV(format,va);
    va_end(va);

    log(loglevel, strData);
  }
}
