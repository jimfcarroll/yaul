/*
 * Copyright (C) 2011-2012
 */

#pragma once

// This file should NEVER be included independently. It is part of the internals of
//   the di.h file and simply separated 
#ifndef DI__DEPENDENCY_INJECTION__H
 "Please don't include \"difactories.h\" directly."
#endif

namespace internal
{

  //=======================================================================
  // More internals
  //=======================================================================

  /**
   * Factory to create an instance of an M with a default constructor
   */
  template<typename M> class Factory0 : public internal::FactoryBase
  {
  protected:
  public:
    inline Factory0() { }

    inline virtual bool dependenciesSatisfied(Context* context) { return true; }

    inline virtual void* create(Context* context) throw (DependencyInjectionException) { return new M; }
  };

  /**
   * Factory to create an instance of an M with a constructor that takes
   *  one parameter.
   */
  template<typename M, typename T1> class Factory1 : public internal::FactoryBase
  {
  protected:
    T1 p1;

  public:
    inline Factory1(const T1& pp1) : p1(pp1) {}

    inline virtual bool dependenciesSatisfied(Context* context) { return p1.available(context); }

    inline virtual void* create(Context* context) throw (DependencyInjectionException)
    {
      typename T1::type cp1 = p1.find(context);
      
      return new M(cp1);
    }
  };

  /**
   * Factory to create an instance of an M with a constructor that takes
   *  two parameter.
   */
  template<typename M, typename T1, typename T2> class Factory2 : public internal::FactoryBase
  {
  protected:
    T1 p1;
    T2 p2;

  public:
    inline Factory2(const T1& pp1, const T2& pp2) : p1(pp1), p2(pp2) {}

    inline virtual bool dependenciesSatisfied(Context* context) 
    { 
      return p1.available(context) && p2.available(context);
    }

    inline virtual void* create(Context* context) throw (DependencyInjectionException)
    {
      typename T1::type cp1 = p1.find(context);
      typename T2::type cp2 = p2.find(context);
      
      return new M(cp1,cp2);
    }
  };

  /**
   * Factory to create an instance of an M with a constructor that takes
   *  three parameter.
   */
  template<typename M, typename T1, typename T2, typename T3> class Factory3 : public internal::FactoryBase
  {
  protected:
    T1 p1;
    T2 p2;
    T3 p3;

  public:
    inline Factory3(const T1& pp1, const T2& pp2, const T3& pp3) : p1(pp1), p2(pp2), p3(pp3) {}

    inline virtual bool dependenciesSatisfied(Context* context) 
    { 
      return p1.available(context) && p2.available(context) && p3.available(context); 
    }

    inline virtual void* create(Context* context) throw (DependencyInjectionException)
    {
      typename T1::type cp1 = p1.find(context);
      typename T2::type cp2 = p2.find(context);
      typename T3::type cp3 = p3.find(context);
      
      return new M(cp1,cp2,cp3);
    }
  };
  //=======================================================================
}
