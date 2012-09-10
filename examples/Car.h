
#pragma once

#include "EngineIf.h"
#include "WheelsIf.h"

class Car
{
  EngineIf* engine;
  WheelsIf* wheels;
public:
  inline Car() : engine(NULL) {}
  inline Car(EngineIf* e) : engine(e) {}

  inline void SetEngine(EngineIf* e) { engine = e; }
  inline void SetWheels(WheelsIf* w) { wheels = w; }

  inline void start()
  { 
    if (engine)
      engine->start();
    else
      throw "Can't start without Engine.";
  }

  inline void go()
  {
    if (wheels)
      wheels->turn();
    else
      throw "Can't go without wheels.";
  }
};
