/*
 * Copyright (C) 2011
 */

#include "../di.h"

#include <unittest++/UnitTest++.h>
#include <iostream>
#include <string>

using namespace di;

namespace constructorDiTests
{
  class MyBean;

  class Foo
  {
  public:
    inline Foo() {}
    inline Foo(MyBean* bean) {}
  };

  class MyBean
  {
  public:
    Foo* foo;
    int ival;
    std::string name;

    inline MyBean(Foo* ptr) : foo(ptr), ival(-1) {}
    inline MyBean(int i) : foo(NULL), ival(i) {}
    inline MyBean(const char* n) : foo(NULL), ival(-1), name(n) {}
  };

  TEST(ci)
  {
    Context context;
    context.hasInstance(Type<Foo>());
    context.hasInstance(Type<MyBean>(),Ref<Foo>());
    context.start();
    MyBean* mybean = context.get(Type<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo != NULL);
    CHECK(mybean->ival == -1);
    CHECK(mybean->foo == context.get(Type<Foo>()));
    context.stop();
  }

  TEST(ciFailed)
  {
    Context context;
    context.hasInstance(Type<MyBean>(),Ref<Foo>());
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
    context.hasInstance(Type<MyBean>(),Ref<Foo>());
    context.hasInstance(Type<Foo>());
    context.start();
    MyBean* mybean = context.get(Type<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo != NULL);
    CHECK(mybean->ival == -1);
    CHECK(mybean->foo == context.get(Type<Foo>()));
    context.stop();
  }

  TEST(ciConstant)
  {
    Context context;
    context.hasInstance(Type<Foo>());
    
    context.hasInstance(Type<MyBean>(),Constant<int>(5));
    context.start();
    MyBean* mybean = context.get(Type<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo == NULL);
    CHECK(mybean->ival == 5);
    context.stop();
  }

  TEST(ciDiffConstant)
  {
    Context context;
    context.hasInstance(Type<Foo>());
    
    context.hasInstance(Type<MyBean>(),Constant<const char*>("Hello"));
    context.start();
    MyBean* mybean = context.get(Type<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo == NULL);
    CHECK(mybean->ival == -1);
    CHECK(mybean->name == "Hello");
    context.stop();
  }


  TEST(ciNamed)
  {
    Context context;
    context.hasInstance("foo",Type<Foo>());
    context.hasInstance(Type<MyBean>(),Ref<Foo>("foo"));
    context.start();
    MyBean* mybean = context.get(Type<MyBean>());
    CHECK(mybean != NULL);
    CHECK(mybean->foo != NULL);
    CHECK(mybean->ival == -1);
    CHECK(mybean->foo == context.get(Type<Foo>()));
    context.stop();
  }

  TEST(ciNamedFailed)
  {
    Context context;
    context.hasInstance(Type<Foo>());
    context.hasInstance(Type<MyBean>(),Ref<Foo>("foo"));
    context.hasInstance(Type<Foo>());
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
    context.hasInstance(Type<MyBean>(),Ref<Foo>());
    context.hasInstance(Type<Foo>(),Ref<MyBean>());
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

}

