/*
 * Copyright (C) 2011
 */

#include "../di.h"

#include <UnitTest++/UnitTest++.h>
#include <iostream>

using namespace di;

namespace rudamentaryTests
{
  class IMyBean
  {
  public:
    virtual ~IMyBean() {}

    virtual void func() = 0;
  };

  static bool funcCalled = false;
  static bool destCalled = false;

  class MyBean : public IMyBean
  {
  public:
    virtual void func() { funcCalled = true; }
    virtual ~MyBean() { destCalled = true; }
  };

  class Bean
  {
    IMyBean* test;
    int val;
  public:

    inline void setMyBean(IMyBean* test_) { test = test_; }
    inline void setMyBean(MyBean* test_) { test = test_; }
    inline void setInt(int i) { val = i; }
    inline void call() { test->func(); }
  };

  TEST(TestDiSimple)
  {
    destCalled = false;
    Context context;
    context.hasInstance(Type<MyBean>());
    context.hasInstance(Type<Bean>()).requires(Type<MyBean>(),&Bean::setMyBean);
    context.start();
    context.stop();
    CHECK(destCalled);
  }

  TEST(TestDiSimpleConstant)
  {
    Context context;
    context.hasInstance(Type<Bean>()).requires(Constant<int>(5),&Bean::setInt);
    context.start();
    context.stop();
  }

  TEST(TestDi)
  {
    destCalled = false;
    Context context;
    context.hasInstance(Type<MyBean>()).provides(Type<IMyBean>());
    context.hasInstance(Type<Bean>()).requires(Type<IMyBean>(),&Bean::setMyBean);
    context.start();
    context.stop();
    CHECK(destCalled);
  }

  TEST(TestDiMissingRequirement)
  {
    destCalled = false;
    bool failure = false;
    Context context;
    context.hasInstance(Type<Bean>()).requires(Type<IMyBean>(),&Bean::setMyBean);
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

  TEST(TestDiAmbiguousRequirement)
  {
    destCalled = false;
    bool failure = false;
    Context context;
    context.hasInstance(Type<MyBean>()).provides(Type<IMyBean>());
    context.hasInstance(Type<MyBean>()).provides(Type<IMyBean>());
    context.hasInstance(Type<Bean>()).requires(Type<IMyBean>(),&Bean::setMyBean);
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
    CHECK(destCalled);
  }
}

namespace simpleexample
{
  class Bar
  {
  };

  class Foo
  {
  public:
    bool calledPostConstruct;
    Bar* bar;

    inline Foo() : calledPostConstruct(false) {}
    void setBar(Bar* bar_) { bar = bar_; }

    void postConstruct() { calledPostConstruct = true; }
  };

  TEST(TestSimpleExample)
  {
    Context context;
    context.hasInstance(Type<Foo>()).
      requires(Type<Bar>(), &Foo::setBar).
      postConstruct(&Foo::postConstruct);

    // notice without a Bar in the context we will have a failure.
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
    CHECK(context.isStopped());

    // but if we add the bar ...
    context.hasInstance(Type<Bar>());

    context.start();

    CHECK(context.isStarted());
    CHECK(!context.isStopped());

    Foo* foo = context.get(Type<Foo>());
    CHECK(foo != NULL);
    CHECK(foo->calledPostConstruct);
  }

  TEST(TestSimpleExampleWithIds)
  {
    Context context;
    context.hasInstance("foo",Type<Foo>()).requires(Type<Bar>("bar"), &Foo::setBar);

    // notice without a Bar in the context we will have a failure.
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

    // but if we add the bar without an id ...
    context.hasInstance(Type<Bar>());

    // ... it should still fail
    failure = false;
    try
    {
      context.start();
    }
    catch (di::DependencyInjectionException& ex)
    {
      failure = true;
    }
    CHECK(failure);

    // but if we add a Bar with a name then it should work
    context.hasInstance("bar",Type<Bar>());
    context.start();
    CHECK(context.isStarted());
    context.stop();
    CHECK(context.isStopped());
  }

