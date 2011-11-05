/*
 * Copyright (C) 2011
 */

#pragma once

// This file should NEVER be included independently. It is part of the internals of
//   the di.h file and simply separated 
#ifndef DI__DEPENDENCY_INJECTION__H
#error "Please don't include \"diinternal.h\" directly."
#endif

/**
 * Things in the di::internal namespace are not meant as part of the public
 *  API. If you're interested in learning the API, see the documenation on the
 *  non-internal API classes in "di.h".
 */
class Context;
template<class T> class Instance;

namespace internal
{
  class RequirementBase;
  class InstanceBase;

  /**
   * holds simple rtti type information. Defines equivalence and toString
   */
  class TypeBase
  {
    friend class InstanceBase;
    friend class RequirementBase;

  protected:
    const std::type_info* type;

    inline TypeBase(const std::type_info& type_) : type(&type_) 
    { 
#ifdef DI__DEPENDENCY_INJECTION_DEBUG
      std::cout << "Creating type:" << type_.name() << std::endl; 
#endif
    }

    inline TypeBase(const TypeBase& other) : type(other.type) {}
    inline TypeBase& operator=(const TypeBase& other) { type=other.type; return *this; }

  public:

    inline bool operator==(const TypeBase& other) const { return (*type) == (*(other.type)); }

    inline bool operator!=(const TypeBase& other) const { return (*type) != (*(other.type)); }

    inline const std::string toString() const { return type->name(); }

    inline const std::type_info& getTypeInfo() const { return (*type); }
  };

  /**
   * base class for template that defines a type conversion between two 
   *  types in a class hierarchy.
   */
  class TypeConverterBase : public TypeBase
  {
    friend class InstanceBase;
  protected:
    inline explicit TypeConverterBase(const std::type_info& type) : TypeBase(type) {}

    virtual void* doConvert(void*) = 0;

    inline bool isTypeToConvertTo(const TypeBase& to) { return (*this) == to; }
  };

  /**
   * Template to allow the instantiation of a particular type conversion
   */
  template<class T, class F> class TypeConverter : public TypeConverterBase
  {
  protected:

    virtual inline void* doConvert(void * from) { return dynamic_cast<T*>((F*)from); }
    inline T* convert(F* from) { return (T*)doConvert(from); }

  public:
    // this needs to be public for the "<class D> Instance<T>::provides" method
    inline TypeConverter() : TypeConverterBase(typeid(T)) {}
  };

  /**
   * Base class for an instance declaration in the DI context
   */
  class InstanceBase
  {
    friend class di::Context;
    friend class RequirementBase;

  protected:

    TypeBase type;
    std::string id;
    bool hasId;

    std::vector<TypeConverterBase*> providesTheseTypes;
    std::vector<RequirementBase*> requirements;

    virtual void doPostConstruct() = 0;
    virtual void doPreDestroy() = 0;

    inline explicit InstanceBase(const TypeBase& tb) : type(tb), hasId(false) {}

    inline InstanceBase(const char* name, const TypeBase& tb) : type(tb), id(name), hasId(true) {}

    inline InstanceBase(const InstanceBase& other) : type(other.type), id(other.id), hasId(other.hasId),
                                                     providesTheseTypes(other.providesTheseTypes), 
                                                     requirements(other.requirements){ }

    inline virtual ~InstanceBase() { }

    inline std::vector<RequirementBase*>& getRequirements() { return requirements; }

    inline const std::string toString() const { return hasId ? (id + ":" + type.toString()) : type.toString(); }

    void* convertTo(const TypeBase& typeToConvertTo) const throw (DependencyInjectionException);

    bool canConvertTo(const TypeBase& other) const;

    virtual const void* getConcrete() const = 0;

    virtual void instantiateInstance() = 0;

    virtual void reset() = 0;
  };

  /**
   * base class for the template that defines a requirement.
   */
  class RequirementBase
  {
    friend class di::Context;

  protected:
    TypeBase instance;
    TypeBase parameter;

    std::string requiredId;
    bool specifiesIdp;

