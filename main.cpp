//
//  main.cpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 26/8/13.
//  This code belongs to public domain. You can do with it whatever you want without any guarantee.
//
#include "gtest.hpp"
#include "Config.hpp"
#include "ConfigError.hpp"
#include <iostream>

using std::cout;
using std::endl;

TEST(ConfigSource, SimpleConfigSource)
{
    const jet::ConfigSource source("<app> <attr> value</attr></app>");
    EXPECT_EQ(
        "<config><app><attr>value</attr></app></config>",
        source.toString(jet::ConfigSource::OneLine));
}

TEST(ConfigSource, EmptyConfigSource)
{
    {
        const jet::ConfigSource source("<config></config>");
        EXPECT_EQ(
            "<config/>\n",
            source.toString());
    }
    CONFIG_ERROR(jet::ConfigSource("  "),
        "Couldn't parse config 'unknown'. Reason:");
}

TEST(ConfigSource, InvalidConfigSource)
{
    CONFIG_ERROR(jet::ConfigSource("  invalid ", "config.xml"),
        "Couldn't parse config 'config.xml'. Reason:");
}

TEST(ConfigSource, SimpleConfigSourcePrettyPrint)
{
    const jet::ConfigSource source("<app><attr>value  </attr></app>");
    EXPECT_EQ(
        "<config>\n"
        "  <app>\n"
        "    <attr>value</attr>\n"
        "  </app>\n"
        "</config>\n",
        source.toString());
}

TEST(ConfigSource, AttrConfigSource)
{
    const jet::ConfigSource source("<app attr='value'/>");
    EXPECT_EQ(
        "<config>\n"
        "  <app>\n"
        "    <attr>value</attr>\n"
        "  </app>\n"
        "</config>\n",
        source.toString());
}

TEST(ConfigSource, NormalizeColonConfigSource)
{
    const jet::ConfigSource source("<config><app:i1 attr='value'/></config>");
    EXPECT_EQ(
        "<config>\n"
        "  <app>\n"
        "    <instance>\n"
        "      <i1>\n"
        "        <attr>value</attr>\n"
        "      </i1>\n"
        "    </instance>\n"
        "  </app>\n"
        "</config>\n",
        source.toString());
}

TEST(ConfigSource, ErrorColonConfigSource)
{
    CONFIG_ERROR(
        jet::ConfigSource("<config><app: attr='value'/></config>", "s1.xml"),
        "Invalid ':' in element 'app:' in config source 's1.xml'. Expected format 'appName:instanceName'");
    CONFIG_ERROR(
        jet::ConfigSource("<config><:i1 attr='value'/></config>", "s1.xml"),
        "Invalid ':' in element ':i1' in config source 's1.xml'. Expected format 'appName:instanceName'");
}

TEST(ConfigSource, CombineAbbriviatedInstanceConfigSource1)
{
    const jet::ConfigSource source(
        "<config>"
        "   <app:i1 attr='value'/>"
        "   <app:i2 attr2='value2'/>"
        "</config>");
    EXPECT_EQ(
        "<config>\n"
        "  <app>\n"
        "    <instance>\n"
        "      <i1>\n"
        "        <attr>value</attr>\n"
        "      </i1>\n"
        "      <i2>\n"
        "        <attr2>value2</attr2>\n"
        "      </i2>\n"
        "    </instance>\n"
        "  </app>\n"
        "</config>\n",
        source.toString());
}

TEST(ConfigSource, CombineAbbriviatedInstanceConfigSource2)
{
    const jet::ConfigSource source(
        "<config>"
        "   <app attr='value' attr1='value1'/>"
        "   <app:i2 attr1='value11' attr2='value2'/>"
        "</config>");
    EXPECT_EQ(
        "<config>\n"
        "  <app>\n"
        "    <attr>value</attr>\n"
        "    <attr1>value1</attr1>\n"
        "    <instance>\n"
        "      <i2>\n"
        "        <attr1>value11</attr1>\n"
        "        <attr2>value2</attr2>\n"
        "      </i2>\n"
        "    </instance>\n"
        "  </app>\n"
        "</config>\n",
        source.toString());
}

