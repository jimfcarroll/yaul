/*
 * Copyright (C) 2011
 */

#include "di.h"

#include "StdString.h"

#include <map>

namespace di
{
  namespace internal
  {
    const std::string RequirementBase::toString() const
    {
      CStdString msg;
      if (specifiesIdp)
        msg.Format("(%s requires %s with id %s)",instance.getTypeInfo().name(), parameter.getTypeInfo().name(), requiredId.c_str());
      else
        msg.Format("(%s requires %s)",instance.getTypeInfo().name(), parameter.getTypeInfo().name());
      return msg;
    }

    void* InstanceBase::convertTo(const TypeBase& typeToConvertTo) const throw (DependencyInjectionException)
    {
      const void* obj = getConcrete();
      for(std::vector<TypeConverterBase*>::const_iterator it = providesTheseTypes.begin(); it != providesTheseTypes.end(); it++)
      {
        TypeConverterBase* typeConverter = (*it);

        if (typeConverter->isTypeToConvertTo(typeToConvertTo))
        {
          void* ret = ((TypeConverterBase*)typeConverter)->doConvert((void*)obj);
          if (ret == NULL)
            throw DependencyInjectionException("Failed to convert a \"%s\" to a \"%s\" using a dynamic_cast for ", typeConverter->toString().c_str(), type.getTypeInfo().name());
          return ret;
        }
      }

      return NULL;
    }

    bool InstanceBase::canConvertTo(const TypeBase& typeToConvertTo) const
    {
      for(std::vector<TypeConverterBase*>::const_iterator it = providesTheseTypes.begin(); it != providesTheseTypes.end(); it++)
      {
        TypeConverterBase* typeConverter = (*it);

        if (typeConverter->isTypeToConvertTo(typeToConvertTo))
          return true;
      }

      return false;
    }

  }

  internal::InstanceBase* Context::find(const std::type_info& typeInfo)
  {
    for(std::vector<internal::InstanceBase*>::iterator it = instances.begin(); it != instances.end(); it++)
    {
      internal::InstanceBase* instance = (*it);

      if (instance->type.getTypeInfo() == typeInfo)
        return instance;
    }
    return NULL;
  }

  internal::InstanceBase* Context::find(const char* id, const std::type_info& typeInfo)
  {
    for(std::vector<internal::InstanceBase*>::iterator it = instances.begin(); it != instances.end(); it++)
    {
      internal::InstanceBase* instance = (*it);

      if (instance->type.getTypeInfo() == typeInfo && instance->id == id)
        return instance;
    }
    return NULL;
  }

  void Context::resetInstances()
  {
    internal::InstanceBase* instance;
    for(std::vector<internal::InstanceBase*>::iterator it = instances.begin(); it != instances.end(); it++)
    {
      // since this results in the instance destructor being called ... in case some moron 
      // throws from the destructor, we don't want to stop deleting.
      try 
      { 
        instance = (*it);
        instance->reset();
      }
      catch (...) 
      { 
        // this prints a message to the log as long as there is a logger set in the exception
        DependencyInjectionException ex("Exception detected in the destructor of the instance for \"%s.\"",instance->toString().c_str());
      }
    }

    curPhase = stopped;
  }

  void Context::stop() throw (DependencyInjectionException)
  {
    if (! isStopped())
    {
      internal::InstanceBase* instance;
      // pre destroy step
      for(std::vector<internal::InstanceBase*>::iterator it = instances.begin(); it != instances.end(); it++)
      {
        instance = (*it);
        try
        {
          instance->doPreDestroy();
        }
        catch (...)
        {
          // hum .... what to do? c++ sucks here in that I cannot get a handle to the 
          // original exception. ... I can either log and rethrow or I can throw another
          // known exception. I wish I could wrap and throw.
          resetInstances();
          throw DependencyInjectionException("Unknown exception intercepted while executing PreDestroy phase on \"%s.\"", instance->toString().c_str());
        }
      }

      // clear out the instances.
      resetInstances();
    }
  }

  void Context::clear()
  {
    try { stop(); } catch (DependencyInjectionException& ex) {}
    for(std::vector<internal::InstanceBase*>::iterator it = instances.begin(); it != instances.end(); it++)
      delete (*it);
    instances.clear();
    curPhase = initial;
  }

  void Context::start() throw (DependencyInjectionException)
  {
    if (isStarted())
      throw DependencyInjectionException("Called start for a second time on a di::Context.");

    // First instantiate
    internal::InstanceBase* instance;
    try
    {
      for(std::vector<internal::InstanceBase*>::iterator it = instances.begin(); it != instances.end(); it++)
      {
        instance = (*it);
        instance->instantiateInstance();
      }
    }
    catch (...)
    {
      // hum .... what to do? c++ sucks here in that I cannot get a handle to the 
      // original exception. ... I can either log and rethrow or I can throw another
      // known exception. I wish I could wrap and throw.
      resetInstances();
      throw DependencyInjectionException("Unknown exception intercepted while instantiating \"%s.\"", instance->toString().c_str());
    }

    // we need a map of Types to the Instances that provide those types
    for(std::vector<internal::InstanceBase*>::iterator it = instances.begin(); it != instances.end(); it++)
    {
      instance = (*it);

      // find out what it requires.
      std::vector<internal::RequirementBase*>& requirements = instance->getRequirements();
      for (std::vector<internal::RequirementBase*>::iterator rit = requirements.begin(); rit != requirements.end(); rit++)
      {
        internal::RequirementBase* requirement = (*rit);

        // now go through all of the instances and see which one satisfies 
        //  the requirements. Make sure that no more than one does.
        std::vector<internal::InstanceBase*> satisfies;

        for(std::vector<internal::InstanceBase*>::iterator it2 = instances.begin(); it2 != instances.end(); it2++)
        {
          internal::InstanceBase* comp = (*it2);

          if (requirement->isSatisfiedBy(comp))
            satisfies.push_back(comp);
        }

        if (satisfies.size() < 1)
        {
          resetInstances();
          throw DependencyInjectionException("Cannot satisfy the requirement of \"%s\" which requires \"%s\".", instance->toString().c_str(), requirement->toString().c_str());
        }
        else if (!requirement->satisfiedWithSet() && satisfies.size() > 1)
        {
          resetInstances();
          throw DependencyInjectionException("Ambiguous requirement of \"%s\" for \"%s\".", instance->toString().c_str(), requirement->toString().c_str());
        }

        // ok then, we need to plug in the dependency
        if (requirement->satisfiedWithSet())
          requirement->satisfyWithSet(satisfies);
        else
        {
          internal::InstanceBase* dep = satisfies.front();
          requirement->satisfyWith(dep);
        }
      }
    }

    // post construct step
    for(std::vector<internal::InstanceBase*>::iterator it = instances.begin(); it != instances.end(); it++)
    {
      instance = (*it);
      try
      {
        instance->doPostConstruct();
      }
      catch (...)
      {
        // hum .... what to do? c++ sucks here in that I cannot get a handle to the 
        // original exception. ... I can either log and rethrow or I can throw another
        // known exception. I wish I could wrap and throw.
        resetInstances();
        throw DependencyInjectionException("Unknown exception intercepted while executing postConstruct phase on \"%s.\"", instance->toString().c_str());
      }
    }

    curPhase = started;
  }
}