    inline RequirementBase(const std::type_info& obj, const std::type_info& param) : 
      instance(obj), parameter(param), specifiesIdp(false) {}
    inline RequirementBase(const char* id, const std::type_info& obj, const std::type_info& param) : 
      instance(obj), parameter(param), requiredId(id), specifiesIdp(id != NULL) {}

    virtual ~RequirementBase() {}

    inline bool specifiesId() const { return specifiesIdp; }

    inline bool isSatisfiedBy(const InstanceBase* dep)
    {
      return specifiesIdp ? (dep->hasId ? (requiredId == dep->id && dep->canConvertTo(parameter)) : false) : dep->canConvertTo(parameter);
    }

    virtual const std::string toString() const;

    virtual void satisfyWith(InstanceBase* dependency) throw (DependencyInjectionException) = 0;
    virtual bool satisfiedWithSet() = 0;
    virtual void satisfyWithSet(const std::vector<InstanceBase*>& dep) throw (DependencyInjectionException) = 0;

    // hacks for child to call other stuff
    inline void* instanceBaseConvertTo(InstanceBase* dep, const TypeBase& typeToConvertTo) const 
      throw (DependencyInjectionException) { return dep->convertTo(typeToConvertTo); }

    inline const std::string toString(const InstanceBase& dep) const { return dep.toString(); }

    inline const void* getConcrete(InstanceBase* instance) const { return instance->getConcrete(); }
  };

  /**
   * This template is used to declare a requirement of an object that
   *  needs to be fulfilled in the container.
   */
  template<class T, class D> class Requirement : public internal::RequirementBase
  {
    friend class di::Instance<T>;
  public:
    typedef void (T::*Setter)(D*);
    typedef void (T::*SetterAll)(const std::vector<D*>);

  private:
    Setter setter;
    SetterAll setterAll;
    InstanceBase* requirementOf;

    inline Requirement(InstanceBase* thingWithSetter, Setter func) : 
      RequirementBase(typeid(T), typeid(D)), setter(func), 
      setterAll(NULL), requirementOf(thingWithSetter) {}
    inline Requirement(InstanceBase* thingWithSetter, SetterAll func) : 
      RequirementBase(typeid(T), typeid(D)), setter(NULL), 
      setterAll(func), requirementOf(thingWithSetter) {}
    inline Requirement(const char* id, InstanceBase* thingWithSetter, Setter func) : 
      RequirementBase(id,typeid(T), typeid(D)), setter(func), setterAll(NULL),
      requirementOf(thingWithSetter) {}

    inline T* getRequirementOf() { return (T*)getConcrete(requirementOf); }

    inline void injectRequirement(D* objectToInject) { ((*getRequirementOf()).*(setter))(objectToInject);  }
    inline void injectRequirementSet(const std::vector<D*>& objectToInject) { ((*getRequirementOf()).*(setterAll))(objectToInject);  }

    inline virtual void satisfyWith(InstanceBase* dep) throw (DependencyInjectionException)
    {
#ifdef DI__DEPENDENCY_INJECTION_DEBUG
      std::cout << "requirement:" << toString() << " is satisfied by " << toString(*dep) << std::endl;
#endif
      D* actualDep = (D*)instanceBaseConvertTo(dep,parameter);
      if (actualDep == NULL)
        throw DependencyInjectionException("Can't satisfy a requirement for '%s' with '%s.'",toString().c_str(),toString(*dep).c_str());
      injectRequirement(actualDep);
    }

    inline virtual bool satisfiedWithSet() { return setterAll != NULL; }

    inline virtual void satisfyWithSet(const std::vector<InstanceBase*>& dep) throw (DependencyInjectionException)
    {
#ifdef DI__DEPENDENCY_INJECTION_DEBUG
      std::cout << "requirement:" << toString() << " is satisfied by: " << std::endl;
      for (std::vector<InstanceBase*>::const_iterator depIt = dep.begin(); depIt != dep.end(); depIt++)
        std::cout << "    " << toString(*(*depIt)) << std::endl;
#endif
      std::vector<D*> param;
      for (std::vector<InstanceBase*>::const_iterator depIt = dep.begin(); depIt != dep.end(); depIt++)
        param.push_back((D*)instanceBaseConvertTo((*depIt),parameter));
      injectRequirementSet(param);
    }
  };

}


