//
//  main.cpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 26/8/13.
//  Copyright (c) 2013 Alexey Tkachenko. All rights reserved.
//
#include "gtest.hpp"
#include "Config.hpp"
#include "ConfigError.hpp"
#include <iostream>

using std::cout;
using std::endl;

TEST(ConfigSource, SimpleConfigSource)
{
    const jet::ConfigSource source("<root> <attr> value</attr></root>");
    EXPECT_EQ("<root><attr>value</attr></root>", source.toString(jet::ConfigSource::OneLine));
}

TEST(ConfigSource, EmptyConfigSource)
{
    CONFIG_ERROR(jet::ConfigSource("  "),
        "Couldn't parse config 'unknown'. Reason:");
}

TEST(ConfigSource, InvalidConfigSource)
{
    CONFIG_ERROR(jet::ConfigSource("  invalid "),
        "Couldn't parse config 'unknown'. Reason:");
}

TEST(ConfigSource, SimpleConfigSourcePrettyPrint)
{
    const jet::ConfigSource source("<root><attr>value  </attr></root>");
    EXPECT_EQ(
        "<root>\n"
        "  <attr>value</attr>\n"
        "</root>\n",
        source.toString());
}

TEST(ConfigSource, AttrConfigSource)
{
    const jet::ConfigSource source("<root attr='value'>   </root>");
    EXPECT_EQ(
        "<root>\n"
        "  <attr>value</attr>\n"
        "</root>\n",
        source.toString());
}

TEST(ConfigSource, ComplexConfigSource)
{
    const jet::ConfigSource source("<root attr1='value1' attr2=\"value2\"><attr3 attr4='value4'><attr5>value5</attr5></attr3></root>");
    EXPECT_EQ(
        "<root>\n"
        "  <attr1>value1</attr1>\n"
        "  <attr2>value2</attr2>\n"
        "  <attr3>\n"
        "    <attr4>value4</attr4>\n"
        "    <attr5>value5</attr5>\n"
        "  </attr3>\n"
        "</root>\n",
        source.toString());
}

TEST(ConfigSource, AmbiguousAttrConfigSource)
{
    CONFIG_ERROR(jet::ConfigSource("<root attr='value1'><attr>value2</attr></root>"),
        "Ambiguous definition of attribute and element 'root.attr' in config 'unknown'");
}

TEST(ConfigSource, InconsistentAttributeDefinitionConfigSource)
{
    CONFIG_ERROR(
        jet::ConfigSource(
            "<root attr='value1'>\n"
            "   data\n"
            "</root>\n",
            "InconsistentAttributeDefinitionConfigSource"),
        "Invalid element 'root' in config 'InconsistentAttributeDefinitionConfigSource' contains both value and child attributes");
}

TEST(ConfigSource, DuplicateElementConfigSource)
{
    CONFIG_ERROR(jet::ConfigSource("<root><attr>value1</attr><attr>value2</attr></root>"),
        "Duplicate definition of element 'root.attr' in config 'unknown'");
}

TEST(ConfigSource, DuplicateAttrConfigSource)
{//TODO: submit bugreport to boost comunity - two attributes with the same name is not well formed xml
    CONFIG_ERROR(jet::ConfigSource("<root attr='value1' attr='value2'></root>"),
        "Duplicate definition of attribute 'root.attr' in config 'unknown'");
}

TEST(Config, SharedAttrConfigWithoutRootConfigElement)
{
    const jet::ConfigSource shared(
        "<shared strAttr='value' intAttr='12'>\n"
        "  <doubleAttr>13.2</doubleAttr>\n"
        "  <subKey attr='value'/>\n"
        "</shared>\n",
        "shared.xml");
    const jet::Config config(shared, "appName");
    EXPECT_EQ("appName", config.name());
    EXPECT_EQ("value",   config.get("strAttr"));
    EXPECT_EQ("value",   config.get<std::string>("strAttr"));
    EXPECT_EQ("12",      config.get("intAttr"));
    EXPECT_EQ(12,        config.get<int>("intAttr"));
    EXPECT_EQ("13.2",    config.get<std::string>("doubleAttr"));
    EXPECT_EQ(13.2,      config.get<double>("doubleAttr"));
    EXPECT_EQ("value",   config.get("subKey.attr"));
}

TEST(Config, InconsistentConfigName)
{
    const jet::ConfigSource source("<wrongConfigName><attr>value</attr></wrongConfigName>", "xmlSource");
    CONFIG_ERROR(jet::Config(source, "ApplicationName"),
        "Can't merge source 'xmlSource' into config 'ApplicationName' because it has different config name 'wrongConfigName'");
}