TEST(ConfigSource, ComplexConfigSource)
{
    const jet::ConfigSource source("<app attr1='value1' attr2=\"value2\"><attr3 attr4='value4'><attr5>value5</attr5></attr3></app>");
    EXPECT_EQ(
        "<config>\n"
        "  <app>\n"
        "    <attr1>value1</attr1>\n"
        "    <attr2>value2</attr2>\n"
        "    <attr3>\n"
        "      <attr4>value4</attr4>\n"
        "      <attr5>value5</attr5>\n"
        "    </attr3>\n"
        "  </app>\n"
        "</config>\n",
        source.toString());
}

TEST(ConfigSource, SystemConfigInvalidData)
{
    CONFIG_ERROR(
        jet::ConfigSource("<config>12345678901</config>", "s1.xml"),
        "Invalid data node '1234567...' under 'config' node in config source 's1.xml'");
}

TEST(ConfigSource, AppConfigInvalidData)
{
    CONFIG_ERROR(
        jet::ConfigSource("<config><appName>data</appName></config>", "s1.xml"),
        "Invalid data node 'data' under 'appName' node in config source 's1.xml'");
}

TEST(ConfigSource, SharedConfigInvalidData)
{
    CONFIG_ERROR(
        jet::ConfigSource("<config><shared>data123</shared></config>", "s1.xml"),
        "Invalid data node 'data123' under 'shared' node in config source 's1.xml'");
}

TEST(ConfigSource, InstanceConfigInvalidData)
{
    CONFIG_ERROR(
        jet::ConfigSource("<config><appName:i1>data</appName:i1></config>", "s1.xml"),
        "Invalid data node 'data' under 'appName:i1' node in config source 's1.xml'");
}

TEST(ConfigSource, InconsistentAttributeDefinitionConfigSource)
{
    CONFIG_ERROR(
        jet::ConfigSource(
            "<app>"
            "   <env attr='value1'>"
            "       data"
            "   </env>"
            "</app>",
            "InconsistentAttributeDefinitionConfigSource"),
        "Invalid element 'config.app.env' in config source 'InconsistentAttributeDefinitionConfigSource' contains both value and child attributes");
}

TEST(ConfigSource, DuplicateAttrConfigSource)
{//TODO: submit bugreport to boost comunity - two attributes with the same name is not well formed xml
    CONFIG_ERROR(jet::ConfigSource("<config attr='value1' attr='value2'></config>"),
        "Duplicate definition of attribute 'config.attr' in config 'unknown'");
}

TEST(ConfigSource, InvalidSharedNodeWithInstance)
{
    CONFIG_ERROR(
        jet::ConfigSource("<config><shared:i1><env PATH='/usr/bin'/></shared></config:i1>", "s1.xml"),
        "Shared node 'shared:i1' can't have instance. Found in config source 's1.xml'");
}


