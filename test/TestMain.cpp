/*
 * Copyright (C) 2011
 */

#include "../Exception.h"

#include <unittest++/UnitTest++.h>

#include <iostream>

class DumbLogger : public YaulCommons::ILogger
{
public:
  virtual void log(int loglevel, const char* message)
  {
    std::cout << loglevel << ":" << message << std::endl;
  }
};

int main()
{
  YaulCommons::Exception::Settings s1;
  s1.setLogger(new DumbLogger);
  return UnitTest::RunAllTests();
}

