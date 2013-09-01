//
//  Config.cpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 26/8/13.
//  Copyright (c) 2013 Alexey Tkachenko. All rights reserved.
//

#include "Config.hpp"
#include "ConfigSourceImpl.hpp"
#include "ConfigError.hpp"
#include <boost/property_tree/exceptions.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <sstream>

#include <iostream>
using std::cout;
using std::endl;

#define SYSTEM_CONFIG_NAME "config"
#define INSTANCE_NODE_NAME "instance"

namespace PT = boost::property_tree;

namespace jet
{

class Config::Impl
{
public:
    explicit Impl(const std::string& name):
        type_(deriveTypeFromName(name)),
        name_(Config::System == type_? SYSTEM_CONFIG_NAME: name)
    {}
    void merge(const ConfigSource::Impl& source)
    {
        const PT::ptree& root = source.getRoot();
        if(root.empty())
            throw ConfigError(str(
                boost::format("Source '%1%' of config '%2%' is empty") %
                    source.name() %
                    (name().empty()? "unknown": name())));
        
        const std::string otherConfigName(root.back().first);
        const PT::ptree& otherConfig(root.back().second);
        if(name().empty())
        {
            type_ = deriveTypeFromName(otherConfigName);
            name_ = Config::System == type_? SYSTEM_CONFIG_NAME: otherConfigName;
        }
        else if(
            (Config::System != type_ && Config::Instance != type_) &&
            root.back().first != name())
            throw ConfigError(str(
                boost::format("Can't merge source '%1%' into config '%2%' because it has different config name '%3%'") %
                    source.name() %
                    name() %
                    otherConfigName));
        if(config_.empty())
            config_ = otherConfig;
        else
            mergeImpl(config_, otherConfig);
        validateConfig(source.name());
    }
    std::string get(const std::string& attrName) const
    {
        return config_.get<std::string>(attrName);
    }
    const std::string& name() const { return name_; }
    Config::Type type() const { return type_; }
private:
    static void mergeImpl(PT::ptree& to, const PT::ptree& from)
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
                mergeImpl(iter->second, mergeTree);
            }
        }
    }
    static Config::Type deriveTypeFromName(const std::string& name)
    {
        if(name.empty())
            return Config::Unknown;
        if(SYSTEM_CONFIG_NAME == boost::algorithm::to_lower_copy(name))
            return Config::System;
        return Config::Application;
    }
    void validateConfig(const std::string& sourceName) const
    {
        if(!config_.data().empty())
            throw ConfigError(str(
                boost::format("Invalid data node '%1%' in config '%2%' taken from config source '%3%'") %
                config_.data() %
                name() %
                sourceName));
    }
    //...
    Config::Type type_;
    std::string name_;
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

Config::~Config() {}

const std::string& Config::name() const
{
    return impl_->name();
}

Config::Type Config::type() const
{
    return impl_->type();
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

} //namespace jet