  TEST(TestSimpleExampleWithDups)
  {
    Context context;
    context.hasInstance(Type<Foo>()).requires(Type<Bar>("bar1"), &Foo::setBar);
    context.hasInstance(Type<Foo>()).requires(Type<Bar>("bar2"), &Foo::setBar);
    context.hasInstance("bar1",Type<Bar>());
    context.hasInstance("bar2",Type<Bar>());
    context.start();

    Foo* foo = context.get(Type<Foo>());
    Bar* bar1 = context.get(Type<Bar>(),"bar1");
    Bar* bar2 = context.get(Type<Bar>(),"bar2");

    CHECK(foo != NULL);
    CHECK(foo->bar == bar1 || foo->bar == bar2);
  }
}

namespace abstractExample
{
  class IBar
  {
  public:
    virtual void func() = 0;
  };

  class Bar : public IBar
  {
  public:
    bool called;

    Bar() : called(false) {}

    virtual ~Bar() {}
    virtual void func() { called = true; }
  };

  class Foo
  {
  public:
    IBar* bar;
    void setIBar(IBar* bar_) { bar = bar_; }

    void call() { bar->func(); }
  };

  TEST(TestSimpleExample)
  {
    Context context;
    context.hasInstance(Type<Foo>()).requires(Type<IBar>(), &Foo::setIBar);
    context.hasInstance(Type<Bar>()).provides(Type<IBar>());

    context.start();
    CHECK(context.isStarted());

    Foo* foo = context.get(Type<Foo>());
    CHECK(foo);

    Bar* bar = context.get(Type<Bar>());
    CHECK(bar);

    foo->call();
    CHECK(bar->called);
  }

  TEST(TestSimpleExampleWithId)
  {
    Context context;
    context.hasInstance(Type<Foo>()).requires(Type<IBar>("bar"), &Foo::setIBar);
    context.hasInstance("bar",Type<Bar>()).provides(Type<IBar>());

    context.start();
    CHECK(context.isStarted());

    Foo* foo = context.get(Type<Foo>());
    CHECK(foo);

    Bar* bar = context.get(Type<Bar>());
    CHECK(bar);

    foo->call();
    CHECK(bar->called);
  }
}

namespace postConstructTest
{

  class PBar
  {
    long long something;
  };

  class SBar
  {
    long long something;
  };

  bool calledIBarPreDestroy;
  class IBar
  {
  public:
    bool calledPostConstruct;

    IBar() : calledPostConstruct(false) {}

    virtual void func() = 0;
    void postConstruct() { calledPostConstruct = true; }
    void preDestroy() { calledIBarPreDestroy = true; }
  };

  class Bar : public PBar, public IBar, public SBar
  {
  public:
    bool called;

    Bar() : called(false) {}

    virtual ~Bar() {}
    virtual void func() { called = true; }
  };

  bool calledFooPreDestroy;
  class Foo
  {
  public:
    bool calledPostConstruct;

    IBar* bar;

    inline Foo() : calledPostConstruct(false) {}
    void setBar(IBar* bar_) { bar = bar_; }

    void postConstruct() { calledPostConstruct = true; }
    void preDestroy() { calledFooPreDestroy = true; }
  };

  TEST(TestSimplePostConstruct)
  {
    Context context;
    context.hasInstance(Type<Foo>()).postConstruct(&Foo::postConstruct);
    context.start();

    Foo* foo = context.get(Type<Foo>());

    CHECK(foo != NULL);
    CHECK(foo->calledPostConstruct);
  }

  TEST(TestParentPostConstruct)
  {
    Context context;
    context.hasInstance(Type<Bar>()).postConstruct(&IBar::postConstruct);
    context.start();

    Bar* bar = context.get(Type<Bar>());
    CHECK(bar != NULL);
    CHECK(bar->calledPostConstruct);
  }

  TEST(TestSimplePreDestroy)
  {
    calledFooPreDestroy = false;
    calledIBarPreDestroy = false;

    Context context;
    context.hasInstance(Type<Foo>()).preDestroy(&Foo::preDestroy);
    context.start();

    context.stop();

    CHECK(calledFooPreDestroy);
  }

