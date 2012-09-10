/*
 * Copyright (C) 2011
 */

#pragma once

#define DI__DEPENDENCY_INJECTION__H
//#define DI__DEPENDENCY_INJECTION_DEBUG

#include "StdString.h"
#include "Exception.h"

#include <typeinfo>
#include <vector>

#ifdef DI__DEPENDENCY_INJECTION_DEBUG
#include <iostream>
#endif

#include <boost/shared_ptr.hpp>

/**
 * These classes represent a simple "dependency injection" framework for c++
 * (see http://en.wikipedia.org/wiki/Dependency_injection). It's loosely based
 * on some of the concepts implemented in the core "spring framework" for java
 * (see http://www.springsource.org/).
 *
 * This API supports "setter" injection AND constructor injection.
 *
 * Assuming you are familiar with dependency injection, the way this api works
 * is that you create a "Context" and identify Instances of your classes
 * while declaring the requirements of the class. For example, suppose I have
 * a class Foo that requires an instance of class Bar. Id like to simply declare
 * that my Context has a "Foo" that requires a "Bar." In pseudo code I might
 * declare that as:
 *
 *  context:
 *    has instance of Foo:
 *        that requires a Bar (and can be set with 'setBar').
 *    has an instance of Bar
 *
 * Using this api you would say:
 *
 *   Context context;
 *   context.hasInstance(Type<Foo>()).requires(Type<Bar>(), &Foo::setBar);
 *   context.hasInstance(Type<Bar>);
 *   context.start();
 *
 * 'start' kicks off the instance lifecycles by instantiating the Type<T>'s 
 * that were specified using the class' default constructor. Then it resolves 
 * and satisfies all of the dependencies. Then the "Post Construct" lifecycle
 * stage is executed (see the section on "Lifecycle stages"). An exception is 
 * thrown if all of the dependencies cannot be resolved or if there is ambiguity 
 * in resolving dependencies.
 *
 * Constructor Injection:
 *
 * You can also specify constructor injection. If Foo had a constructor that
 * took a "Bar" the above example could be implemented as follows:
 *
 *   Context context;
 *   context.hasInstance(Type<Foo>(),Type<Bar>());
 *   context.hasInstance(Type<Bar>);
 *   context.start();
 *
 * Any Type's (or Constants, see below) specified after the instance type are
 * assumed to be parameters of the instances constructor.
 *
 * Constructor injection also allow you to pass Constants. Suppose, in the above
 * example, the only constuctor on Foo took a Bar, and also an int. You could
 * pass a constant value to the 'int' parameter of the constructor as follows:
 *
 *   Context context;
 *   context.hasInstance(Type<Foo>(),Type<Bar>(),Constant<int>(5));
 *   context.hasInstance(Type<Bar>);
 *   context.start();
 *
 * When 'context.start()' is called, the constructor invocation would look like
 * the following:
 *
 *   new Foo(bar, 5);
 *
 * ... where 'bar' is an instance of a 'Bar' also instantiated by the Context
 *
 * Abstraction/Inheritence:
 * Abstraction is not handled as cleanly as I'd hoped. The reason is because
 * I could not figure out a means of run-time traversal of a class hierarchy
 * as RTTI doesn't support it.
 *
 * What this means in a nutshell is that you must DECLARE abstractions. Expanding 
 * the above example: if a class Foo requires an interface, say IBar, then the 
 * Instance that implements IBar must declare that it satisfies that dependency.
 *
 * The following will NOT work:
 *
 *   Context context;
 *   context.hasInstance(Type<Foo>()).requires(Type<IBar>(), &Foo::setIBar);
 *   context.hasInstance(Type<Bar>);
 *   context.start();
 *
 * Even if Bar extends/implements IBar, the book keeping done by the 
 * context wont understand that the instance of Bar satisfies that dependency.
 * Instead you need to tell the context this. The line above that declares 
 * the instance of Bar should instead look like:
 *
 *   context.hasInstance(Type<Bar>().provides(Type<IBar>()));
 *
 * Set injection:
 *
 * You can specify that all of the instances of a particular type be a requirement
 *  of another instance. For example, if class Foo has a method 
 *  'void setBars(const std::vector<IBar*>)' then you can have the instance
 *  of Foo injected with all of the instances of Bar in the context as follows:
 *
 *  Context context;
 *  context.hasInstance(Type<Foo>()).requiresAll(Type<IBar>(),&Foo::setBars);
 *  context.hasInstance(Type<Bar>()).provides(Type<IBar>());
 *  context.hasInstance(Type<Bar>()).provides(Type<IBar>());
 *  context.hasInstance(Type<Bar>()).provides(Type<IBar>());
 *  context.start();
 *
 * Note that this example combines the abstraction with the set injection but it
 * would (of course) also work with simple concrete types. Given 
 * 'void Foo::setBars(const std::vector<Bar*>)' the following is fine:
 *
 *  Context context;
 *  context.hasInstance(Type<Foo>()).requiresAll(Type<Bar>(),&Foo::setBars);
 *  context.hasInstance(Type<Bar>());
 *  context.hasInstance(Type<Bar>());
 *  context.hasInstance(Type<Bar>());
 *  context.start();
 *
 * Chaining:
 *
 * Context::hasInstance as well as the Instance methods 'provides' and 'requires'
 * returns the Instance& so that you can chain calls.
 *
 * If an instance requires more declaration/clarifications they can be chained:
 *
 *   context.hasInstance(Type<Foo>()).
 *      requires(Type<Bar>(), &Foo::setBar).
 *      requires(Type<Other>(), &Foo::setOther).
 *      provides(Type<IFoo>());
 *
 * Note that, by default, all instances 'provide' their own type. The following is an
 *  error:
 *
 *   context.hasInstance(Type<Foo>()).provides(Type<Foo>());
 *
 * Using instance ids:
 *
 * It's possible to name instances so that requirements and dependencies can be explicitly
 * identified. For example:
 *
 *   context.hasInstance(Type<Foo>()).requires(Type<IBar>("bar1"), &Foo::setIBar);
 *   context.hasInstance(Type<Foo>()).requires(Type<IBar>("bar2"), &Foo::setIBar);
 *   context.hasInstance("bar1",Type<Bar>());
 *   context.hasInstance("bar2",Type<Bar>());
 *
 * Lifecycle stages:
 *
 * Instances in the context go through several "lifecycles stages" (or "phases"). These
 * are, in order:
 *
 * 1) Instantiation - instantiation of the defined instances.
 * 2) Wiring - satisfying of the requirements via calling the identified setters
 * 3) PostConstruction - which calls all of the postConstruct methods that
 *    were declared to the context.
 * ...
 * 4) PreDestruction - which calls all of the preDestroy methods that
 *    were declared to the context.
 * 5) Deletion - Deletes all of the instances that were instantiated 
 *    during the Instantiation lifecycle stage.
 *
 * Context::start executes the first three and Context::stop executes the last two.
 *
 * "PostConstruct" and "PreDestroy" are lifecycle stages where the instances are notified
 * via callbacks that were previously identified to the context using the 
 * 'Instance<T>::postConstruct' and 'Instance<T>::preDestroy' register methods respectively.
 * For example:
 *
 * context.hasInstance(Type<Foo>()).
 *     postConstruct(&Foo::postConstructMethod).
 *     preDestroy(&Foo::preDestroyMethod)...
 *
 * After the context has wired up all of the dependnecies, Foo::postConstructMethod 
 * will be called (along with any other registerd postConstruct method on any other
 * instance).
 *
 * NOTE: Currently the Context expects only one postConstruct (and/or) preDestroy callback
 * to be registered per instance. 
 *
 */

