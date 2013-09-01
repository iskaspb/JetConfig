//
//  ConfigSource.cpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 1/9/13.
//  Copyright (c) 2013 Alexey Tkachenko. All rights reserved.
//
#include "ConfigSourceImpl.hpp"
#include "ConfigError.hpp"
#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

namespace PT = boost::property_tree;

namespace jet
{

ConfigSource::Impl::Impl(std::istream& input, const std::string& name, ConfigSource::Format format):
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

ConfigSource::Impl::Impl(const std::string& filename, ConfigSource::Format format): name_(filename)
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

std::string ConfigSource::Impl::toString(const bool pretty) const
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

void ConfigSource::Impl::normalizeXmlTree(PT::ptree& rawTree)
{
    if (rawTree.empty())
        throw ConfigSource(str(
            boost::format("Config source '%1%' is empty") % name()));
    PT::ptree::value_type& child(rawTree.front());
    
    normalizeXmlTreeImpl(child.first, child.second);
}

void ConfigSource::Impl::normalizeXmlTreeImpl(const PT::path& currentPath, PT::ptree& rawTree)
{
    const PT::ptree::iterator end(rawTree.end());
    for(PT::ptree::iterator iter = rawTree.begin(); end != iter;)
    {
        if("<xmlattr>" == iter->first)
        {
            copyUniqueChildren(currentPath, iter->second, rawTree);
            iter = rawTree.erase(iter);
        }
        else
        {
            normalizeXmlTreeImpl(currentPath/PT::path(iter->first), iter->second);
            ++iter;
        }
    }
}

void ConfigSource::Impl::copyUniqueChildren(const PT::path& currentPath, const PT::ptree& from, PT::ptree& to) const
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

void ConfigSource::Impl::validateTree(const PT::ptree& tree) const
{
    if (tree.empty())
        throw ConfigSource(str(
            boost::format("Config source '%1%' is empty") % name()));
    const PT::ptree::value_type& child(tree.front());
    
    validateTreeImpl(child.first, child.second);
}

void ConfigSource::Impl::validateTreeImpl(const PT::path& currentPath, const PT::ptree& tree) const
{
    if(!tree.empty() && !tree.data().empty())
        throw ConfigError(str(
            boost::format("Invalid element '%1%' in config '%2%' contains both value and child attributes") %
            currentPath.dump() %
            name()));
    BOOST_FOREACH(const PT::ptree::value_type& node, tree)
    {
        const PT::path nodePath(currentPath/PT::path(node.first));
        if(tree.count(node.first) > 1)
            throw ConfigError(str(
                boost::format("Duplicate definition of element '%1%' in config '%2%'") %
                    nodePath.dump() %
                    name()));
        validateTreeImpl(nodePath, node.second);
    }
}
    
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
                "Couldn't parse config '%1%'. Reason: %2%") %
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

}//namespace jet
