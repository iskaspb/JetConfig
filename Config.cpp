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
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <sstream>

//#include <iostream>
//using std::cout;
//using std::endl;

namespace PT = boost::property_tree;

namespace jet
{

const ConfigLock lock = {};

class Config::Impl
{
public:
    Impl(const std::string& appName, const std::string& instanceName):
        locked_(false),
        appName_(appName),
        instanceName_(instanceName)
    {
        if(appName_.empty())
            throw ConfigError("Empty config name");
        if(!instanceName_.empty())
            config_.push_back(PT::ptree::value_type(instanceName_, PT::ptree()));
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
        
        if(otherConfigName == ROOT_NODE_NAME)
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
                        const boost::optional<const PT::ptree&> dupInstanceNode(
                            appNode->get_child_optional(
                                INSTANCE_NODE_NAME "." + instanceName_));
                        if(dupInstanceNode)
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
        else if(otherConfigName == SHARED_NODE_NAME)
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
    boost::optional<std::string> get(const std::string& attrName) const
    {
        BOOST_FOREACH(const PT::ptree::value_type& node, config_)
        {
            const boost::optional<const PT::ptree&> attrNode(
                node.second.get_child_optional(attrName));
            if(attrNode)
                return attrNode->get_value<std::string>();
        }
        return boost::none;
    }
    std::string name() const
    {
        std::string res(appName_);
        if(!instanceName_.empty())
        {
            res += INSTANCE_DELIMITER_CHAR;
            res += instanceName_;
        }
        return res;
    }
    const std::string& appName() const { return appName_; }
    const std::string& instanceName() const { return instanceName_; }
    void lock() { locked_ = true; }
    void print(std::ostream& os) const
    {
        PT::write_xml(os, config_, PT::xml_writer_make_settings(' ', 2));
    }
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
            if(nodeName == INSTANCE_NODE_NAME)
                throw ConfigError(str(
                    boost::format("Subsecion name '" INSTANCE_NODE_NAME "' is prohibited in '" SHARED_NODE_NAME "' section. Config source '%1%'") %
                    sourceName));
            if(node.second.empty())
                throw ConfigError(str(
                    boost::format("Attribute '%1%' in '" SHARED_NODE_NAME "' section of config source '%2%' must be defined under subsection") %
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
            const boost::optional<const PT::ptree&> instanceNode(
                self.get_child_optional(
                    INSTANCE_NODE_NAME "." + instanceName_));
            if(instanceNode)
                mergeInstance(*instanceNode, sourceName);
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
    std::string appName_, instanceName_;//...name() := appName_ [':' instanceName_]
    PT::ptree config_;
};

Config::Config(const std::string& appName, const std::string& instanceName):
    impl_(new Impl(appName, instanceName))
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

std::string Config::name() const
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
    boost::optional<std::string> value(impl_->get(attrName));
    if(value)
        return *value;
    throw ConfigError(str(
        boost::format("Can't find property '%1%' in config '%2%'") %
        attrName %
        name()));
}

boost::optional<std::string> Config::getOptional(const std::string& attrName) const
{
    return impl_->get(attrName);
}

std::string Config::get(const std::string& attrName, const std::string& defaultValue) const
{
    boost::optional<std::string> value(impl_->get(attrName));
    if(value)
        return *value;
    return defaultValue;
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

void Config::throwValueConversionError(const std::string& attrName, const std::string& value) const
{
    throw ConfigError(str(
        boost::format("Can't convert value '%1%' of a property '%2%' in config '%3%'") %
        value %
        attrName %
        name()));
}

std::ostream& operator<<(std::ostream& os, const Config& config)
{
    config.impl_->print(os);
    return os;
}

} //namespace jet
