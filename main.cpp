#include "gtest.hpp"
#include "Config.hpp"
#include <iostream>

using std::cout;
using std::endl;

TEST(JetConfig, SimpleConfigSource)
{
    const jet::ConfigSource source("<root> <attr> value</attr></root>");
    EXPECT_EQ("<root><attr>value</attr></root>", source.toString(jet::ConfigSource::OneLine));
}

TEST(JetConfig, EmptyConfigSource)
{
    CONFIG_ERROR(const jet::ConfigSource source("  "),
        "Couldn't parse config 'unknown'. Reason:");
}

TEST(JetConfig, InvalidConfigSource)
{
    CONFIG_ERROR(const jet::ConfigSource source("  invalid "),
        "Couldn't parse config 'unknown'. Reason:");
}

TEST(JetConfig, SimpleConfigSourcePrettyPrint)
{
    const jet::ConfigSource source("<root><attr>value  </attr></root>");
    EXPECT_EQ(
        "<root>\n"
        "  <attr>value</attr>\n"
        "</root>\n",
        source.toString());
}

TEST(JetConfig, AttrConfigSource)
{
    const jet::ConfigSource source("<root attr='value'>   </root>");
    EXPECT_EQ(
        "<root>\n"
        "  <attr>value</attr>\n"
        "</root>\n",
        source.toString());
}

TEST(JetConfig, ComplexConfigSource)
{
    const jet::ConfigSource source("<root attr1='value1' attr2=\"value2\"><attr3 attr4='value4'><attr5>value5</attr5>value3</attr3></root>");
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
    CONFIG_ERROR(const jet::ConfigSource source("<root attr='value1'><attr>value2</attr></root>"),
        "Ambiguous definition of attribute and element 'root.attr' in config 'unknown'");
}

TEST(JetConfig, DuplicateElementConfigSource)
{
    CONFIG_ERROR(jet::ConfigSource source("<root><attr>value1</attr><attr>value2</attr></root>"),
        "Duplicate definition of element 'root.attr' in config 'unknown'");
}

TEST(JetConfig, DuplicateAttrConfigSource)
{//TODO: submit bugreport to boost comunity - two attributes with the same name is not well formed xml
    CONFIG_ERROR(const jet::ConfigSource source("<root attr='value1' attr='value2'></root>"),
        "Duplicate definition of attribute 'root.attr' in config 'unknown'");
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

TEST(JetConfig, InconsistentConfigName)
{
    const jet::ConfigSource source("<wrongConfigName><attr>value</attr></wrongConfigName>", "xmlSource");
    CONFIG_ERROR(const jet::Config config(source, "ApplicationName"),
        "Can't merge source 'xmlSource' into config 'ApplicationName' because it has different config name 'wrongConfigName'");
}

//...Config: negative test for inconsistent config name in two config sources during merge
//...Config: negative test case for empty configuration

//...Config/SystemConfig: negative test for data inside root element (root it's just a holder for config - having data without attribute name is useless)
//...SystemConfig: negative test for data inside second level element (it doesn't make sense because second level element is a process name and it should have attribute name)
//...Config: negative test for data inside <instance> element (instance config is specialization of process config so useless to have data without attribute)
//...Config/SystemConfig: negative test for data insdie <env> element (environment variables must be with names)