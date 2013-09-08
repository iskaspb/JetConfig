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
    const jet::ConfigSource source("<config> <attr> value</attr></config>");
    EXPECT_EQ("<config><attr>value</attr></config>", source.toString(jet::ConfigSource::OneLine));
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
    const jet::ConfigSource source("<config><attr>value  </attr></config>");
    EXPECT_EQ(
        "<config>\n"
        "  <attr>value</attr>\n"
        "</config>\n",
        source.toString());
}

TEST(ConfigSource, AttrConfigSource)
{
    const jet::ConfigSource source("<config attr='value'>   </config>");
    EXPECT_EQ(
        "<config>\n"
        "  <attr>value</attr>\n"
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
    const jet::ConfigSource source("<config attr1='value1' attr2=\"value2\"><attr3 attr4='value4'><attr5>value5</attr5></attr3></config>");
    EXPECT_EQ(
        "<config>\n"
        "  <attr1>value1</attr1>\n"
        "  <attr2>value2</attr2>\n"
        "  <attr3>\n"
        "    <attr4>value4</attr4>\n"
        "    <attr5>value5</attr5>\n"
        "  </attr3>\n"
        "</config>\n",
        source.toString());
}

TEST(ConfigSource, InconsistentAttributeDefinitionConfigSource)
{
    CONFIG_ERROR(
        jet::ConfigSource(
            "<config attr='value1'>\n"
            "   data\n"
            "</config>\n",
            "InconsistentAttributeDefinitionConfigSource"),
        "Invalid element 'config' in config source 'InconsistentAttributeDefinitionConfigSource' contains both value and child attributes");
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

TEST(Config, NormalizeKeywords)
{
    EXPECT_EQ(jet::ConfigSource("<Config/>").toString(), "<config/>\n");
    EXPECT_EQ(
        jet::ConfigSource("<Shared/>").toString(jet::ConfigSource::OneLine),
        "<shared/>");
    EXPECT_EQ(
        jet::ConfigSource(
            "<config><Shared/></config>").toString(jet::ConfigSource::OneLine),
        "<config><shared/></config>");
    EXPECT_EQ(
        jet::ConfigSource(
            "<app><Instance><i1/></Instance></app>"
            ).toString(jet::ConfigSource::OneLine),
        "<app><instance><i1/></instance></app>");
    EXPECT_EQ(
        jet::ConfigSource(
            "<config><app><Instance><i1/></Instance></app></config>"
            ).toString(jet::ConfigSource::OneLine),
        "<config><app><instance><i1/></instance></app></config>");
    EXPECT_EQ(jet::ConfigSource("<APP/>").toString(), "<APP/>\n");
    EXPECT_EQ(//...this is for Windows file name convention
        jet::ConfigSource(
            "<APP/>",
            "s1.xml",
            jet::ConfigSource::xml,
            jet::ConfigSource::CaseInsensitive
            ).toString(),
        "<app/>\n");
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

TEST(Config, InconsistentConfigName)
{
    const jet::ConfigSource source("<wrongConfigName><attr>value</attr></wrongConfigName>", "xmlSource");
    CONFIG_ERROR(jet::Config("ApplicationName") << source << jet::lock,
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

TEST(Config, SystemConfigInvalidData)
{
    const jet::ConfigSource s1("<config>data</config>", "s1.xml");
    CONFIG_ERROR(
        jet::Config("appName") << s1,
        "Invalid data node 'data' in config 'appName' taken from config source 's1.xml'");
}

TEST(Config, AppConfigInvalidData)
{
    const jet::ConfigSource s1("<config><appName>data</appName></config>", "s1.xml");
    CONFIG_ERROR(
        jet::Config("appName") << s1,
        "Invalid data node 'data' in config 'appName' taken from config source 's1.xml'");
}

TEST(Config, SharedConfigInvalidData)
{
    const jet::ConfigSource s1("<config><shared>data</shared></config>", "s1.xml");
    CONFIG_ERROR(
        jet::Config("appName") << s1,
        "Invalid data node 'data' in config 'appName' taken from config source 's1.xml'");
}

TEST(Config, InstanceConfigInvalidData1)
{
    const jet::ConfigSource s1("<config><appName:i1>data</appName:i1></config>", "s1.xml");
    jet::Config config("appName", "i1");
    CONFIG_ERROR(
        config << s1 << jet::lock,
        "Invalid data node 'data' in config 'appName:i1' taken from config source 's1.xml'");
}

TEST(Config, InstanceConfigInvalidData2)
{
    const jet::ConfigSource s3("<config><appName><instance><i1>data</i1></instance></appName></config>", "s3.xml");
    jet::Config config("appName", "i1");
    CONFIG_ERROR(
        config << s3 << jet::lock,
        "Invalid data node 'data' in config 'appName:i1' taken from config source 's3.xml'");
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

TEST(Config, ProhibitedSimpleSharedAttributes)
{//...this is to prevent situation when shared attributes are treated as default values for simple attributes in application config
    const jet::ConfigSource s1(
        "<shared attr1='value1'>\n"
        "   <attr2>value2</attr2>\n"
        "</shared>",
        "s1.xml");
    CONFIG_ERROR(
        jet::Config("appName") << s1,
        "Attribute 'attr1' in 'shared' section of config source 's1.xml' must be defined under subsection");
}

TEST(Config, ProhibitedInstanceSubsectionInSharedSection)
{//...'instance' is keyword
    const jet::ConfigSource s1(
        "<shared>\n"
        "   <instance attr='value'/>\n"
        "</shared>",
        "s1.xml");
    CONFIG_ERROR(
        jet::Config("appName") << s1,
        "Subsecion name 'instance' is prohibited in 'shared' section. Config source 's1.xml'");
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

//TODO: add tests that merges sections of 'instance', <appConfig> and 'shared' in different combinations
//TODO: add test for serialization of Config
//TODO: add 'lock' optimization
//TODO: Create <config> root element if ConfigSource doesn't have
//TODO: provide way to get sequence of parameters with the same name
//TODO: add command line config source
//TODO: add environment config source