  TEST(TestParentPreDestroy)
  {
    calledFooPreDestroy = false;
    calledIBarPreDestroy = false;

    Context context;
    context.hasInstance(Type<Bar>()).preDestroy(&IBar::preDestroy);
    context.start();
    context.stop();

    CHECK(calledIBarPreDestroy);
  }

  TEST(TestSimplePostConstructAndPreDestroy)
  {
    calledFooPreDestroy = false;
    calledIBarPreDestroy = false;

    Context context;
    context.hasInstance(Type<Foo>()).postConstruct(&Foo::postConstruct).preDestroy(&Foo::preDestroy);
    context.start();

    Foo* foo = context.get(Type<Foo>());

    CHECK(foo != NULL);
    CHECK(foo->calledPostConstruct);
    CHECK(!calledFooPreDestroy);

    context.stop();

    CHECK(calledFooPreDestroy);
  }

  TEST(TestParentPostConstructAndPreDestroy)
  {
    calledFooPreDestroy = false;
    calledIBarPreDestroy = false;

    Context context;
    context.hasInstance(Type<Bar>()).postConstruct(&IBar::postConstruct).preDestroy(&IBar::preDestroy);
    context.start();

    Bar* bar = context.get(Type<Bar>());
    CHECK(bar != NULL);
    CHECK(bar->calledPostConstruct);

    CHECK(!calledIBarPreDestroy);

    context.stop();

    CHECK(calledIBarPreDestroy);
  }
}

namespace vectorTest
{
  class IBar
  {
  public:
    virtual void func() = 0;
  };

  class Bar : public IBar
  {
  public:
    bool called;

    Bar() : called(false) {}

    virtual ~Bar() {}
    virtual void func() { called = true; }
  };

  class Foo
  {
  public:
    std::vector<IBar*> bars;

    inline Foo() {}
    void setBars(const std::vector<IBar*> bar_) 
    { 
      bars = bar_;
    }
  };

  TEST(TestSimple)
  {
    Context context;
    context.hasInstance(Type<Foo>()).requiresAll(Type<IBar>(),&Foo::setBars);
    context.hasInstance(Type<Bar>()).provides(Type<IBar>());
    context.hasInstance(Type<Bar>()).provides(Type<IBar>());
    context.hasInstance(Type<Bar>()).provides(Type<IBar>());
    context.start();

    Foo* foo = context.get(Type<Foo>());

    CHECK(foo != NULL);
    CHECK(foo->bars.size() == 3);
  }

}

namespace otherTests
{
  class IBar
  {
  public:
    virtual void func() = 0;
  };

  static bool barDestructorCalled = false;

  class Bar : public IBar
  {
  public:
    bool called;

    Bar() : called(false) {}

    virtual ~Bar() { barDestructorCalled = true; }
    virtual void func() { called = true; }
  };

  static bool fooDestructorCalled = false;
  static bool fooConstructorCalled = false;

  class Foo
  {
  public:
    inline ~Foo() { fooDestructorCalled = true; }
    inline Foo() { fooConstructorCalled = true; }
    IBar* bar;
    void setIBar(IBar* bar_) { bar = bar_; }
  };

  TEST(TestStartStopStart)
  {
    {
      Context context;
      context.hasInstance(Type<Foo>()).requires(Type<IBar>(),&Foo::setIBar);
      context.hasInstance(Type<Bar>()).provides(Type<IBar>());

      CHECK(!fooDestructorCalled);
      CHECK(!fooConstructorCalled);
      CHECK(!context.isStarted());

      context.start();

      CHECK(!fooDestructorCalled);
      CHECK(fooConstructorCalled);
      CHECK(context.isStarted());

      fooConstructorCalled = false;

      context.stop();
      CHECK(fooDestructorCalled);
      CHECK(!fooConstructorCalled);
      CHECK(context.isStopped());

      fooDestructorCalled = false;

      context.start();

      CHECK(!fooDestructorCalled);
      CHECK(fooConstructorCalled);
      CHECK(context.isStarted());

      fooConstructorCalled = false;
      barDestructorCalled = false;
    } // context destructor

    CHECK(fooDestructorCalled);
    CHECK(barDestructorCalled);
    CHECK(!fooConstructorCalled);
  }
}