TEST(ConfigSource, InvalidDuplicates)
{
    CONFIG_ERROR(
        jet::ConfigSource(
            "<config>"
            "   <shared><lib attr='value'/></shared>"
            "   <shared><env PATH='/usr/bin'/></shared>"
            "</config>",
            "s1.xml"),
        "Duplicate shared node in config source 's1.xml'");
    CONFIG_ERROR(
        jet::ConfigSource(
            "<config>"
            "   <shared>"
            "       <lib attr='value'/>"
            "       <lib attr2='value2'/>"
            "   </shared>"
            "</config>",
            "s1.xml"),
        "Duplicate shared node 'lib' in config source 's1.xml'");
    CONFIG_ERROR(
        jet::ConfigSource(
            "   <shared>"
            "       <lib attr='value'/>"
            "       <lib attr2='value2'/>"
            "   </shared>",
            "s1.xml"),
        "Duplicate shared node 'lib' in config source 's1.xml'");
    CONFIG_ERROR(
        jet::ConfigSource(
            "<config>"
            "   <appName attr='value'/>"
            "   <appName><env PATH='/usr/bin'/></appName>"
            "</config>", "s1.xml"),
        "Duplicate node 'appName' in config source 's1.xml'");
    CONFIG_ERROR(
        jet::ConfigSource(
            "<config>\n"
            "   <appName:i1><env PATH='/usr/bin'/></appName:i1>\n"
            "   <appName:i1><env PATH='/usr:/usr/bin'/></appName:i1>\n"
            "</config>\n",
            "s1.xml"),
        "Duplicate node 'appName:i1' in config source 's1.xml'");
    CONFIG_ERROR(
        jet::ConfigSource(
            "<config>\n"
            "   <appName><instance><i1 attr='value'/></instance></appName>\n"
            "   <appName:i1><env PATH='/usr/bin'/></appName:i1>\n"
            "</config>\n",
            "s1.xml"),
        "Duplicate node 'appName:i1' in config source 's1.xml'");
    CONFIG_ERROR(
        jet::ConfigSource(
            "<config>\n"
            "   <appName>\n"
            "       <instance><i1 attr='value'/></instance>\n"
            "       <instance><i2 attr='value'/></instance>\n"
            "   </appName>\n"
            "</config>\n",
            "s1.xml"),
        "Duplicate instance node under 'appName' node in config source 's1.xml'");
    CONFIG_ERROR(
        jet::ConfigSource(
            "<appName>\n"
            "   <instance><i1 attr='value'/></instance>\n"
            "   <instance><i2 attr='value'/></instance>\n"
            "</appName>\n",
            "s1.xml"),
        "Duplicate instance node under 'appName' node in config source 's1.xml'");
    CONFIG_ERROR(
        jet::ConfigSource(
            "<config>\n"
            "   <appName>"
            "       <instance>"
            "           <i1 attr='value'/>"
            "           <i1 attr2='value2'/>"
            "       </instance>"
            "   </appName>\n"
            "</config>\n",
            "s1.xml"),
        "Duplicate node 'appName:i1' in config source 's1.xml'");
}

TEST(ConfigSource, NormalizeKeywords)
{
    EXPECT_EQ(jet::ConfigSource("<Config/>").toString(), "<config/>\n");
    EXPECT_EQ(
        jet::ConfigSource("<Shared/>").toString(jet::ConfigSource::OneLine),
        "<config><shared/></config>");
    EXPECT_EQ(
        jet::ConfigSource(
            "<config><Shared/></config>").toString(jet::ConfigSource::OneLine),
        "<config><shared/></config>");
    EXPECT_EQ(
        jet::ConfigSource(
            "<app><Instance><i1/></Instance></app>"
            ).toString(jet::ConfigSource::OneLine),
        "<config><app><instance><i1/></instance></app></config>");
    EXPECT_EQ(
        jet::ConfigSource(
            "<config><app><Instance><i1/></Instance></app></config>"
            ).toString(jet::ConfigSource::OneLine),
        "<config><app><instance><i1/></instance></app></config>");
    EXPECT_EQ(
        jet::ConfigSource("<APP/>").toString(jet::ConfigSource::OneLine),
        "<config><APP/></config>");
    EXPECT_EQ(//...this is for Windows file name convention
        jet::ConfigSource(
            "<APP/>",
            "s1.xml",
            jet::ConfigSource::xml,
            jet::ConfigSource::CaseInsensitive
            ).toString(jet::ConfigSource::OneLine),
        "<config><app/></config>");
}

TEST(ConfigSource, ProhibitedSimpleSharedAttributes)
{//...this is to prevent situation when shared attributes are treated as default values for simple attributes in application config
    CONFIG_ERROR(
        jet::ConfigSource(
            "<shared attr1='value1'>\n"
            "   <attr2>value2</attr2>\n"
            "</shared>",
            "s1.xml"),
        "Config source 's1.xml' is invalid: 'shared' node can not contain direct properties. See 'shared.attr1' property");
}

TEST(ConfigSource, ProhibitedInstanceSubsectionInSharedSection)
{//...not allowed because 'instance' is keyword
    CONFIG_ERROR(
        jet::ConfigSource(
            "<shared>\n"
            "   <Instance attr='value'/>\n"
            "</shared>",
            "s1.xml"),
        "Config source 's1.xml' is invalid: 'shared' node can not contain 'Instance' node");
}