namespace di
{
  /**
   * Generic exception thrown by various DI container operations.
   */
  YAUL_STANDARD_EXCEPTION(DependencyInjectionException);

  inline std::ostream& operator<<(std::ostream& stream, const DependencyInjectionException& ex)
  {
    stream << "DependencyInjectionException:" << ex.getMessage();
    return stream;
  }

  // Nothing to see here, move along ...
  #include "internal/dibase.h"

  /**
   * This class represents the means of declaring type information
   *  to the context.
   *
   * When a Type is used in a 'requires' clause it represents a 'reference' 
   * (in the general sense) to another managed instance within the context. 
   *
   * When used for either setter injection, constructor injection, or even
   * to identify an instance in a hasInstance clause, an 'id' can optionally
   * be supplied in the constructor. This id narrows the scope of reference
   * or, in the case it's used in the 'hasInstance' clause, it names the 
   * instance being instantiated. Therefore the two lines are equivalent:
   *
   * context.hasInstance("foo",Type<Foo>());
   * context.hasInstance(Type<Foo>("foo"));
   */
  template <class T> class Type : public internal::TypeBase
  {
  public:
    inline Type() : internal::TypeBase(typeid(T)) {}
    inline Type(const char* id) : internal::TypeBase(id,typeid(T)) {}
    virtual ~Type() {}

    /**
     * public methods defined in TypeBase include:
     *
     *     const std::string Type<T>::toString() const;
     *
     * toString method returns the string representation of the Type<T> instance. This
     * is simply the typeinfo name() result wrapped in a std::string.
     *
     *     const std::type_info& Type<T>::getTypeInfo() const
     *
     * getTypeInfo returns the rtti typeinfo instance for the type T.
     *
     * operator== and operator!= are defined for Type<T> and delegate
     * to the == and != on typeinfo.
     */

    typedef T* type;

    inline T* findProvides(Context* context) const throw (DependencyInjectionException);
    inline bool available(Context* context) const;
 };

