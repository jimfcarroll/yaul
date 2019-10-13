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
  template<class T, class D, class RDT> class Requirement : public internal::RequirementBase
  {
    friend class di::Bean<T>;

    typename Setter<T,RDT>::type setter;
    D parameter;

    inline Requirement(const D& ty, typename Setter<T,RDT>::type func) : setter(func), parameter(ty) {}
  protected:
    inline virtual void satisfy(BeanBase* instance, Context* context) /* throw (DependencyInjectionException) */;
  };

  template<class T, class D, class RDT> class RequirementConstant : public internal::RequirementBase
  {
    friend class di::Bean<T>;

    typename Setter<T,RDT>::type setter;
    D parameter;

    inline RequirementConstant(const D& ty, typename Setter<T,RDT>::type func) : setter(func), parameter(ty) {}
  protected:
    inline virtual void satisfy(BeanBase* instance, Context* context) /* throw (DependencyInjectionException) */;
  };

  
  template<class T, class D, class RDT> class RequirementAll : public internal::RequirementBase
  {
    friend class di::Bean<T>;

    typename SetterAll<T,RDT>::type setter;
    D parameter;

    inline RequirementAll(const D& ty, typename SetterAll<T,RDT>::type func) : setter(func), parameter(ty) {}
  protected:
    inline virtual void satisfy(BeanBase* instance, Context* context) /* throw (DependencyInjectionException) */;
  };
}