TEST(Config, AttribConfigSourceMerge)
{
    const jet::ConfigSource s1("<appName attr1='10' attr2='something'></appName>");
    const jet::ConfigSource s2("<appName attr2='20' attr3='value3'></appName>");
    jet::Config config("appName");
    config << s1 << s2;
    EXPECT_EQ("appName", config.name());
    EXPECT_EQ(10,        config.get<int>("attr1"));
    EXPECT_EQ(20,        config.get<int>("attr2"));
    EXPECT_EQ("value3",  config.get("attr3"));
}

TEST(Config, SubtreeConfigSourceMerge)
{
    const jet::ConfigSource s1(
        "<appName>\n"
        "  <attribs attr1='10' attr2='something'></attribs>\n"
        "</appName>\n",
        "s1");
    const jet::ConfigSource s2(
        "<appName>\n"
        "  <attribs attr2='20' attr3='value3'>\n"
        "    <subkey attr='data'></subkey>\n"
        "  </attribs>\n"
        "</appName>\n",
        "s2");
    jet::Config config("appName");
    config << s1 << s2;
    EXPECT_EQ("appName", config.name());
    EXPECT_EQ(10,        config.get<int>("attribs.attr1"));
    EXPECT_EQ(20,        config.get<int>("attribs.attr2"));
    EXPECT_EQ("value3",  config.get("attribs.attr3"));
    EXPECT_EQ("data",    config.get("attribs.subkey.attr"));
}

TEST(Config, UnmatchedNamesConfigSourceMerge)
{
    const jet::ConfigSource s1("<appName attr1='10' attr2='something'></appName>", "s1.xml");
    const jet::ConfigSource s2("<anotherAppName attr2='20' attr3='value3'></appName>", "s2.xml");
    CONFIG_ERROR(jet::Config("appName") << s1 << s2,
        "Can't merge source 's2.xml' into config 'appName' because it has different config name 'anotherAppName'");
}

TEST(Config, AnyCaseSystemConfigName)
{
    const jet::ConfigSource s1("<conFIG><appName attr='10'></appName></conFIG>", "s1.xml");
    const jet::Config config(s1, "appName");
    EXPECT_EQ(10, config.get<int>("attr"));
}

TEST(Config, SystemConfigInvalidData)
{
    const jet::ConfigSource s1("<config>data</config>", "s1.xml");
    CONFIG_ERROR(
        const jet::Config config(s1, "appName"),
        "Invalid data node 'data' in config 'appName' taken from config source 's1.xml'");
}

TEST(Config, AppConfigInvalidData)
{
    const jet::ConfigSource s1("<config><appName>data</appName></config>", "s1.xml");
    CONFIG_ERROR(
        const jet::Config config(s1, "appName"),
        "Invalid data node 'data' in config 'appName' taken from config source 's1.xml'");
}

TEST(Config, SharedConfigInvalidData)
{
    const jet::ConfigSource s1("<config><shared>data</shared></config>", "s1.xml");
    CONFIG_ERROR(
        const jet::Config config(s1, "appName"),
        "Invalid data node 'data' in config 'appName' taken from config source 's1.xml'");
}

TEST(Config, InstanceConfigName)
{
    jet::Config config("appName.i1");
    EXPECT_EQ("appName.i1", config.name());
    EXPECT_EQ("appName", config.appName());
    EXPECT_EQ("i1", config.instanceName());
}

TEST(Config, InstanceConfigInvalidData1)
{
    const jet::ConfigSource s1("<config><appName.i1>data</appName.i1></config>", "s1.xml");
    jet::Config config("appName.i1");
    CONFIG_ERROR(
        config << s1 << jet::lock,
        "Invalid data node 'data' in config 'appName.i1' taken from config source 's1.xml'");
}
TEST(Config, InstanceConfigInvalidData2)
{
    const jet::ConfigSource s2("<config><appName><instance.i1>data</instance.i1></appName></config>", "s2.xml");
    jet::Config config("appName.i1");
    CONFIG_ERROR(
        config << s2 << jet::lock,
        "Invalid data node 'data' in config 'appName.i1' taken from config source 's2.xml'");
}
TEST(Config, InstanceConfigInvalidData3)
{
    const jet::ConfigSource s3("<config><appName><instance><i1>data</i1></instance></appName></config>", "s3.xml");
    jet::Config config("appName.i1");
    CONFIG_ERROR(
        config << s3 << jet::lock,
        "Invalid data node 'data' in config 'appName.i1' taken from config source 's3.xml'");
}

//TODO: add test case to check that 'instance' is not treated as attribute name