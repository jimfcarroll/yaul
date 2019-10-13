
#pragma once

#include <stdio.h>

#include "WheelsIf.h"

class Wheels : public WheelsIf
{
public:
  virtual inline void turn() /* throw (const char*) */
  {
    printf("Turning!\n");
  }
};