TEST(Config, SharedAttrConfigWithoutRootConfigElement)
{
    const jet::ConfigSource shared(
        "<shared>\n"
        "   <libName strAttr='value' intAttr='12'>\n"
        "       <doubleAttr>13.2</doubleAttr>\n"
        "       <subKey attr='value'/>\n"
        "   </libName>\n"
        "</shared>\n",
        "shared.xml");
    jet::Config config("appName");
    config << shared << jet::lock;
    EXPECT_EQ(std::string("appName"), config.name());
    EXPECT_EQ("value",   config.get("libName.strAttr"));
    EXPECT_EQ("value",   config.get<std::string>("libName.strAttr"));
    EXPECT_EQ("12",      config.get("libName.intAttr"));
    EXPECT_EQ(12,        config.get<int>("libName.intAttr"));
    EXPECT_EQ("13.2",    config.get<std::string>("libName.doubleAttr"));
    EXPECT_EQ(13.2,      config.get<double>("libName.doubleAttr"));
    EXPECT_EQ("value",   config.get("libName.subKey.attr"));
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

TEST(Config, AnyCaseSystemConfigName)
{
    const jet::ConfigSource s1("<conFIG><appName attr='10'></appName></conFIG>", "s1.xml");
    jet::Config config("appName");
    config << s1;
    EXPECT_EQ(10, config.get<int>("attr"));
}

TEST(Config, InstanceConfigName)
{
    jet::Config config("appName", "i1");
    EXPECT_EQ("appName:i1", config.name());
    EXPECT_EQ("appName", config.appName());
    EXPECT_EQ("i1", config.instanceName());
}

TEST(Config, LockConfig)
{
    const jet::ConfigSource s1(
        "<appName:1 attr='value'/>",
        "s1.xml");
    const jet::ConfigSource s2(
        "<appName:1 attr2='10'/>",
        "s2.xml");
    jet::Config config("appName", "1");
    config << s1 << jet::lock;
    EXPECT_EQ("value", config.get("attr"));
    CONFIG_ERROR(
        config << s2,
        "Config 'appName:1' is locked");
}

TEST(Config, Getters)
{
    const jet::ConfigSource s1(
        "<app:1 str='value' int='10'/>",
        "s1.xml");

    jet::Config config("app", "1");
    config << s1 << jet::lock;
    
    //...simple getter
    EXPECT_EQ("value", config.get("str"));
    EXPECT_EQ(10, config.get<int>("int"));
    CONFIG_ERROR(
        config.get("unknown"),
        "Can't find property 'unknown' in config 'app:1'");
    CONFIG_ERROR(
        config.get<int>("str"),
        "Can't convert value 'value' of a property 'str' in config 'app:1'");
    
    //...optional getter
    EXPECT_EQ("value", *config.getOptional("str"));
    EXPECT_EQ(10, *config.getOptional<int>("int"));
    EXPECT_EQ(boost::none, config.getOptional("unknown"));
    CONFIG_ERROR(
        config.getOptional<int>("str"),
        "Can't convert value 'value' of a property 'str' in config 'app:1'");
    
    //...default getter
    EXPECT_EQ("value", config.get("str", "anotherValue"));
    EXPECT_EQ(10, config.get("int", 12));
    EXPECT_EQ("another value", config.get("unknown", "another value"));
    CONFIG_ERROR(
        config.get("str", 12),
        "Can't convert value 'value' of a property 'str' in config 'app:1'");
    
}

TEST(Config, Serialize)
{
    const jet::ConfigSource s1(
        "<app str='value1' int='10'/><app2 attr='value'/>",
        "s1.xml");
    const jet::ConfigSource s2(
        "<app:1 str='value2' float='10.'/>",
        "s2.xml");

    jet::Config config("app", "1");
    config << s1 << s2;

    const char* expectedResult =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<config>\n\
  <1>\n\
    <str>value2</str>\n\
    <float>10.</float>\n\
  </1>\n\
  <app>\n\
    <str>value1</str>\n\
    <int>10</int>\n\
  </app>\n\
  <shared/>\n\
</config>\n";

    std::stringstream strm;
    strm << config;
    EXPECT_EQ(expectedResult, strm.str());
}

//TODO: get rid of 'lock' logic and merge everything in in one pass
//TODO: add tests that merges sections of 'instance', <appConfig> and 'shared' in different combinations
//TODO: expose internal property tree for complex queries
//TODO: add command line config source
//TODO: add environment config source
