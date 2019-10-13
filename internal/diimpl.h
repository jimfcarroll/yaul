/*
 * Copyright (C) 2011
 */

#pragma once

// This file should NEVER be included independently. It is part of the internals of
//   the di.h file and simply separated 
#ifndef DI__DEPENDENCY_INJECTION__H
#error "Please don't include \"diimpl.h\" directly."
#endif

namespace internal
{
  template<class T, class D, class RDT> inline void Requirement<T,D,RDT>::satisfy(BeanBase* instance, Context* context) /* throw (DependencyInjectionException) */
  {
#ifdef DI__DEPENDENCY_INJECTION_DEBUG
    std::cout << "requirement:" << toString() << " is satisfied by " << dep->toString() << std::endl;
#endif
    std::vector<BeanBase*> satisfiedBy;
    parameter.findAll(satisfiedBy,context,false);
    if (satisfiedBy.size() == 0)
      throw DependencyInjectionException("Cannot satisfy the requirement of \"%s\" which requires \"%s\".", instance->toString().c_str(), parameter.toString().c_str());
    if (satisfiedBy.size() > 1)
      throw DependencyInjectionException("Ambiguous requirement of \"%s\" for \"%s\".", instance->toString().c_str(), parameter.toString().c_str());
    BeanBase* dep = satisfiedBy.front();
    (((T*)instance->getConcrete())->*(setter)) ((RDT)dep->getConcrete());
  }

  template<class T, class D, class RDT> inline void RequirementConstant<T,D,RDT>::satisfy(BeanBase* instance, Context* context) /* throw (DependencyInjectionException) */
  {
#ifdef DI__DEPENDENCY_INJECTION_DEBUG
    std::cout << "requirement:" << toString() << " is satisfied by " << dep->toString() << std::endl;
#endif
    (((T*)instance->getConcrete())->*(setter)) (parameter.findIsAlso(context));
  }

  template<class T, class D, class RDT> inline void RequirementAll<T,D,RDT>::satisfy(BeanBase* instance, Context* context) /* throw (DependencyInjectionException) */
  {
#ifdef DI__DEPENDENCY_INJECTION_DEBUG
    std::cout << "requirement:" << toString() << " is satisfied by " << dep->toString() << std::endl;
#endif
    std::vector<BeanBase*> satisfiedBy;
    parameter.findAll(satisfiedBy,context,false);
    if (satisfiedBy.size() == 0)
      throw DependencyInjectionException("Cannot satisfy the requirement of \"%s\" which requires \"%s\".", instance->toString().c_str(), parameter.toString().c_str());
    std::vector<RDT> instances;
    for(std::vector<internal::BeanBase*>::iterator it = satisfiedBy.begin(); it != satisfiedBy.end(); it++)
      instances.push_back((RDT)(*it)->getConcrete());
    (((T*)instance->getConcrete())->*(setter)) (instances);
  }
}

template<typename T> void Instance<T>::findAll(std::vector<internal::BeanBase*>& ret, Context* context, bool exact) const /* throw (DependencyInjectionException) */
{
  context->findAll(ret,*this,this->getId(),exact);
}

template<typename T> inline T* Instance<T>::findIsAlso(Context* context) const /* throw (DependencyInjectionException) */
{
  internal::BeanBase* inst = context->find(*this,objId,false);
  return inst ? (type)(inst->convertTo(*this)) : NULL;
}

template<typename T> inline bool Instance<T>::available(Context* context) const 
{ 
  internal::BeanBase* inst = context->find(*this,objId,false);
  return inst != NULL && inst->instantiated();
}

