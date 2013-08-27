//
//  Config.cpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 26/8/13.
//  Copyright (c) 2013 Alexey Tkachenko. All rights reserved.
//

#include "Config.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
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

class ConfigSource::Impl
{
public:
    Impl(std::istream& input, const std::string& name, ConfigSource::Format format):
        name_(name)
    {
        switch (format)
        {
            case ConfigSource::xml:
                PT::read_xml(input, root_, PT::xml_parser::trim_whitespace);
                normalizeXmlTree(root_);
                break;
            default:
                ConfigError(
                    boost::str(
                        boost::format("Parsing of config format %1% is not implemented") %
                        format));
        }
        validateTree(root_);
    }
    Impl(const std::string& filename, ConfigSource::Format format): name_(filename)
    {
        switch (format)
        {
            case ConfigSource::xml:
                PT::read_xml(filename, root_, PT::xml_parser::trim_whitespace);
                normalizeXmlTree(root_);
                break;
            default:
                ConfigError(
                    boost::str(
                        boost::format("Parsing of config format %1% is not implemented") %
                        format));
        }
        validateTree(root_);
    }
    std::string toString(const bool pretty) const
    {
        std::stringstream strm;
        if(pretty)
            PT::write_xml(strm, root_, PT::xml_writer_make_settings(' ', 2));
        else
            PT::write_xml(strm, root_);
        std::string firstString;
        std::getline(strm, firstString);
        if(firstString.size() > 2 && //...get rid of the first line with <?xml ...?>
            '<' == firstString[0] && '?' == firstString[1])
            return strm.str().substr(firstString.size() + 1);
        return strm.str();
    }
    const std::string& name() const { return name_; }
    const PT::ptree& getRoot() const { return root_; }
private:
    void normalizeXmlTree(PT::ptree& rawTree, const PT::path& currentPath = PT::path())
    {
        const PT::ptree::iterator end(rawTree.end());
        for(PT::ptree::iterator iter = rawTree.begin(); end != iter;)
        {
            if("<xmlattr>" == iter->first)
            {
                copyUniqueChildren(iter->second, rawTree, currentPath);
                iter = rawTree.erase(iter);
            }
            else
            {
                normalizeXmlTree(iter->second, currentPath/PT::path(iter->first));
                ++iter;
            }
        }
    }
    void copyUniqueChildren(const PT::ptree& from, PT::ptree& to, const PT::path& currentPath) const
    {
        BOOST_REVERSE_FOREACH(const PT::ptree::value_type& node, from)
        {
            if(from.count(node.first) > 1)
                throw ConfigError(str(
                    boost::format("Duplicate definition of attribute '%1%' in config '%2%'") %
                        (currentPath/PT::path(node.first)).dump() %
                        name()));
            if(to.count(node.first))
                throw ConfigError(str(
                    boost::format("Ambiguous definition of attribute and element '%1%' in config '%2%'") %
                        (currentPath/PT::path(node.first)).dump() %
                        name()));
            to.push_front(node);
        }
    }
    void validateTree(const PT::ptree& tree, const PT::path& currentPath = PT::path()) const
    {
        BOOST_FOREACH(const PT::ptree::value_type& node, tree)
        {
            const PT::path nodePath(currentPath/PT::path(node.first));
            if(tree.count(node.first) > 1)
                throw ConfigError(str(
                    boost::format("Duplicate definition of element '%1%' in config '%2%'") %
                        nodePath.dump() %
                        name()));
            validateTree(node.second, nodePath);
        }
    }
    PT::ptree root_;
    std::string name_;
};

ConfigSource::ConfigSource(
    const std::string& source,
    const std::string& name,
    Format format) try
{
    std::stringstream strm;
    strm << source;
    impl_.reset(new Impl(strm, name, format));
}
catch(const PT::ptree_error& ex)
{
    const std::string msg(
        "Couldn't parse config '" +
        name +
        "'. Reason: " +
        ex.what());
    throw ConfigError(msg);
}

ConfigSource::ConfigSource(
    std::istream& source,
    const std::string& name,
    Format format) try :
    impl_(new Impl(source, name, format))
{
}
catch(const PT::ptree_error& ex)
{
    const std::string msg(
        std::string("Couldn't parse config. Reason: ") +
        ex.what());
    throw ConfigError(
        boost::str(
            boost::format("Couldn't parse config '%1%'. Reason: %2%") %
            name %
            ex.what()));
}

ConfigSource::ConfigSource(const boost::shared_ptr<Impl>& impl): impl_(impl) {}


ConfigSource::~ConfigSource() {}

ConfigSource ConfigSource::createFromFile(const std::string& filename, Format format) try
{
    const boost::shared_ptr<Impl> impl(new Impl(filename, format));
    return ConfigSource(impl);
}
catch(const PT::ptree_error& ex)
{
    throw ConfigError(
        boost::str(
            boost::format(
                "Couldn't parse config '%1%\'. Reason: %2%") %
                filename %
                ex.what()));
}

std::string ConfigSource::toString(OutputType outputType) const try
{
    return impl_->toString(Pretty == outputType);
}
catch(const PT::ptree_error& ex)
{
    throw ConfigSource(
        boost::str(
            boost::format(
                "Couldn't stringify config '%1%'. Reason: %2%") %
                impl_->name() %
                ex.what()));
}

class ProcessConfig::Impl
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
        config_ = root.back().second;
    }
    std::string get(const std::string& attrName) const
    {
        return config_.get<std::string>(attrName);
    }
    const std::string& name() const { return name_; }
private:
    std::string name_;
    PT::ptree config_;
};

ProcessConfig::ProcessConfig(const ConfigSource& source, const std::string& name):
    impl_(new Impl(name))
{
    impl_->merge(*source.impl_);
}

ProcessConfig::~ProcessConfig() {}

std::string ProcessConfig::name() const
{
    return impl_->name();
}

std::string ProcessConfig::get(const std::string& attrName) const
{
    return impl_->get(attrName);
}

} //namespace jet
