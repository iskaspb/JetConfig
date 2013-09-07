//
//  ConfigSource.cpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 1/9/13.
//  This code belongs to public domain. You can do with it whatever you want without any guarantee.
//
#include "ConfigSourceImpl.hpp"
#include "ConfigError.hpp"
#include <boost/property_tree/exceptions.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/foreach.hpp>
#include <boost/format.hpp>

namespace PT = boost::property_tree;
typedef PT::ptree::value_type ValueType;
typedef PT::ptree::iterator Iter;
typedef PT::ptree::assoc_iterator AssocIter;
typedef PT::ptree Tree;
typedef PT::path Path;

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
    normalizeColon(root_);
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
    normalizeColon(root_);
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

void ConfigSource::Impl::normalizeXmlTree(Tree& rawTree)
{
    if (rawTree.empty())
        throw ConfigSource(str(
            boost::format("Config source '%1%' is empty") % name()));
    Tree::value_type& child(rawTree.front());
    
    normalizeXmlTreeImpl(child.first, child.second);
}

void ConfigSource::Impl::normalizeXmlTreeImpl(const Path& currentPath, Tree& rawTree)
{
    const Iter end(rawTree.end());
    for(Iter iter = rawTree.begin(); end != iter;)
    {
        if("<xmlattr>" == iter->first)
        {
            copyUniqueChildren(currentPath, iter->second, rawTree);
            iter = rawTree.erase(iter);
        }
        else
        {
            normalizeXmlTreeImpl(currentPath/Path(iter->first), iter->second);
            ++iter;
        }
    }
}

void ConfigSource::Impl::normalizeColon(Tree& root)
{
    //TODO: you can assume only one root node only for xml
    const std::string& rootName(root.front().first);
    if(boost::to_lower_copy(rootName) == SHARED_NODE_NAME)
        return;
    if(boost::to_lower_copy(rootName) != ROOT_NODE_NAME)
    {
        normalizeColonImpl(root, root.begin());
        return;
    }
    
    Tree& configNode(root.front().second);
    const Iter end(configNode.end());
    for(Iter iter(configNode.begin()); end != iter;)
    {
        iter = normalizeColonImpl(configNode, iter);
    }
}

namespace
{
inline Iter findOrInsertChild(Tree& parent, Iter insertPosition, const std::string& key)
{
    const AssocIter assocIter = parent.find(key);
    if(parent.not_found() != assocIter)
        return parent.to_iterator(assocIter);
    return parent.insert(insertPosition, ValueType(key, Tree()));
}
}//anonymous namespace

Iter ConfigSource::Impl::normalizeColonImpl(
    Tree& parent,
    const Iter& childIter)
{
    const std::string& childName(childIter->first);
    const size_t pos = childName.find(':');
    if(std::string::npos == pos)
    {
        Iter nextIter(childIter);
        std::advance(nextIter, 1);
        return nextIter;
    }
    const std::string appName(childName.substr(0, pos));
    const std::string instanceName(childName.substr(pos + 1));
    if(appName.empty() || instanceName.empty())
        throw ConfigError(str(
            boost::format("Invalid colon in element '%1%' in config source '%2%'. Expected format 'appName:instanceName'") %
                childName %
                name()));
    const Iter newChildIter = findOrInsertChild(parent, childIter, appName);
    const Iter grandChildIter = findOrInsertChild(newChildIter->second, newChildIter->second.end(), INSTANCE_NODE_NAME);
    const Iter grandGrandChildIter = findOrInsertChild(grandChildIter->second, grandChildIter->second.end(), instanceName);
    childIter->second.swap(grandGrandChildIter->second);
    
    return parent.erase(childIter);
}

void ConfigSource::Impl::copyUniqueChildren(const Path& currentPath, const Tree& from, Tree& to) const
{
    BOOST_REVERSE_FOREACH(const Tree::value_type& node, from)
    {
        if(from.count(node.first) > 1)
            throw ConfigError(str(
                boost::format("Duplicate definition of attribute '%1%' in config '%2%'") %
                    (currentPath/Path(node.first)).dump() %
                    name()));
        to.push_front(node);
    }
}

void ConfigSource::Impl::validateTree(const Tree& tree) const
{
    if (tree.empty())
        throw ConfigSource(str(
            boost::format("Config source '%1%' is empty") % name()));
    const Tree::value_type& child(tree.front());
    
    validateTreeImpl(child.first, child.second);
}

void ConfigSource::Impl::validateTreeImpl(const Path& currentPath, const Tree& tree) const
{
    if(!tree.empty() && !tree.data().empty())
        throw ConfigError(str(
            boost::format("Invalid element '%1%' in config '%2%' contains both value and child attributes") %
            currentPath.dump() %
            name()));
    BOOST_FOREACH(const Tree::value_type& node, tree)
    {
        const Path nodePath(currentPath/Path(node.first));
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
