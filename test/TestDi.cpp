/*
 * Copyright (C) 2011
 */

#include "../di.h"

#include <UnitTest++/UnitTest++.h>
#include <iostream>
#include <string>

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
    std::string str;
  public:
    inline Bean() : test(NULL), val(-1) {}
    inline Bean(int v) : test(NULL), val(v) {}
    inline Bean(const std::string& value) : str(value), test(NULL), val(-1) {}

    inline void setMyBean(IMyBean* test_) { test = test_; }
    inline void setMyBean(MyBean* test_) { test = test_; }
    inline void setInt(int i) { val = i; }
    inline void call() { test->func(); }

    inline int getVal() { return val; }
    inline const std::string& getStr() { return str; }
  };

  TEST(TestDiSimple)
  {
    destCalled = false;
    Context context;
    context.has(Instance<MyBean>());
    context.has(Instance<Bean>()).requires(Instance<MyBean>(),&Bean::setMyBean);
    context.start();
    context.stop();
    CHECK(destCalled);
  }

  TEST(TestDiSimpleConstant)
  {
    Context context;
    context.has(Instance<Bean>()).requires(Constant<int>(5),&Bean::setInt);
    context.start();

    Bean* bean = context.get(Instance<Bean>());
    CHECK(bean != NULL);
    CHECK(bean->getVal() == 5);
    
    context.stop();
  }

  TEST(TestDiConstructorConstant)
  {
    Context context;
    std::string str = "Yo Dude";
    context.has(Instance<Bean>(),Constant<std::string>(str));
    context.start();

    Bean* bean = context.get(Instance<Bean>());
    CHECK(bean != NULL);
    CHECK(bean->getStr() == "Yo Dude");
    
    context.stop();
  }

  TEST(TestDiConstructorObjectConstant)
  {
    Context context;
    context.has(Instance<Bean>(),Constant<int>(5));
    context.start();

    Bean* bean = context.get(Instance<Bean>());
    CHECK(bean != NULL);
    CHECK(bean->getVal() == 5);
    
    context.stop();
  }

  TEST(TestDi)
  {
    destCalled = false;
    Context context;
    context.has(Instance<MyBean>()).isAlso(Instance<IMyBean>());
    context.has(Instance<Bean>()).requires(Instance<IMyBean>(),&Bean::setMyBean);
    context.start();
    context.stop();
    CHECK(destCalled);
  }

  TEST(TestDiMissingRequirement)
  {
    destCalled = false;
    bool failure = false;
    Context context;
    context.has(Instance<Bean>()).requires(Instance<IMyBean>(),&Bean::setMyBean);
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
    context.has(Instance<MyBean>()).isAlso(Instance<IMyBean>());
    context.has(Instance<MyBean>()).isAlso(Instance<IMyBean>());
    context.has(Instance<Bean>()).requires(Instance<IMyBean>(),&Bean::setMyBean);
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
    context.has(Instance<Foo>()).
      requires(Instance<Bar>(), &Foo::setBar).
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
    context.has(Instance<Bar>());

    context.start();

    CHECK(context.isStarted());
    CHECK(!context.isStopped());

    Foo* foo = context.get(Instance<Foo>());
    CHECK(foo != NULL);
    CHECK(foo->calledPostConstruct);
  }

  TEST(TestSimpleExampleWithIds)
  {
    Context context;
    context.has("foo",Instance<Foo>()).requires(Instance<Bar>("bar"), &Foo::setBar);

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
    context.has(Instance<Bar>());

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
    context.has("bar",Instance<Bar>());
    context.start();
    CHECK(context.isStarted());
    context.stop();
    CHECK(context.isStopped());
  }

  TEST(TestSimpleExampleWithDups)
  {
    Context context;
    context.has(Instance<Foo>()).requires(Instance<Bar>("bar1"), &Foo::setBar);
    context.has(Instance<Foo>()).requires(Instance<Bar>("bar2"), &Foo::setBar);
    context.has("bar1",Instance<Bar>());
    context.has("bar2",Instance<Bar>());
    context.start();

    Foo* foo = context.get(Instance<Foo>());
    Bar* bar1 = context.get(Instance<Bar>(),"bar1");
    Bar* bar2 = context.get(Instance<Bar>(),"bar2");

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
    context.has(Instance<Foo>()).requires(Instance<IBar>(), &Foo::setIBar);
    context.has(Instance<Bar>()).isAlso(Instance<IBar>());

    context.start();
    CHECK(context.isStarted());

    Foo* foo = context.get(Instance<Foo>());
    CHECK(foo);

    Bar* bar = context.get(Instance<Bar>());
    CHECK(bar);

    foo->call();
    CHECK(bar->called);
  }

  TEST(TestSimpleExampleWithId)
  {
    Context context;
    context.has(Instance<Foo>()).requires(Instance<IBar>("bar"), &Foo::setIBar);
    context.has("bar",Instance<Bar>()).isAlso(Instance<IBar>());

    context.start();
    CHECK(context.isStarted());

    Foo* foo = context.get(Instance<Foo>());
    CHECK(foo);

    Bar* bar = context.get(Instance<Bar>());
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
    context.has(Instance<Foo>()).postConstruct(&Foo::postConstruct);
    context.start();

    Foo* foo = context.get(Instance<Foo>());

    CHECK(foo != NULL);
    CHECK(foo->calledPostConstruct);
  }

  TEST(TestParentPostConstruct)
  {
    Context context;
    context.has(Instance<Bar>()).postConstruct(&IBar::postConstruct);
    context.start();

    Bar* bar = context.get(Instance<Bar>());
    CHECK(bar != NULL);
    CHECK(bar->calledPostConstruct);
  }

  TEST(TestSimplePreDestroy)
  {
    calledFooPreDestroy = false;
    calledIBarPreDestroy = false;

    Context context;
    context.has(Instance<Foo>()).preDestroy(&Foo::preDestroy);
    context.start();

    context.stop();

    CHECK(calledFooPreDestroy);
  }

  TEST(TestParentPreDestroy)
  {
    calledFooPreDestroy = false;
    calledIBarPreDestroy = false;

    Context context;
    context.has(Instance<Bar>()).preDestroy(&IBar::preDestroy);
    context.start();
    context.stop();

    CHECK(calledIBarPreDestroy);
  }

  TEST(TestSimplePostConstructAndPreDestroy)
  {
    calledFooPreDestroy = false;
    calledIBarPreDestroy = false;

    Context context;
    context.has(Instance<Foo>()).postConstruct(&Foo::postConstruct).preDestroy(&Foo::preDestroy);
    context.start();

    Foo* foo = context.get(Instance<Foo>());

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
    context.has(Instance<Bar>()).postConstruct(&IBar::postConstruct).preDestroy(&IBar::preDestroy);
    context.start();

    Bar* bar = context.get(Instance<Bar>());
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
    context.has(Instance<Foo>()).requiresAll(Instance<IBar>(),&Foo::setBars);
    context.has(Instance<Bar>()).isAlso(Instance<IBar>());
    context.has(Instance<Bar>()).isAlso(Instance<IBar>());
    context.has(Instance<Bar>()).isAlso(Instance<IBar>());
    context.start();

    Foo* foo = context.get(Instance<Foo>());

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

  static Bar* fooStaticSetter = NULL;

  class Foo
  {
  public:
    inline ~Foo() { fooDestructorCalled = true; }
    inline Foo() { fooConstructorCalled = true; }
    IBar* bar;
    void setIBar(IBar* bar_) { bar = bar_; }

    static void setBarStatic(Bar* bar);
  };

  void Foo::setBarStatic(Bar* bar) {
    fooStaticSetter = bar;
  }

  TEST(TestStartStopStart)
  {
    {
      Context context;
      context.has(Instance<Foo>()).requires(Instance<IBar>(),&Foo::setIBar);
      context.has(Instance<Bar>()).isAlso(Instance<IBar>());

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

  TEST(TestStaticMemberFunction)
  {
    Context context;
    context.has(Instance<Bar>());
    context.staticMethodRequirement(&Foo::setBarStatic);

    context.start();

    CHECK(fooStaticSetter != NULL);
    CHECK(fooStaticSetter == context.get(Instance<Bar>()));
  }
}


