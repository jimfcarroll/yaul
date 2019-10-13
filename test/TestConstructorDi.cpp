/*
 * Copyright (C) 2011
 */

#include "../di.h"

#include <UnitTest++/UnitTest++.h>
#include <iostream>
#include <string>

using namespace di;

namespace constructorDiTests
{
  class MyBean;

  class IFoo
  {
  };

  class Foo : public IFoo
  {
  public:
    MyBean* mybean;
    inline Foo() {}
    inline Foo(MyBean* bean) { mybean = bean; }
  };

  class MyBean
  {
  public:
    Foo* foo;
    int ival;
    std::string name;

    inline MyBean(IFoo* ptr) : foo((Foo*)ptr), ival(-1) {}
    inline MyBean(Foo* ptr) : foo(ptr), ival(-1) {}
    inline MyBean(int i) : foo(NULL), ival(i) {}
    inline MyBean(const char* n) : foo(NULL), ival(-1), name(n) {}
    inline MyBean(Foo* ptr, int i, const char* n) : foo(ptr), ival(i), name(n) {}
    inline MyBean(Foo* ptr, int i, const char* n, Foo* ptr2) : foo(ptr2), ival(i), name(n) {}
  };

  TEST(ciReverseDep)
  {
    Context context;
    context.has(Instance<Foo>(), Instance<MyBean>());
    context.has(Instance<MyBean>(), Constant<int>(5));
    context.start();
    MyBean* mybean = context.get(Instance<MyBean>());
    Foo* foo = context.get(Instance<Foo>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo == NULL);
    CHECK(foo != NULL);
    CHECK(mybean->ival == 5);
    CHECK(foo->mybean == mybean);
    context.stop();
  }

  TEST(ci)
  {
    Context context;
    context.has(Instance<Foo>());
    context.has(Instance<MyBean>(),Instance<Foo>());
    context.start();
    MyBean* mybean = context.get(Instance<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo != NULL);
    CHECK(mybean->ival == -1);
    CHECK(mybean->foo == context.get(Instance<Foo>()));
    context.stop();
  }

  TEST(ciIsAlso)
  {
    Context context;
    context.has(Instance<Foo>()).isAlso(Instance<IFoo>());
    context.has(Instance<MyBean>(),Instance<IFoo>());
    context.start();
    MyBean* mybean = context.get(Instance<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo != NULL);
    CHECK(mybean->ival == -1);
    CHECK(mybean->foo == context.get(Instance<Foo>()));
    context.stop();
  }

  TEST(ciFailed)
  {
    Context context;
    context.has(Instance<MyBean>(),Instance<Foo>());
    bool failure = false;
    try
    {
      context.start();
    }
    catch (di::DependencyInjectionException& ex)
    {
      failure = true;
    }
    CHECK(failure);
    context.stop();
  }

  TEST(ciReverse)
  {
    Context context;
    context.has(Instance<MyBean>(),Instance<Foo>());
    context.has(Instance<Foo>());
    context.start();
    MyBean* mybean = context.get(Instance<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo != NULL);
    CHECK(mybean->ival == -1);
    CHECK(mybean->foo == context.get(Instance<Foo>()));
    context.stop();
  }

  TEST(ci3Params)
  {
    Context context;
    context.has(Instance<MyBean>(),Instance<Foo>(),Constant<int>(5),Constant<const char*>("Hello"));
    context.has(Instance<Foo>());
    context.start();
    MyBean* mybean = context.get(Instance<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo != NULL);
    CHECK(mybean->ival == 5);
    CHECK(mybean->name == "Hello");
    CHECK(mybean->foo == context.get(Instance<Foo>()));
    context.stop();
  }

  TEST(ci4ParamsWObjConstant)
  {
    Foo* tmpfoo;
    Context context;
    context.has(Instance<MyBean>(),Instance<Foo>(),Constant<int>(5),Constant<const char*>("Hello"),Constant<Foo*>(tmpfoo = new Foo));
    context.has(Instance<Foo>());
    context.start();
    MyBean* mybean = context.get(Instance<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo != NULL);
    CHECK(mybean->ival == 5);
    CHECK(mybean->name == "Hello");
    CHECK(mybean->foo == tmpfoo);
    context.stop();
  }


  TEST(ciConstant)
  {
    Context context;
    context.has(Instance<Foo>());
    
    context.has(Instance<MyBean>(),Constant<int>(5));
    context.start();
    MyBean* mybean = context.get(Instance<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo == NULL);
    CHECK(mybean->ival == 5);
    context.stop();
  }

  TEST(ciDiffConstant)
  {
    Context context;
    context.has(Instance<Foo>());
    
    context.has(Instance<MyBean>(),Constant<const char*>("Hello"));
    context.start();
    MyBean* mybean = context.get(Instance<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo == NULL);
    CHECK(mybean->ival == -1);
    CHECK(mybean->name == "Hello");
    context.stop();
  }


  TEST(ciNamed)
  {
    Context context;
    context.has("foo",Instance<Foo>());
    context.has(Instance<MyBean>(),Instance<Foo>("foo"));
    context.start();
    MyBean* mybean = context.get(Instance<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo != NULL);
    CHECK(mybean->ival == -1);
    CHECK(mybean->foo == context.get(Instance<Foo>()));
    context.stop();
  }

  TEST(ciNamedFailed)
  {
    Context context;
    context.has(Instance<Foo>());
    context.has(Instance<MyBean>(),Instance<Foo>("foo"));
    context.has(Instance<Foo>());
    bool failure = false;
    try
    {
      context.start();
    }
    catch (di::DependencyInjectionException& ex)
    {
      failure = true;
    }
    CHECK(failure);
    context.stop();
  }

  TEST(ciCircularRef)
  {
    Context context;
    context.has(Instance<MyBean>(),Instance<Foo>());
    context.has(Instance<Foo>(),Instance<MyBean>());
    bool failure = false;
    try
    {
      context.start();
    }
    catch (di::DependencyInjectionException& ex)
    {
      failure = true;
    }
    CHECK(failure);
    context.stop();
  }

  static bool called = false;
  static void MyStaticSetter(MyBean* bean) { called = true; }

  TEST(ciStaticSetter)
  {
    Context context;
    context.has(Instance<Foo>());
    context.has(Instance<MyBean>(),Instance<Foo>());
    context.staticMethodRequirement(&MyStaticSetter);
    context.start();
    MyBean* mybean = context.get(Instance<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo != NULL);
    CHECK(mybean->ival == -1);
    CHECK(mybean->foo == context.get(Instance<Foo>()));
    CHECK(called);
    context.stop();
  }

}

