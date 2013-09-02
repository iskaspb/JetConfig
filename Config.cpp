//
//  Config.cpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 26/8/13.
//  This code belongs to public domain. You can do with it whatever you want without any guarantee.
//

#include "Config.hpp"
#include "ConfigSourceImpl.hpp"
#include "ConfigError.hpp"
#include <boost/property_tree/exceptions.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <sstream>

#include <boost/property_tree/xml_parser.hpp>
#include <iostream>
using std::cout;
using std::endl;

#define ROOT_NODE_NAME     "config"
#define SHARED_NODE_NAME   "shared"
#define INSTANCE_NODE_NAME "instance"

namespace PT = boost::property_tree;

namespace jet
{

const ConfigLock lock = {};

class Config::Impl
{
public:
    Impl(const std::string& name):
        locked_(false),
        name_(name)
    {
        const size_t dotPos = name_.find('.');
        appName_ = name_.substr(0, dotPos);
        if(appName_.empty())
            throw ConfigError("Empty config name");
        if(dotPos != std::string::npos)
        {
            instanceName_ = name_.substr(dotPos + 1);
            if(instanceName_.empty())
                throw ConfigError(str(
                    boost::format("Invalid config name '%1%'") % name_ ));
            config_.push_back(PT::ptree::value_type(instanceName_, PT::ptree()));
        }
        config_.push_back(PT::ptree::value_type(appName_, PT::ptree()));
        config_.push_back(PT::ptree::value_type(SHARED_NODE_NAME, PT::ptree()));
    }
    void merge(const ConfigSource::Impl& source)
    {
        if(locked_)
            throw ConfigError(str(boost::format("Config '%1%' is locked") % name()));
        const PT::ptree& root = source.getRoot();
        const std::string& otherConfigName(root.back().first);
        const PT::ptree& otherConfig(root.back().second);
        
        if(boost::to_lower_copy(otherConfigName) == ROOT_NODE_NAME)
        {
            validateNoData(otherConfig, source.name());
            {//...merge shared attributes (if found)
                const boost::optional<const PT::ptree&> sharedNode(
                    otherConfig.get_child_optional(SHARED_NODE_NAME));
                if(sharedNode)
                    mergeShared(*sharedNode, source.name());
            }
            if(!instanceName_.empty())
            {//...merge instance
                const PT::ptree::const_assoc_iterator instanceNode(
                    otherConfig.find(name()));
                if(instanceNode != otherConfig.not_found())
                {
                    //...validate for duplicate instance defintion
                    const boost::optional<const PT::ptree&> appNode(
                        otherConfig.get_child_optional(appName()));
                    if(appNode)
                    {
                        bool duplicateInstanceDefinition = false;
                        {
                            const PT::ptree::const_assoc_iterator dupInstanceNode(
                                appNode->find(
                                    INSTANCE_NODE_NAME "." + instanceName_));
                            duplicateInstanceDefinition = dupInstanceNode != appNode->not_found();
                        }
                        if(!duplicateInstanceDefinition)
                        {
                            const boost::optional<const PT::ptree&> dupInstanceNode(
                                appNode->get_child_optional(
                                    INSTANCE_NODE_NAME "." + instanceName_));
                            duplicateInstanceDefinition = dupInstanceNode;
                        }
                        if(duplicateInstanceDefinition)
                            throw ConfigError(str(
                                boost::format("Config source '%1%' has duplicate definition of config '%2%'") %
                                source.name() %
                                name()));
                    }
                    mergeInstance(instanceNode->second, source.name());
                    return;
                }
            }
            {//...merge application config without instance
                const boost::optional<const PT::ptree&> selfNode(
                    otherConfig.get_child_optional(appName()));
                if(selfNode)
                    mergeSelf(*selfNode, source.name());
            }
        }
        else if(boost::to_lower_copy(otherConfigName) == SHARED_NODE_NAME)
        {
            mergeShared(otherConfig, source.name());
        }
        else if(appName() == otherConfigName)
        {
            mergeSelf(otherConfig, source.name());
        }
        else if(name() == otherConfigName)
        {
            mergeInstance(otherConfig, source.name());
        }
        else
        {
            throw ConfigError(str(
                boost::format("Can't merge source '%1%' into config '%2%' because it has different config name '%3%'") %
                    source.name() %
                    name() %
                    otherConfigName));
        }
    }
    std::string get(const std::string& attrName) const
    {
        BOOST_FOREACH(const PT::ptree::value_type& node, config_)
        {
            const boost::optional<const PT::ptree&> attrNode(
                node.second.get_child_optional(attrName));
            if(attrNode)
                return attrNode->get_value<std::string>();
        }
        return std::string();
    }
    const std::string& name() const { return name_; }
    const std::string& appName() const { return appName_; }
    const std::string& instanceName() const { return instanceName_; }
    void lock() { locked_ = true; }
private:
    void mergeShared(const PT::ptree& from, const std::string& sourceName)
    {
        validateNoData(from, sourceName);
        validateShared(from, sourceName);
        assert(config_.back().first == SHARED_NODE_NAME);
        PT::ptree& shared(config_.back().second);
        merge(shared, from);
    }
    void validateShared(const PT::ptree& from, const std::string& sourceName)
    {
        BOOST_FOREACH(const PT::ptree::value_type& node, from)
        {
            const std::string& nodeName = node.first;
            if(boost::to_lower_copy(nodeName) == "instance")
                throw ConfigError(str(
                    boost::format("Subsecion name 'instance' is prohibited in 'shared' section. Config source '%1%'") %
                    sourceName));
            if(node.second.empty())
                throw ConfigError(str(
                    boost::format("Attribute '%1%' in 'shared' section of config source '%2%' must be defined under subsection") %
                    nodeName %
                    sourceName));
        }
    }
    void mergeSelf(const PT::ptree& from, const std::string& sourceName)
    {
        validateNoData(from, sourceName);
        if(instanceName_.empty())
        {
            assert(config_.size() == 2);
            assert(config_.front().first == name());
            PT::ptree& self(config_.front().second);
            merge(self, from);
        }
        else
        {
            assert(config_.size() == 3);
            PT::ptree::iterator iter(config_.begin());
            ++iter;
            assert(iter->first == appName());
            PT::ptree& self(iter->second);
            merge(self, from);
            bool hasInstanceNode = false;
            {
                const boost::optional<const PT::ptree&> instanceNode(
                    self.get_child_optional(
                        INSTANCE_NODE_NAME "." + instanceName_));
                if(instanceNode)
                {
                    hasInstanceNode = true;
                    mergeInstance(*instanceNode, sourceName);
                }
            }
            {
                const PT::ptree::const_assoc_iterator instanceNode(
                    self.find(
                        INSTANCE_NODE_NAME "." + instanceName_));
                if(instanceNode != self.not_found())
                {
                    if(hasInstanceNode)
                        throw ConfigError(str(
                            boost::format("Config source '%1%' has duplicate definition of config '%2%'") %
                            sourceName %
                            name()));
                    mergeInstance(instanceNode->second, sourceName);
                }
            }
        }
    }
    void mergeInstance(const PT::ptree& from, const std::string& sourceName)
    {
        validateNoData(from, sourceName);
        assert(!instanceName_.empty());
        assert(config_.size() == 3);
        assert(config_.front().first == instanceName());
        PT::ptree& instance(config_.front().second);
        merge(instance, from);
    }
    static void merge(PT::ptree& to, const PT::ptree& from)
    {
        BOOST_FOREACH(const PT::ptree::value_type& node, from)
        {
            const std::string& mergeName(node.first);
            const PT::ptree& mergeTree(node.second);
            const PT::ptree::assoc_iterator iter(to.find(mergeName));
            if(iter == to.not_found())
            {
                to.push_back(node);
            }
            else if(mergeTree.empty())
            {
                iter->second = mergeTree;
            }
            else
            {
                merge(iter->second, mergeTree);
            }
        }
    }
    void validateNoData(const PT::ptree& tree, const std::string& sourceName)
    {
        if(!tree.data().empty())
            throw ConfigError(str(
                boost::format("Invalid data node '%1%' in config '%2%' taken from config source '%3%'") %
                tree.data() %
                name() %
                sourceName));
    }
    //...
    bool locked_;
    std::string name_, appName_, instanceName_;//...name_ := appName_ ['.' instanceName_]
    PT::ptree config_;
};

Config::Config(const ConfigSource& source, const std::string& name):
    impl_(new Impl(name))
{
    impl_->merge(*source.impl_);
}

Config::Config(const std::string& name):
    impl_(new Impl(name))
{}

Config::Config(const Config& other):
    impl_(other.impl_)
{}

const Config& Config::operator=(const Config& other)
{
    if(impl_ == other.impl_)
        return *this;
    impl_ = other.impl_;
    return *this;
}

Config::~Config() {}

const std::string& Config::name() const
{
    return impl_->name();
}

const std::string& Config::appName() const
{
    return impl_->appName();
}

const std::string& Config::instanceName() const
{
    return impl_->instanceName();
}

std::string Config::get(const std::string& attrName) const
{
    return impl_->get(attrName);
}

Config& Config::operator<<(const ConfigSource& source)
{
    impl_->merge(*source.impl_);
    return *this;
}

void Config::operator<<(ConfigLock)
{
    impl_->lock();
}

} //namespace jet
