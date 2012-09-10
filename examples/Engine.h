
#pragma once

#include <stdio.h>

#include "EngineIf.h"

class Engine : public EngineIf
{
public:
  virtual inline void start() throw (const char*)
  {
    printf("Engine Started!\n");
  }
};
