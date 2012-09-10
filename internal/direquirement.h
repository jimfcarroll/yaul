/*
 * Copyright (C) 2011
 */

#pragma once

// This file should NEVER be included independently. It is part of the internals of
//   the di.h file and simply separated 
#ifndef DI__DEPENDENCY_INJECTION__H
#error "Please don't include \"direquirement.h\" directly."
#endif

namespace internal
{
  template<class T, class D> class Requirement : public internal::RequirementBase
  {
    friend class di::Instance<T>;

    typename Setter<T,D*>::type setter;
    Type<D> parameter;

    inline Requirement(const Type<D>& ty, typename Setter<T,D*>::type func) : setter(func), parameter(ty) {}
  protected:
    inline virtual void satisfy(InstanceBase* instance, Context* context) throw (DependencyInjectionException);
  };

  template<class T, class D> class RequirementAll : public internal::RequirementBase
  {
    friend class di::Instance<T>;

    typename SetterAll<T,D*>::type setter;
    Type<D> parameter;

    inline RequirementAll(const Type<D>& ty, typename SetterAll<T,D*>::type func) : setter(func), parameter(ty) {}
  protected:
    inline virtual void satisfy(InstanceBase* instance, Context* context) throw (DependencyInjectionException);
  };
}

