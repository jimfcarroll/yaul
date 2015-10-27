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
  // This class is used to prevent copying ... it has no friends (awwww!)
  class NoCopy { inline NoCopy(const NoCopy& o) {} public: inline NoCopy() {} };

  class RequirementBase;
  class InstanceBase;
  class FactoryBase;

  /**
   * holds simple rtti type information. Defines equivalence and toString
   */
  class TypeBase
  {
    friend class InstanceBase;
    friend class RequirementBase;
    friend class FactoryBase;

  protected:
    const char* objId;
    const std::type_info* type;

    inline TypeBase(const std::type_info& type_) : objId(NULL), type(&type_) 
    { 
#ifdef DI__DEPENDENCY_INJECTION_DEBUG
      std::cout << "Creating type:" << type_.name() << std::endl; 
#endif
    }

    inline TypeBase(const char* id, const std::type_info& type_) : objId(id), type(&type_) 
    { 
#ifdef DI__DEPENDENCY_INJECTION_DEBUG
      std::cout << "Creating type:" << type_.name() << std::endl; 
#endif
    }

    inline TypeBase(const TypeBase& other) : objId(other.objId), type(other.type) {}
    inline TypeBase& operator=(const TypeBase& other) { objId = other.objId; type=other.type; return *this; }

  public:

    inline bool sameType(const TypeBase& other) const { return (*type) == (*(other.type)); }
    inline const std::string toString() const { return type->name(); }
    inline const std::type_info& getTypeInfo() const { return (*type); }
    inline const char* getId() const { return objId; }
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
    inline bool isTypeToConvertTo(const TypeBase& to) { return (*this).sameType(to); }
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

  class FactoryBase
  {
  private:
  protected:

  public:

    virtual void* create(Context* context) throw (DependencyInjectionException) = 0;

    virtual bool dependenciesSatisfied(Context* context) = 0;
  };

  /**
   * Base class for an instance declaration in the DI context
   */
  class InstanceBase : public NoCopy
  {
    friend class di::Context;
    friend class RequirementBase;
    friend class FactoryBase;

  protected:

    TypeBase type;
    std::string id;
    bool hasId;

    std::vector<TypeConverterBase*> providesTheseTypes;
    std::vector<RequirementBase*> requirements;
    internal::FactoryBase* factory;
    bool hasInstance;

    virtual void doPostConstruct() = 0;
    virtual void doPreDestroy() = 0;

    inline InstanceBase(FactoryBase* f, const char* name, const TypeBase& tb) : 
      type(tb), hasId(false), factory(f), hasInstance(false) { if (name) { id = name; hasId = true; } }

    inline virtual ~InstanceBase() { if (factory) delete factory; }

    inline std::vector<RequirementBase*>& getRequirements() { return requirements; }

    inline void setFactory(FactoryBase* newFactory) { if (factory) delete factory; factory = newFactory; }

    bool canConvertTo(const TypeBase& other) const;

    virtual void instantiateInstance(di::Context*) = 0;

    virtual void reset() = 0;
  public:
    virtual const void* getConcrete() const = 0;

    void* convertTo(const TypeBase& typeToConvertTo) const throw (DependencyInjectionException);

    inline bool instantiated() { return hasInstance; }

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

    virtual void satisfy(InstanceBase* instance, Context* context) throw (DependencyInjectionException) = 0;
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

