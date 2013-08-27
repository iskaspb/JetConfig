#include <gtest/gtest.h>
#include "Config.hpp"
#include <iostream>

using std::cout;
using std::endl;

TEST(JetConfig, SimpleConfigSource)
{
    jet::ConfigSource source("<root> <attr> value</attr></root>");
    EXPECT_EQ("<root><attr>value</attr></root>", source.toString(jet::ConfigSource::OneLine));
}

TEST(JetConfig, SimpleConfigSourcePrettyPrint)
{
    jet::ConfigSource source("<root><attr>value  </attr></root>");
    EXPECT_EQ(
        "<root>\n"
        "  <attr>value</attr>\n"
        "</root>\n",
        source.toString());
}

TEST(JetConfig, AttrConfigSource)
{
    jet::ConfigSource source("<root attr='value'>   </root>");
    EXPECT_EQ(
        "<root>\n"
        "  <attr>value</attr>\n"
        "</root>\n",
        source.toString());
}

TEST(JetConfig, ComplexConfigSource)
{
    jet::ConfigSource source("<root attr1='value1' attr2=\"value2\"><attr3 attr4='value4'><attr5>value5</attr5>value3</attr3></root>");
    EXPECT_EQ(
        "<root>\n"
        "  <attr1>value1</attr1>\n"
        "  <attr2>value2</attr2>\n"
        "  <attr3>\n"
        "    value3\n"
        "    <attr4>value4</attr4>\n"
        "    <attr5>value5</attr5>\n"
        "  </attr3>\n"
        "</root>\n",
        source.toString());
}

TEST(JetConfig, AmbiguousAttrConfigSource)
{
    try
    {
        jet::ConfigSource source("<root attr='value1'><attr>value2</attr></root>");
        FAIL() << "Negative test for duplicate attribute: exception is expected";
    }
    catch(const jet::ConfigError& ex)
    {
        EXPECT_EQ(
            "Ambiguous definition of attribute and element 'root.attr' in config 'unknown'",
            std::string(ex.what()));
    }
}

TEST(JetConfig, DuplicateElementConfigSource)
{
    try
    {
        jet::ConfigSource source("<root><attr>value1</attr><attr>value2</attr></root>");
        FAIL() << "Negative test for duplicate element: exception is expected";
    }
    catch(const jet::ConfigError& ex)
    {
        EXPECT_EQ(
            "Duplicate definition of element 'root.attr' in config 'unknown'",
            std::string(ex.what()));
    }
}

TEST(JetConfig, DuplicateAttrConfigSource)
{//TODO: submit bugreport to boost comunity - two attributes with the same name is not well formed xml
    try
    {
        jet::ConfigSource source("<root attr='value1' attr='value2'></root>");
        FAIL() << "Negative test for duplicate attribute: exception is expected";
    }
    catch(const jet::ConfigError& ex)
    {
        EXPECT_EQ(
            "Duplicate definition of attribute 'root.attr' in config 'unknown'",
            std::string(ex.what()));
    }
}


//...ProcessConfig/SystemConfig: negative test for data inside root element (root it's just a holder for config - having data without attribute name is useless)
//...SystemConfig: negative test for data inside second level element (it doesn't make sense because second level element is a process name and it should have attribute name)
//...ProcessConfig: negative test for data inside <instance> element (instance config is specialization of process config so useless to have data without attribute)
//...ProcessConfig/SystemConfig: negative test for data insdie <env> element (environment variables must be with names)