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
#include <boost/foreach.hpp>
#include <iostream>

using std::cout;
using std::endl;

TEST(ConfigSource, SimpleConfigSource)
{
    const jet::ConfigSource source("<app> <attr> value</attr></app> ");
    EXPECT_EQ(
        "<config><app><attr>value</attr></app></config>",
        source.toString(jet::ConfigSource::OneLine));
}

TEST(ConfigSource, EmptyConfigSource)
{
    {
        const jet::ConfigSource source("<config></config> ");
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
    const jet::ConfigSource source(" <app><attr>value  </attr></app>");
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
    const jet::ConfigSource source("<app attr='value'/> ");
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
    EXPECT_EQ("value",   config.get("libName.strAttr "));
    EXPECT_EQ("value",   config.get<std::string>(" libName.strAttr"));
    EXPECT_EQ("12",      config.get("libName.intAttr"));
    EXPECT_EQ(12,        config.get<int>("libName.intAttr "));
    EXPECT_EQ("13.2",    config.get<std::string>(" libName.doubleAttr"));
    EXPECT_EQ(13.2,      config.get<double>("  libName.doubleAttr"));
    EXPECT_EQ("value",   config.get("libName.subKey.attr"));
}

TEST(Config, AttribConfigSourceMerge)
{
    const jet::ConfigSource s1("<appName attr1='10' attr2='something'></appName>");
    const jet::ConfigSource s2("<appName attr2='20' attr3='value3'></appName>");
    jet::Config config("appName");
    config << s1 << s2 << jet::lock;
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
    config << s1 << s2 << jet::lock;
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
    config << s1 << jet::lock;
    EXPECT_EQ(10, config.get<int>("attr"));
}

TEST(Config, InstanceConfigName)
{
    jet::Config config("appName ", "i1 ");
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
    jet::Config config("appName ", " 1");
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
    EXPECT_EQ("value", config.getNode("str").get());
    EXPECT_EQ(10, config.get<int>("int"));
    CONFIG_ERROR(
        config.get("unknown"),
        "Can't find property 'unknown' in config 'app:1'");
    CONFIG_ERROR(
        config.get<int>("str"),
        "Can't convert value 'value' of a property 'str' in config 'app:1'");
    
    //...optional getter
    EXPECT_EQ("value", *config.getOptional("str"));
    EXPECT_EQ("value", *config.getNodeOptional("str")->getOptional());
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

TEST(Config, Initialization)
{
    const jet::ConfigSource s1(
        "<app str='value1' int='10'/><app:1><lib attr1='0s1'/></app:1><app2 attr='value'/><shared><lib attr1='1s1' attr2='2s1'/></shared>",
        "s1.xml");
    const jet::ConfigSource s2(
        "<config><app:1 str='value2' float='10.'/><shared><lib attr1='1s2'/></shared></config>",
        "s2.xml");

    jet::Config config("app", "1");
    config << s1 << s2;
    
    {//...check config before locking it
        const char* unlockedConfig =
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<config>\n\
  <1>\n\
    <lib>\n\
      <attr1>0s1</attr1>\n\
    </lib>\n\
    <str>value2</str>\n\
    <float>10.</float>\n\
  </1>\n\
  <app>\n\
    <str>value1</str>\n\
    <int>10</int>\n\
  </app>\n\
  <shared>\n\
    <lib>\n\
      <attr1>1s2</attr1>\n\
      <attr2>2s1</attr2>\n\
    </lib>\n\
  </shared>\n\
</config>\n";

        std::stringstream strm;
        strm << config;
        EXPECT_EQ(unlockedConfig, strm.str());
        CONFIG_ERROR(config.get("str"), "Initialization of config 'app:1' is not finished");
    }
    
    config << jet::lock;
    
    {//...and after
        const char* unlockedConfig =
"<app:1>\n\
<lib>\n\
  <attr1>0s1</attr1>\n\
  <attr2>2s1</attr2>\n\
</lib>\n\
<str>value2</str>\n\
<int>10</int>\n\
<float>10.</float>\n\
</app:1>\n";
        std::stringstream strm;
        strm << config;
        EXPECT_EQ(unlockedConfig, strm.str());
    }
}

TEST(Config, getNode)
{
    const jet::ConfigSource s1(
        "<appName>\n"
        "  <attribs attr1='10' attr2='something'></attribs>\n"
        "</appName>\n",
        "s1");
    const jet::ConfigSource s2(
        "<appName:1>\n"
        "  <attribs attr2='20' attr3='value3'>\n"
        "    <subkey attr='data'></subkey>\n"
        "  </attribs>\n"
        "</appName:1>\n",
        "s2");
    jet::Config config("appName", "1");
    config << s1 << s2 << jet::lock;
    const jet::ConfigNode attribs(config.getNode("attribs"));
    EXPECT_EQ("appName:1.attribs", attribs.name());
    EXPECT_EQ(10,        attribs.get<int>("attr1"));
    EXPECT_EQ(20,        attribs.get<int>("attr2"));
    EXPECT_EQ("value3",  attribs.get("attr3"));
    EXPECT_EQ("data",    attribs.get("subkey.attr"));
    
    const jet::ConfigNode subkey(attribs.getNode("subkey"));
    EXPECT_EQ("appName:1.attribs.subkey", subkey.name());
    EXPECT_EQ("data", subkey.get("attr"));
    
    CONFIG_ERROR(attribs.getNode("UNKNOWN"), "Config 'appName:1.attribs' doesn't have child 'UNKNOWN'");
    
}

TEST(Config, getChildrenOf)
{
    const jet::ConfigSource s1(
"<deployment>\n\
    <regions>\n\
        <UK>\n\
            <boxes>\n\
                <box hostname='UK1'/>\n\
                <box hostname='UK2'/>\n\
                <box hostname='UK3'/>\n\
            </boxes>\n\
        </UK>\n\
        <US>\n\
            <boxes>\n\
                <box hostname='US1'/>\n\
                <box hostname='US2'/>\n\
            </boxes>\n\
        </US>\n\
    </regions>\n\
</deployment>\n",
        "s1");
    jet::Config deployment("deployment");
    deployment << s1 << jet::lock;
    const std::vector<jet::ConfigNode> regions(deployment.getChildrenOf("regions"));
    EXPECT_EQ(2, regions.size());
    BOOST_FOREACH(const jet::ConfigNode& region, regions)
    {
        const std::string& regionName = region.nodeName();
        const std::vector<jet::ConfigNode> boxes(region.getChildrenOf("boxes"));
        if("US" == regionName)
            EXPECT_EQ(2, boxes.size());
        else
            EXPECT_EQ(3, boxes.size());
        unsigned index = 1;
        BOOST_FOREACH(const jet::ConfigNode& box, boxes)
        {
            const std::string expectedHostname(regionName + boost::lexical_cast<std::string>(index));
            EXPECT_EQ(expectedHostname, box.get<std::string>("hostname"));
            const std::string expectedBoxName("deployment.regions." + regionName + ".boxes.box");
            EXPECT_EQ(expectedBoxName, box.name());
            ++index;
        }
    }
}

TEST(Config, getChildrenOfRoot)
{
    const jet::ConfigSource s1(
"<deployment>\n\
    <UK attr='1'/>\n\
    <US attr='2'/>\n\
    <HK attr='23'/>\n\
</deployment>\n",
        "s1");
    jet::Config deployment("deployment");
    deployment << s1 << jet::lock;
    
    jet::ConfigNodes nodes(deployment.getChildrenOf());
    ASSERT_EQ(3, nodes.size());
    EXPECT_EQ("UK", nodes[0].nodeName());
    EXPECT_EQ("US", nodes[1].nodeName());
    EXPECT_EQ("HK", nodes[2].nodeName());
}

TEST(Config, getValueOfIntermidiateNode)
{
    const jet::ConfigSource s1(
"<deployment>\n\
    <UK attr='1'/>\n\
    <US attr='2'/>\n\
    <HK attr='23'/>\n\
</deployment>\n",
        "s1");
    jet::Config deployment("deployment");
    deployment << s1 << jet::lock;
    
    EXPECT_EQ("1", deployment.getNode("UK.attr").get());
    
    CONFIG_ERROR(deployment.getNode("UK").get(), "Node 'deployment.UK' is intermidiate node without value");
}



TEST(Config, repeatingNodeMergeWithoutConflicts)
{
    const jet::ConfigSource s1(
"<app>\n\
    <key attr='1'/>\n\
    <key attr='2'/>\n\
</app>\n",
        "s1");
    
    const jet::ConfigSource s2(
"<app>\n\
    <key1 attr='1'/>\n\
    <key1 attr='2'/>\n\
</app>\n",
        "s2");
    
    jet::Config config("app");
    
    config << s1 << s2 << jet::lock;
    
    std::stringstream strm;
    strm << config;

    const char* mergedConfig =
"<app>\n\
<key>\n\
  <attr>1</attr>\n\
</key>\n\
<key>\n\
  <attr>2</attr>\n\
</key>\n\
<key1>\n\
  <attr>1</attr>\n\
</key1>\n\
<key1>\n\
  <attr>2</attr>\n\
</key1>\n\
</app>\n";

    EXPECT_EQ(mergedConfig, strm.str());
}

TEST(Config, repeatingNodeMergeWithConflicts1)
{
    const jet::ConfigSource s1(
"<app>\n\
    <key attr='1'/>\n\
</app>\n",
        "s1");
    
    const jet::ConfigSource s2(
"<app>\n\
    <key attr='1'/>\n\
    <key attr='2'/>\n\
</app>\n",
        "s2");
    
    jet::Config config("app");
    
    config << s1;
    CONFIG_ERROR(config << s2, "Can't do ambiguous merge of node 'key' from config source 's2' to config 'app'");
}

TEST(Config, repeatingNodeMergeWithConflicts2)
{
    const jet::ConfigSource s1(
"<app>\n\
    <key attr='1'/>\n\
    <key attr='2'/>\n\
</app>\n",
        "s1");
    
    const jet::ConfigSource s2(
"<app>\n\
    <key attr='1'/>\n\
</app>\n",
        "s2");
    
    jet::Config config("app");
    
    config << s1;
    CONFIG_ERROR(config << s2, "Can't do ambiguous merge of node 'key' from config source 's2' to config 'app'");
}

//TODO: test xml comments
//TODO: (SourceConfig) prohibit '.' separator everywhere except application name
//TODO: add command line config source
//TODO: add environment config source
