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
template<class T> class Bean;

namespace internal
{
  // This class is used to prevent copying ... it has no friends (awwww!)
  class NoCopy { inline NoCopy(const NoCopy& o) {} public: inline NoCopy() {} };

  class RequirementBase;
  class BeanBase;
  class FactoryBase;

  /**
   * holds simple rtti type information. Defines equivalence and toString
   */
  class InstanceBase
  {
    friend class BeanBase;
    friend class RequirementBase;
    friend class FactoryBase;

  protected:
    const char* objId;
    const std::type_info* type;

    inline InstanceBase(const std::type_info& type_) : objId(NULL), type(&type_) 
    { 
#ifdef DI__DEPENDENCY_INJECTION_DEBUG
      std::cout << "Creating type:" << type_.name() << std::endl; 
#endif
    }

    inline InstanceBase(const char* id, const std::type_info& type_) : objId(id), type(&type_) 
    { 
#ifdef DI__DEPENDENCY_INJECTION_DEBUG
      std::cout << "Creating type:" << type_.name() << std::endl; 
#endif
    }

    inline InstanceBase(const InstanceBase& other) : objId(other.objId), type(other.type) {}
    inline InstanceBase& operator=(const InstanceBase& other) { objId = other.objId; type=other.type; return *this; }

  public:

    inline bool sameInstance(const InstanceBase& other) const { return (*type) == (*(other.type)); }
    inline const std::string toString() const { return type->name(); }
    inline const std::type_info& getInstanceInfo() const { return (*type); }
    inline const char* getId() const { return objId; }
  };

  /**
   * base class for template that defines a type conversion between two 
   *  types in a class hierarchy.
   */
  class InstanceConverterBase : public InstanceBase
  {
    friend class BeanBase;
  protected:
    inline explicit InstanceConverterBase(const std::type_info& type) : InstanceBase(type) {}
    virtual void* doConvert(void*) = 0;
    inline bool isInstanceToConvertTo(const InstanceBase& to) { return (*this).sameInstance(to); }
  };

  /**
   * Template to allow the instantiation of a particular type conversion
   */
  template<class T, class F> class InstanceConverter : public InstanceConverterBase
  {
  protected:

    virtual inline void* doConvert(void * from) { return dynamic_cast<T*>((F*)from); }
    inline T* convert(F* from) { return (T*)doConvert(from); }

  public:
    // this needs to be public for the "<class D> Bean<T>::isAlso" method
    inline InstanceConverter() : InstanceConverterBase(typeid(T)) {}
  };

  class FactoryBase
  {
  private:
  protected:

  public:
    virtual inline ~FactoryBase() {}

    virtual void* create(Context* context) /*throw (DependencyInjectionException) */ = 0;

    virtual bool dependenciesSatisfied(Context* context) = 0;
  };

  /**
   * Base class for an instance declaration in the DI context
   */
  class BeanBase : public NoCopy
  {
    friend class di::Context;
    friend class RequirementBase;
    friend class FactoryBase;

  protected:

    InstanceBase type;
    std::string id;
    bool hasId;

    std::vector<InstanceConverterBase*> isAlsoTheseInstances;
    std::vector<RequirementBase*> requirements;
    internal::FactoryBase* factory;
    bool hasBean;

    virtual void doPostConstruct() = 0;
    virtual void doPreDestroy() = 0;

    inline BeanBase(FactoryBase* f, const char* name, const InstanceBase& tb) : 
      type(tb), hasId(false), factory(f), hasBean(false) { if (name) { id = name; hasId = true; } }

    inline virtual ~BeanBase() { if (factory) delete factory; }

    inline std::vector<RequirementBase*>& getRequirements() { return requirements; }

    inline void setFactory(FactoryBase* newFactory) { if (factory) delete factory; factory = newFactory; }

    bool canConvertTo(const InstanceBase& other) const;

    virtual void instantiateBean(di::Context*) = 0;

    virtual void reset() = 0;
  public:
    virtual const void* getConcrete() const = 0;

    void* convertTo(const InstanceBase& typeToConvertTo) const /* throw (DependencyInjectionException) */;

    inline bool instantiated() { return hasBean; }

    inline const std::string toString() const { return hasId ? (id + ":" + type.toString()) : type.toString(); }
  };

  /**
   * base class for the template that defines a requirement.
   */
  class RequirementBase
  {
    friend class di::Context;

  protected:
    inline RequirementBase() {  }
    virtual ~RequirementBase() {}

    virtual void satisfy(BeanBase* instance, Context* context) /* throw (DependencyInjectionException) */ = 0;
  };

  template<class T, class D> struct Setter
  {
    typedef void (T::*type)(D);
  };

  template<class T, class D> struct SetterAll
  {
    typedef void (T::*type)(const std::vector<D>);
  };

}

