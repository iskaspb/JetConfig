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
#include <boost/foreach.hpp>
#include <boost/format.hpp>
#include <sstream>

#include <iostream>
using std::cout;
using std::endl;

namespace PT = boost::property_tree;

namespace jet
{

class Config::Impl
{
public:
    explicit Impl(const std::string& name): name_(name) {}
    void merge(const ConfigSource::Impl& source)
    {
        const PT::ptree& root = source.getRoot();
        if(root.empty())
            throw ConfigError(str(
                boost::format("Source '%1%' of config '%2%' is empty") %
                    source.name() %
                    (name().empty()? "unknown": name())));
        if(name().empty())
            name_ = root.back().first;
        else if(root.back().first != name())
            throw ConfigError(str(
                boost::format("Can't merge source '%1%' into config '%2%' because it has different config name '%3%'") %
                    source.name() %
                    name() %
                    root.back().first));
        if(config_.empty())
            config_ = root.back().second;
        else
            mergeImpl(config_, root.back().second);
    }
    std::string get(const std::string& attrName) const
    {
        return config_.get<std::string>(attrName);
    }
    const std::string& name() const { return name_; }
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
