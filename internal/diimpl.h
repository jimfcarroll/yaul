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
  template<class T, class D> inline void Requirement<T,D>::satisfy(InstanceBase* instance, Context* context) throw (DependencyInjectionException)
  {
#ifdef DI__DEPENDENCY_INJECTION_DEBUG
    std::cout << "requirement:" << toString() << " is satisfied by " << dep->toString() << std::endl;
#endif
    std::vector<InstanceBase*> satisfiedBy;
    context->findAll(satisfiedBy,parameter,parameter.getId(),false);
    if (satisfiedBy.size() == 0)
      throw DependencyInjectionException("Cannot satisfy the requirement of \"%s\" which requires \"%s\".", instance->toString().c_str(), parameter.toString().c_str());
    if (satisfiedBy.size() > 1)
      throw DependencyInjectionException("Ambiguous requirement of \"%s\" for \"%s\".", instance->toString().c_str(), parameter.toString().c_str());
    InstanceBase* dep = satisfiedBy.front();
    (((T*)instance->getConcrete())->*(setter)) ((D*)dep->getConcrete());
  }

  template<class T, class D> inline void RequirementAll<T,D>::satisfy(InstanceBase* instance, Context* context) throw (DependencyInjectionException)
  {
#ifdef DI__DEPENDENCY_INJECTION_DEBUG
    std::cout << "requirement:" << toString() << " is satisfied by " << dep->toString() << std::endl;
#endif
    std::vector<InstanceBase*> satisfiedBy;
    context->findAll(satisfiedBy,parameter,parameter.getId(),false);
    if (satisfiedBy.size() == 0)
      throw DependencyInjectionException("Cannot satisfy the requirement of \"%s\" which requires \"%s\".", instance->toString().c_str(), parameter.toString().c_str());
    std::vector<D*> instances;
    for(std::vector<internal::InstanceBase*>::iterator it = satisfiedBy.begin(); it != satisfiedBy.end(); it++)
      instances.push_back((D*)(*it)->getConcrete());
    (((T*)instance->getConcrete())->*(setter)) (instances);
  }
}

template<typename T> inline T* Type<T>::findProvides(Context* context) const throw (DependencyInjectionException)
{
  internal::InstanceBase* inst = context->find(*this,objId,false);
  return inst ? (type)(inst->convertTo(*this)) : NULL;
}

template<typename T> inline bool Type<T>::available(Context* context) const 
{ 
  internal::InstanceBase* inst = context->find(*this,objId,false);
  return inst != NULL && inst->instantiated();
}

