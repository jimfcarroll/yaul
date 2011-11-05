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
 * This API supports "setter" injection only (primarily because it's easier
 * to implement in C++ than constructor injection and setter injection, while
 * some might argue is not as "pure" as constructor injection, is more flexible).
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
 *   context.hasInstance(Type<Foo>()).requires("bar1",Type<IBar>(), &Foo::setIBar);
 *   context.hasInstance(Type<Foo>()).requires("bar2",Type<IBar>(), &Foo::setIBar);
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
  #include "diinternal.h"

  /**
   * This class represents the means of declaring type information
   *  to the context.
   */
  template <class T> class Type : public internal::TypeBase
  {
  public:
    inline Type() : internal::TypeBase(typeid(T)) {}

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
  };

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

    inline Instance() : InstanceBase(Type<T>()), postConstructMethod(NULL), 
                        preDestroyMethod(NULL) { provides(Type<T>()); }

    inline explicit Instance(const char* name) : 
      InstanceBase(name,Type<T>()), postConstructMethod(NULL), 
      preDestroyMethod(NULL) { provides(Type<T>()); }

    inline Instance(const Instance<T>& other) : 
      InstanceBase(other), ref(other.ref), postConstructMethod(NULL), 
      preDestroyMethod(NULL) { provides(Type<T>()); }

    virtual ~Instance() {}

  protected:
    virtual inline const void* getConcrete() const { return ref.get(); }

    virtual inline void instantiateInstance() { ref = boost::shared_ptr<T>(new T); }

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
     * dependency.
     */
    template<typename D> inline Instance<T>& requires(const Type<D>& dependency, typename internal::Requirement<T,D>::Setter setter) 
    {
      requirements.push_back(new internal::Requirement<T,D>(this,setter));
      return *this;
    }

    /**
     * Use this method to declare that this instance requires a particular
     * dependency.
     */
    template<typename D> inline Instance<T>& requires(const char* id, const Type<D>& dependency, typename internal::Requirement<T,D>::Setter setter) 
    {
      requirements.push_back(new internal::Requirement<T,D>(id,this,setter));
      return *this;
    }

    /**
     * Use this method to declare that this instance requires a particular
     * dependency.
     */
    template<typename D> inline Instance<T>& requiresAll(const Type<D>& dependency, typename internal::Requirement<T,D>::SetterAll setter) 
    {
      requirements.push_back(new internal::Requirement<T,D>(this,setter));
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

  /**
   * A context defines the specific instance of a dependency injection container.
   *  There is typically one per application but this is not a requirement if there
   *  is a reason to have an application with multiple sub-eco-systems of 
   *  interrelated implementations.
   */
  class Context
  {
    std::vector<internal::InstanceBase*> instances;

    internal::InstanceBase* find(const std::type_info& typeInfo);
    internal::InstanceBase* find(const char* id, const std::type_info& typeInfo);

    void resetInstances();

    enum Phase { initial = 0, started, stopped };
    Phase curPhase;

  public:

    virtual ~Context() { clear(); }

    inline Context() : curPhase(initial) {}

    /**
     * Use this method to declare that the context has an instance of a 
     * particular type. The instance will be created using the default 
     * constructor prior to the method returning. (It is possible this
     * may change in a future implementation so please don't write code
     * that either expects this to be the case, or expects this not to
     * be the case).
     */
    template<typename T> inline Instance<T>& hasInstance(const Type<T>& bean) 
    { 
      Instance<T>* newInstance = new Instance<T>();
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
      Instance<T>* newInstance = new Instance<T>(id);
      instances.push_back(newInstance);
      return *newInstance;
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
     * Allows retrieving an object by its type. If there is more than
     *  one instance that is of this type, it will simply return the
     *  first one it finds in the context.
     */
    template<typename T> T* get(Type<T> typeToFind) 
    { 
      internal::InstanceBase* ret = find(typeToFind.getTypeInfo()); 
      
      return ret != NULL ? ((Instance<T>*)ret)->get() : NULL;
    }

    /**
     * Allows retrieving an object by its type and Id. If there is more than
     *  one instance that is of this type, it will simply return the
     *  first one it finds in the context.
     */
    template<typename T> T* get(const char* id, Type<T> typeToFind) 
    { 
      internal::InstanceBase* ret = find(id,typeToFind.getTypeInfo()); 
      
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

}

#undef DI__DEPENDENCY_INJECTION__H