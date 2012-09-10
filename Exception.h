/*
 * Copyright (C) 2011
 */

#pragma once

#include "StdString.h"
#include <iostream>

#define YAUL_COPYVARARGS(fmt) va_list argList; va_start(argList, fmt); set(fmt, argList); va_end(argList)
#define YAUL_STANDARD_EXCEPTION(E) \
  class E : public YaulCommons::Exception \
  { \
  public: \
    inline E(const char* message,...) : Exception(#E) { YAUL_COPYVARARGS(message); } \
    \
    inline E(const E& other) : Exception(other) {} \
  }

namespace YaulCommons
{
  /**
   * This class is the superclass for all exceptions 
   * It provides a means for the bindings to retrieve error messages as needed.
   */
  class Exception
  {
  private:
    std::string classname;
    std::string message;

  protected:
    inline Exception(const char* classname_) : classname(classname_) { }
    inline Exception(const char* classname_, const char* message_) : classname(classname_), message(message_) { }

    inline Exception(const Exception& other) : classname(other.classname), message(other.message) {}

    /**
     * This method is called from the constructor of subclasses. It
     * will set the message from varargs as well as call log message
     */
    inline void set(const char* fmt, va_list& argList)
    {
      // this is a hack and wont work if CStdString ever has
      //  state information
      //      ((CStdString*)(&message))->FormatV(fmt, argList);
      CStdString tmps;
      tmps.FormatV(fmt, argList);
      message = tmps;
      std::cout << "EXCEPTION:" << getExceptionType() << ":" << getMessage() << std::endl;
    }

    /**
     * This message can be called from the constructor of subclasses.
     * It will set the message and log the throwing.
     */
    inline void setMessage(const char* fmt, ...)
    {
      // calls 'set'
      YAUL_COPYVARARGS(fmt);
    }

  public:
    inline const char* getMessage() const { return message.c_str(); }
    inline const char* getExceptionType() const { return classname.c_str(); }
  };
}

