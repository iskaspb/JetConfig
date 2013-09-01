#include <gtest/gtest.h>
#include "Config.hpp"
#include <iostream>

using std::cout;
using std::endl;

#define EXPECT_EXCEPTION(EXPRESSION, EXCEPTION, MESSAGE) \
{ \
    const std::string message(MESSAGE); \
    try \
    { \
        do { EXPRESSION; } while(0); \
        FAIL() << "Negative test: exception " #EXCEPTION " is expected"; \
    } \
    catch(const EXCEPTION& ex) \
    { \
        const std::string exceptionMessage(std::string(ex.what()).substr(0, message.size())); \
        EXPECT_EQ(message, exceptionMessage); \
    } \
}

TEST(JetConfig, SimpleConfigSource)
{
    jet::ConfigSource source("<root> <attr> value</attr></root>");
    EXPECT_EQ("<root><attr>value</attr></root>", source.toString(jet::ConfigSource::OneLine));
}

TEST(JetConfig, EmptyConfigSource)
{
    EXPECT_EXCEPTION(jet::ConfigSource source("  "),
        jet::ConfigError, "Couldn't parse config 'unknown'. Reason:");
}

TEST(JetConfig, InvalidConfigSource)
{
    EXPECT_EXCEPTION(jet::ConfigSource source("  invalid "),
        jet::ConfigError, "Couldn't parse config 'unknown'. Reason:");
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
    EXPECT_EXCEPTION(jet::ConfigSource source("<root attr='value1'><attr>value2</attr></root>"),
        jet::ConfigError, "Ambiguous definition of attribute and element 'root.attr' in config 'unknown'");
}

TEST(JetConfig, DuplicateElementConfigSource)
{
    EXPECT_EXCEPTION(jet::ConfigSource source("<root><attr>value1</attr><attr>value2</attr></root>"),
        jet::ConfigError, "Duplicate definition of element 'root.attr' in config 'unknown'");
}

TEST(JetConfig, DuplicateAttrConfigSource)
{//TODO: submit bugreport to boost comunity - two attributes with the same name is not well formed xml
    EXPECT_EXCEPTION(jet::ConfigSource source("<root attr='value1' attr='value2'></root>"),
        jet::ConfigError, "Duplicate definition of attribute 'root.attr' in config 'unknown'");
}

TEST(JetConfig, AttrConfig)
{
    const jet::ConfigSource source(
        "<configName strAttr='value' intAttr='12'>\n"
        "  <doubleAttr>13.2</doubleAttr>\n"
        "  <subKey attr='value'/>\n"
        "</configName>\n",
        "testSource");
    const jet::Config config(source);
    EXPECT_EQ("configName", config.name());
    EXPECT_EQ("value",      config.get("strAttr"));
    EXPECT_EQ("value",      config.get<std::string>("strAttr"));
    EXPECT_EQ("12",         config.get("intAttr"));
    EXPECT_EQ(12,           config.get<int>("intAttr"));
    EXPECT_EQ("13.2",       config.get<std::string>("doubleAttr"));
    EXPECT_EQ(13.2,         config.get<double>("doubleAttr"));
    EXPECT_EQ("value",      config.get("subKey.attr"));
}
//...Config: negative test for inconsistent config name taken from configuration source and from constructor parameter
//...Config: negative test for inconsistent config name in two config sources during merge
//...Config: negative test case for empty configuration

//...Config/SystemConfig: negative test for data inside root element (root it's just a holder for config - having data without attribute name is useless)
//...SystemConfig: negative test for data inside second level element (it doesn't make sense because second level element is a process name and it should have attribute name)
//...Config: negative test for data inside <instance> element (instance config is specialization of process config so useless to have data without attribute)
//...Config/SystemConfig: negative test for data insdie <env> element (environment variables must be with names)