  /**
   * A Constant value can be supplied to satisfy constructor requirements during
   * constructor injection using this template.
   */
  template <typename T> class Constant
  {
  private:
    T instance;
  public:
    typedef T type;

    inline Constant(T val) : instance(val) {}
    inline Constant(const Constant& o) : instance(o.instance) {}

    inline const T& findProvides(Context* context) { return instance; }
    inline bool available(Context* context) { return true; }
  };

  // Nothing to see here, move along ...
  #include "internal/direquirement.h"

  /**
   * This template allows the declaration of object instances in a context.
   */
  template<class T> class Instance : public internal::InstanceBase
  {
    friend class Context;

  public:
    typedef void (T::*PostConstructMethod)();
    typedef void (T::*PreDestroyMethod)();

  private:
    boost::shared_ptr<T> ref;

    PostConstructMethod postConstructMethod;
    PreDestroyMethod preDestroyMethod;

    virtual void doPostConstruct()
    {
      if (postConstructMethod != NULL)
        ((*get()).*(postConstructMethod))();
    }

    virtual void doPreDestroy()
    {
      if (preDestroyMethod != NULL)
        ((*get()).*(preDestroyMethod))();
    }

    inline explicit Instance(boost::shared_ptr<internal::FactoryBase> factory, const char* name) : 
      InstanceBase(factory, name,Type<T>()), postConstructMethod(NULL), 
      preDestroyMethod(NULL) { provides(Type<T>()); }

    inline Instance(const Instance<T>& other) : 
      InstanceBase(other), ref(other.ref), postConstructMethod(NULL), 
      preDestroyMethod(NULL) { provides(Type<T>()); }

    virtual ~Instance() {}

  protected:
    virtual inline const void* getConcrete() const { return ref.get(); }

    virtual inline void instantiateInstance(Context* c) { ref = boost::shared_ptr<T>((T*)factory->create(c)); hasInstance = true; }

    virtual inline void reset() { ref.reset(); }

  public:

    /**
     * It is possible to explicitly declare that an Instance satisfies a particular
     *  requirement. This is often necessary because relationships within class
     *  hierarchies are not understood by the DI API (if someone can figure out
     *  a way to do this then be my guest).
     */
    template<typename D> inline Instance<T>& provides(const Type<D>& typeInfo) throw (DependencyInjectionException)
    {
      providesTheseTypes.push_back(new internal::TypeConverter<D,T>);
      return *this; 
    }

    /**
     * Use this method to declare that this instance requires a particular
     * dependency. Using a Ref you can alternatively supply a name for the
     * object that this instance requires.
     */
    template<typename D> inline Instance<T>& requires(const Type<D>& dependency, typename internal::Setter<T,D*>::type setter) 
    {
      requirements.push_back(new internal::Requirement<T,D>(dependency,setter));
      return *this;
    }

    /**
     * Use this method to declare that this instance requires a particular
     * dependency.
     */
    template<typename D> inline Instance<T>& requiresAll(const Type<D>& dependency, typename internal::SetterAll<T,D*>::type setter) 
    {
      requirements.push_back(new internal::RequirementAll<T,D>(dependency,setter));
      return *this;
    }

    /**
     * Calling this method instructs the context to call the postConstructMethod
     *  on the Instance after everything is initialized and wired.
     */
    inline Instance<T>& postConstruct(PostConstructMethod postConstructMethod_) throw (DependencyInjectionException)
    {
      if (postConstructMethod != NULL)
        throw DependencyInjectionException("Multiple postConstruct registrations detected for '%s'. \"There can be only one (per instance).\"",this->toString().c_str());

      postConstructMethod = postConstructMethod_;
      return *this;
    }

    /**
     * Calling this method instructs the context to call the preDestroyMethod
     *  on the Instance before everything is deleted.
     */
    inline Instance<T>& preDestroy(PreDestroyMethod preDestroyMethod_) throw (DependencyInjectionException)
    {
      if (preDestroyMethod != NULL)
        throw DependencyInjectionException("Multiple preDestroy registrations detected for '%s'. \"There can be only one (pre instance).\"",this->toString().c_str());

      preDestroyMethod = preDestroyMethod_;
      return *this;
    }

    /**
     * Returns the underlying instance.
     */
    inline T* get() { return (T*)getConcrete(); }
  };

