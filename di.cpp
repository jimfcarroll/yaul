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

    bool InstanceBase::canConvertTo(const internal::TypeBase& typeToConvertTo) const
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

  internal::InstanceBase* Context::find(const internal::TypeBase& typeInfo, const char* id, bool exact)
  {
    for(std::vector<internal::InstanceBase*>::iterator it = instances.begin(); it != instances.end(); it++)
    {
      internal::InstanceBase* instance = (*it);

      if (((exact && instance->type.getTypeInfo() == typeInfo.getTypeInfo()) ||
           (!exact && instance->canConvertTo(typeInfo))) && 
          (id == NULL || instance->id == id))
        return instance;
    }
    return NULL;
  }

  void Context::findAll(std::vector<internal::InstanceBase*>& ret, const internal::TypeBase& typeInfo, const char* id, bool exact)
  {
    for(std::vector<internal::InstanceBase*>::iterator it = instances.begin(); it != instances.end(); it++)
    {
      internal::InstanceBase* instance = (*it);

      if (((exact && instance->type.getTypeInfo() == typeInfo.getTypeInfo()) ||
           (!exact && instance->canConvertTo(typeInfo))) && 
          (id == NULL || instance->id == id))
        ret.push_back(instance);
    }
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
      catch (DependencyInjectionException& die) { throw die; }
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
        catch (DependencyInjectionException& die) { throw die; }
        catch (...)
        {
          // hum .... what to do? c++ sucks here in that I cannot get a handle to the 
          // original exception. ... I can either log and rethrow or I can throw another
          // known exception. I wish I could wrap and throw.
          resetInstances();
          throw DependencyInjectionException("Unknown exception intercepted while executing PreDestroy phase on \"%s.\"", instance->toString().c_str());
        }
      }

    }

    // clear out the instances.
    resetInstances();
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
    // This loop is stupid. Instead the order of instantiation ought to be determined.
    //  but for now this works.
    internal::InstanceBase* instance;
    try
    {
      std::vector<internal::InstanceBase*> workingList;
      workingList = instances;

      while (workingList.size() > 0)
      {
        unsigned int preCount = workingList.size();
        internal::InstanceBase* firstNotInstantiated = NULL;

        std::vector<internal::InstanceBase*> tmpvector;

        for(std::vector<internal::InstanceBase*>::iterator it = workingList.begin(); it != workingList.end(); it++)
        {
          instance = (*it);
          if (instance->factory->dependenciesSatisfied(this))
            instance->instantiateInstance(this);
          else if (firstNotInstantiated == NULL)
          {
            firstNotInstantiated = instance;
            tmpvector.push_back(instance);
          }
        }

        workingList = tmpvector;

        if (preCount == workingList.size() && workingList.size() > 0)
          throw DependencyInjectionException("Cannot resolve constructor dependencies for \"%s\"", firstNotInstantiated->toString().c_str());
      }
    }
    catch (DependencyInjectionException& die) { throw die; }
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
        requirement->satisfy(instance,this);
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
      catch (DependencyInjectionException& die) { throw die; }
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
