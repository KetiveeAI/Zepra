/**
 * @file class_tests.cpp
 * @brief Unit tests for ES6 Class functionality
 */

#include <gtest/gtest.h>
#include "runtime/objects/class_fields.hpp"
#include "runtime/objects/function.hpp"
#include "runtime/objects/object.hpp"
#include "runtime/objects/value.hpp"

using namespace Zepra::Runtime;

// =============================================================================
// PrivateFieldName Tests
// =============================================================================

TEST(ClassTests, PrivateFieldNameUniqueness) {
    PrivateFieldName name1("field");
    PrivateFieldName name2("field");
    
    EXPECT_EQ(name1.name(), "field");
    EXPECT_EQ(name2.name(), "field");
    
    // IDs should differ even if names are same
    EXPECT_NE(name1.id(), name2.id());
    EXPECT_FALSE(name1 == name2);
    
    PrivateFieldName copy = name1;
    EXPECT_EQ(copy.id(), name1.id());
    EXPECT_TRUE(copy == name1);
}

// =============================================================================
// ClassDefinition Tests
// =============================================================================

TEST(ClassTests, ClassDefinitionBasics) {
    ClassDefinition def("MyClass");
    EXPECT_EQ(def.name(), "MyClass");
    EXPECT_EQ(def.superClass(), nullptr);
    EXPECT_EQ(def.constructor(), nullptr);
}

TEST(ClassTests, ClassDefinitionFields) {
    ClassDefinition def("Fields");
    
    def.addPublicField("pub", Value::number(1));
    def.addPrivateField("priv", Value::number(2));
    def.addStaticField("staticPub", Value::number(3));
    def.addStaticField("staticPriv", Value::number(4), true);
    
    const auto& fields = def.fields();
    ASSERT_EQ(fields.size(), 4);
    
    EXPECT_EQ(fields[0].name, "pub");
    EXPECT_FALSE(fields[0].isPrivate);
    EXPECT_FALSE(fields[0].isStatic);
    
    EXPECT_EQ(fields[1].name, "priv");
    EXPECT_TRUE(fields[1].isPrivate);
    EXPECT_FALSE(fields[1].isStatic);
    
    EXPECT_EQ(fields[2].name, "staticPub");
    EXPECT_FALSE(fields[2].isPrivate);
    EXPECT_TRUE(fields[2].isStatic);
    
    EXPECT_EQ(fields[3].name, "staticPriv");
    EXPECT_TRUE(fields[3].isPrivate);
    EXPECT_TRUE(fields[3].isStatic);
}

TEST(ClassTests, ClassBuilderStructure) {
    ClassDefinition def("Point");
    
    // Build creates constructor and prototype
    Function* Ctor = def.build();
    
    ASSERT_NE(Ctor, nullptr);
    EXPECT_EQ(Ctor->name(), "Point");
    
    // Check prototype setup
    Value protoVal = Ctor->get("prototype");
    EXPECT_TRUE(protoVal.isObject());
    
    Object* proto = protoVal.asObject();
    Value ctorVal = proto->get("constructor");
    EXPECT_TRUE(ctorVal.isObject());
    EXPECT_EQ(ctorVal.asObject(), Ctor);
    
    delete Ctor;  // Prototype is owned by Ctor usually via cycle, simplified here check memory model
    // Note: In real GC, these would be collected. Here we manually managing for test.
    // Assuming simple new/delete for test objects unless part of a VM heap.
}

// =============================================================================
// ClassInstance & Private Fields Tests
// =============================================================================

TEST(ClassTests, ClassInstancePrivateStorage) {
    ClassBrand brand = ClassBrand::create();
    ClassInstance instance(brand);
    
    EXPECT_TRUE(instance.hasBrand(brand));
    
    PrivateFieldName field1("x");
    PrivateFieldName field2("y");
    
    instance.setPrivate(field1, Value::number(10));
    instance.setPrivate(field2, Value::number(20));
    
    EXPECT_TRUE(instance.hasPrivate(field1));
    EXPECT_TRUE(instance.hasPrivate(field2));
    
    EXPECT_EQ(instance.getPrivate(field1).asNumber(), 10.0);
    EXPECT_EQ(instance.getPrivate(field2).asNumber(), 20.0);
    
    // Non-existent field
    PrivateFieldName field3("z");
    EXPECT_FALSE(instance.hasPrivate(field3)); 
    EXPECT_TRUE(instance.getPrivate(field3).isUndefined());
}

// =============================================================================
// ClassUtils Tests
// =============================================================================

TEST(ClassTests, InitializeInstanceFields) {
    ClassDefinition def("Data");
    def.addPublicField("x", Value::number(100));
    def.addPrivateField("secret", Value::string(new String("shh")));
    
    // Needed to store private fields
    ClassBrand brand = ClassBrand::create();
    ClassInstance* instance = new ClassInstance(brand);
    
    ClassUtils::initializeInstanceFields(instance, def);
    
    // Check public field
    EXPECT_EQ(instance->get("x").asNumber(), 100.0);
    
    // Check private field
    const PrivateFieldName& secretName = def.getPrivateFieldName("secret");
    EXPECT_TRUE(instance->hasPrivate(secretName));
    EXPECT_EQ(instance->getPrivate(secretName).toString(), "shh");
    
    delete instance;
}

TEST(ClassTests, InitializeStatics) {
    ClassDefinition def("MathHelper");
    def.addStaticField("PI", Value::number(3.14));
    
    bool blockRan = false;
    Function* staticBlock = new Function("block", [&blockRan](const FunctionCallInfo&) -> Value {
        blockRan = true;
        return Value::undefined();
    }, 0);
    
    def.addStaticBlock(staticBlock);
    
    Function* Ctor = def.build();
    ClassUtils::initializeStatics(Ctor, def);
    
    // Check static field
    EXPECT_EQ(Ctor->get("PI").asNumber(), 3.14);
    
    // Check static block execution
    EXPECT_TRUE(blockRan);
    
    delete Ctor;
    delete staticBlock;
}

TEST(ClassTests, InheritanceSetup) {
    ClassDefinition baseDef("Base");
    Function* Base = baseDef.build();
    
    ClassDefinition subDef("Sub");
    subDef.setExtends(Base);
    Function* Sub = subDef.build();
    
    // Check proto chain: Sub.prototype.__proto__ === Base.prototype
    Object* subProto = Sub->get("prototype").asObject();
    Object* baseProto = Base->get("prototype").asObject();
    
    EXPECT_EQ(subProto->prototype(), baseProto);
    
    delete Base;
    delete Sub;
}
