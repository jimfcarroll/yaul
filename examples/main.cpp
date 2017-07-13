
#include "yaul/di.h"

#include "Car.h"
#include "Engine.h"
#include "Wheels.h"

#include <stdio.h>

using namespace di;
using namespace YaulCommons;

int main(int argc, char* argv[])
{
  Context context;
  try
  {
    context.has(Instance<Car>(),Instance<EngineIf>()).postConstruct(&Car::start);

    context.has(Instance<Engine>()).isAlso(Instance<EngineIf>());

    context.start();
  }
  catch (const Exception& e)
  {
    printf("Exception: %s, %s\n", e.getExceptionType(), e.getMessage());
  }

  printf("... event loop ...\n");

  try
  {
    context.stop();
  }
  catch (const Exception& e)
  {
    printf("Exception: %s, %s\n", e.getExceptionType(), e.getMessage());
  }

}