  // Still nothing to see here, move along ...
  #include "internal/difactories.h"

  /**
   * A context defines the specific instance of a dependency injection container.
   *  There is typically one per application but this is not a requirement if there
   *  is a reason to have an application with multiple sub-eco-systems of 
   *  interrelated implementations.
   */
  class Context
  {
    std::vector<internal::InstanceBase*> instances;

    void resetInstances();

    enum Phase { initial = 0, started, stopped };
    Phase curPhase;

    friend class internal::FactoryBase;

  public:

    internal::InstanceBase* find(const internal::TypeBase& typeInfo,const char* id = NULL, bool exact = true);

    void findAll(std::vector<internal::InstanceBase*>& ret, const internal::TypeBase& typeInfo,const char* id = NULL, bool exact = true);

    virtual ~Context() { clear(); }

    inline Context() : curPhase(initial) {}

    /**
     * Use this method to declare that the context has an instance of a 
     * particular type. The instance will be created using the default 
     * constructor during the start call.
     */
    template<typename T> inline Instance<T>& hasInstance(const Type<T>& bean) 
    { 
      Instance<T>* newInstance = new Instance<T>(boost::shared_ptr<internal::FactoryBase>(new internal::Factory0<T>),bean.getId());
      instances.push_back(newInstance);
      return *newInstance;
    }

    /**
     * Use this method to declare that the context has an instance of a 
     * particular type with a given id. The instance will be created using the default 
     * constructor prior to the method returning. (It is possible this
     * may change in a future implementation so please don't write code
     * that either expects this to be the case, or expects this not to
     * be the case).
     */
    template<typename T> inline Instance<T>& hasInstance(const char* id, const Type<T>& bean)
    { 
      Instance<T>* newInstance = new Instance<T>(boost::shared_ptr<internal::FactoryBase>(new internal::Factory0<T>),id);
      instances.push_back(newInstance);
      return *newInstance;
    }


    /**
     * This template method creates an instance that uses constructor injection.
     * This form assumes that constructor of the object "T" takes one parameter.
     * The parameter is identified by P1 which can be either:
     *
     *  1) A reference to another instance using the di::Ref class template.
     *  2) A constant value using the di::Constant class template.
     *
     * Anything else passed will create a compile error.
     */
    template<typename T, typename P1> inline Instance<T>& hasInstance(const Type<T>& bean, const P1& p1)
    { 
      Instance<T>* newInstance = new Instance<T>(boost::shared_ptr<internal::FactoryBase>(new internal::Factory1<T,P1>(p1)),bean.getId());
      instances.push_back(newInstance);
      return *newInstance;
    }

    /**
     * This template method creates an instance that uses constructor injection.
     * This form assumes that constructor of the object "T" takes two parameter.
     * The parameter is identified by P? which can be either:
     *
     *  1) A reference to another instance using the di::Ref class template.
     *  2) A constant value using the di::Constant class template.
     *
     * Anything else passed will create a compile error.
     */
    template<typename T, typename P1, typename P2> inline Instance<T>& hasInstance(const Type<T>& bean, const P1& p1, const P2& p2)
    { 
      Instance<T>* newInstance = new Instance<T>(boost::shared_ptr<internal::FactoryBase>(new internal::Factory2<T,P1,P2>(p1,p2)), bean.getId());
      instances.push_back(newInstance);
      return *newInstance;
    }

    /**
     * This template method creates an instance that uses constructor injection.
     * This form assumes that constructor of the object "T" takes two parameter.
     * The parameter is identified by P? which can be either:
     *
     *  1) A reference to another instance using the di::Ref class template.
     *  2) A constant value using the di::Constant class template.
     *
     * Anything else passed will create a compile error.
     */
    template<typename T, typename P1, typename P2, typename P3> 
    inline Instance<T>& hasInstance(const Type<T>& bean, const P1& p1, const P2& p2, const P3& p3)
    { 
      Instance<T>* newInstance = new Instance<T>(boost::shared_ptr<internal::FactoryBase>(new internal::Factory3<T,P1,P2,P3>(p1,p2,p3)), bean.getId());
      instances.push_back(newInstance);
      return *newInstance;
    }

    /**
     * This template method creates an instance that uses constructor injection.
     * This form assumes that constructor of the object "T" takes two parameter.
     * The parameter is identified by P? which can be either:
     *
     *  1) A reference to another instance using the di::Ref class template.
     *  2) A constant value using the di::Constant class template.
     *
     * Anything else passed will create a compile error.
     */
    template<typename T, typename P1, typename P2, typename P3, typename P4> 
    inline Instance<T>& hasInstance(const Type<T>& bean, const P1& p1, const P2& p2, const P3& p3, const P4& p4)
    { 
      Instance<T>* newInstance = new Instance<T>(boost::shared_ptr<internal::FactoryBase>(new internal::Factory4<T,P1,P2,P3,P4>(p1,p2,p3,p4)), bean.getId());
      instances.push_back(newInstance);
      return *newInstance;
    }

    template<typename T>
    inline void staticMethodRequirement( void (*staticSetter)(T* instance) )
    {
      hasInstance(Type<internal::StaticSetterCaller<T> >(), Constant<typename internal::StaticSetterCaller<T>::StaticSetter>(staticSetter)).
        requires(Type<T>(),&internal::StaticSetterCaller<T>::set);
    }

    /**
     * This triggers the wiring and startup object lifecycle stages. Theses include,
     *  in order:
     *
     * 1) Instantiation - instantiation of the defined instances.
     * 2) Wiring - satisfying of the requirements via calling the identified setters
     * 3) Post Construction - which calls all of the postConstruct methods that
     *    were declared to the context.
     *
     * A failure to start (indicated by an exception) will automatically reset the
     * instances. Therefore it is possible that the instances constructors and 
     * destructors may have executed
     */
    void start() throw (DependencyInjectionException);

    /**
     * progress through the stop/shutdown lifecycle stages. These include,
     *   in order:
     * 
     * 1) Pre Destroy - which calls all of the preDestroy methods that
     *    were declared to the context.
     * 2) Deletion - Deletes all of the instances that were instantiated 
     *    during the Instantiation lifecycle stage.
     */
    void stop() throw (DependencyInjectionException);

    /**
     * clear() will reset the Context to it's initial state prior to any instances
     * even being added. It clears all Instances from the context, first invoking
     * stop(). Since Exceptions thrown from stop() are swallowed, it is recommended
     * that you don't use this method. Call stop explicitly and allow the Context
     * destructor to clean up the container.
     */
    void clear();

    /**
     * Allows retrieving an object by its type and Id. If there is more than
     *  one instance that is of this type, it will simply return the
     *  first one it finds in the context.
     */
    template<typename T> T* get(const Type<T>& typeToFind, const char* id = NULL) 
    { 
      internal::InstanceBase* ret = find(typeToFind,id); 
      
      return ret != NULL ? ((Instance<T>*)ret)->get() : NULL;
    }

    /**
     * Is the Context stopped. This will be true prior to start or after stop 
     * is called.
     */
    inline bool isStopped() { return curPhase == initial || curPhase == stopped; }

    /**
     * isStarted() will be true once the 'start()' call succeeds.
     */
    inline bool isStarted() { return curPhase == started; }
  };

  // Nothing to see here, move along ...
  #include "internal/diimpl.h"

}

#undef DI__DEPENDENCY_INJECTION__H
