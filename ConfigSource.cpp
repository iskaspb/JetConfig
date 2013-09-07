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
typedef PT::ptree::const_iterator CIter;
typedef PT::ptree::assoc_iterator AssocIter;
typedef PT::ptree::const_assoc_iterator CAssocIter;
typedef PT::ptree Tree;
typedef PT::path Path;

namespace jet
{

namespace
{

class Validator: boost::noncopyable
{
public:
    Validator(const std::string& sourceName): sourceName_(sourceName) {}
    void ensureTreeDoesNotHaveDataAndAttributeNodes(const Tree& root) const
    {
        const Tree::value_type& child(root.front());
        
        ensureNodeDoesNotHaveDataAndAttribute(child.first, child.second);
    }
    void ensureNonEmptyTree(const Tree& tree) const
    {
        if (tree.empty())
            throw ConfigSource(str(
                boost::format("Config source '%1%' is empty") % sourceName_));
    }
    void ensureSingleRootTree(const Tree& tree) const
    {
        if(tree.size() > 1)
            throw ConfigSource(str(
                boost::format("Invalid config source '%1%'. Only one configuration root element is allowed") %
                sourceName_));
    }
    void ensureNoSharedNodeDuplicates(const Tree& root) const
    {
        const Tree::value_type& child(root.front());
        if(child.first != ROOT_NODE_NAME)
            return;
        if(child.second.count(SHARED_NODE_NAME) > 1)
            throw ConfigError(str(
                boost::format("Duplicate shared node in config source '%1%'") % sourceName_));
            
    }
    void ensureNoSharedSubnodeDuplicates(const Tree& root) const
    {
        const Tree* sharedNode = findSharedNode(root);
        if(!sharedNode)
            return;
        BOOST_FOREACH(const Tree::value_type& sharedChild, *sharedNode)
        {
            if(sharedNode->count(sharedChild.first) > 1)
                throw ConfigError(str(
                    boost::format("Duplicate shared node '%1%' in config source '%2%'") %
                    sharedChild.first %
                    sourceName_));
        }
    }
    void ensureNoAppNodeDuplicates(const Tree& root) const
    {
        const Tree::value_type& child(root.front());
        if(child.first != ROOT_NODE_NAME)
            return;
        BOOST_FOREACH(const Tree::value_type& grandChild, child.second)
        {
            if(child.second.count(grandChild.first) > 1)
                throw ConfigError(str(
                    boost::format("Duplicate node '%1%' in config source '%2%'") %
                    grandChild.first %
                    sourceName_));
        }
    }
    void ensureNoInstanceNodeDuplicates(const Tree& root) const
    {
        const Tree::value_type& child(root.front());
        if(child.first != ROOT_NODE_NAME)
            return ensureNoInstanceNodeDuplicatesImpl(child.first, child.second);

        BOOST_FOREACH(const Tree::value_type& grandChild, child.second)
        {
            ensureNoInstanceNodeDuplicatesImpl(grandChild.first, grandChild.second);
        }
    }
    void ensureNoInstanceSubnodeDuplicates(const Tree& root) const
    {
        const Tree::value_type& child(root.front());
        if(child.first != ROOT_NODE_NAME)
            return ensureNoInstanceSubnodeDuplicatesImpl(child.first, child.second);

        BOOST_FOREACH(const Tree::value_type& grandChild, child.second)
        {
            ensureNoInstanceSubnodeDuplicatesImpl(grandChild.first, grandChild.second);
        }
    }
private:
    void ensureNodeDoesNotHaveDataAndAttribute(const Path& currentPath, const Tree& tree) const
    {
        if(!tree.empty() && !tree.data().empty())
            throw ConfigError(str(
                boost::format("Invalid element '%1%' in config source '%2%' contains both value and child attributes") %
                currentPath.dump() %
                sourceName_));
        BOOST_FOREACH(const Tree::value_type& node, tree)
        {
            const Path nodePath(currentPath/Path(node.first));
            ensureNodeDoesNotHaveDataAndAttribute(nodePath, node.second);
        }
    }
    const Tree* findSharedNode(const Tree& root) const
    {
        const Tree::value_type& child(root.front());
        if(child.first == SHARED_NODE_NAME)
            return &child.second;
        if(child.first == ROOT_NODE_NAME)
        {
            const CAssocIter iter = child.second.find(SHARED_NODE_NAME);
            if(child.second.not_found() != iter)
                return &iter->second;
        }
        return 0;
    }
    void ensureNoInstanceNodeDuplicatesImpl(const std::string& appName, const Tree& appNode) const
    {
        if(SHARED_NODE_NAME == appName)//...in fact this is not an application node
            return;
        if(appNode.count(INSTANCE_NODE_NAME) > 1)
            throw ConfigError(str(
                boost::format("Duplicate " INSTANCE_NODE_NAME " node under '%1%' node in config source '%2%'") %
                appName %
                sourceName_));
    }
    void ensureNoInstanceSubnodeDuplicatesImpl(const std::string& appName, const Tree& appNode) const
    {
        if(SHARED_NODE_NAME == appName)//...in fact this is not an application node
            return;
        const CAssocIter instanceIter = appNode.find(INSTANCE_NODE_NAME);
        if(appNode.not_found() == instanceIter)
            return;
        BOOST_FOREACH(const Tree::value_type& instanceSubnode, instanceIter->second)
        {
            if(instanceIter->second.count(instanceSubnode.first) > 1)
            {
                std::string instanceName(appName);
                instanceName += INSTANCE_DELIMITER_CHAR;
                instanceName += instanceSubnode.first;
                throw ConfigError(str(
                    boost::format("Duplicate node '%1%' in config source '%2%'") %
                    instanceName %
                    sourceName_));
            }
        }
    }
    const std::string& sourceName_;
};

}//anonymous namespace

ConfigSource::Impl::Impl(
    std::istream& input,
    const std::string& name,
    ConfigSource::Format format,
    ConfigSource::FileNameStyle fileNameStyle):
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
    processRawTree(fileNameStyle);
}

ConfigSource::Impl::Impl(
    const std::string& filename,
    ConfigSource::Format format,
    ConfigSource::FileNameStyle fileNameStyle):
    name_(filename)
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
    processRawTree(fileNameStyle);
}

void ConfigSource::Impl::processRawTree(ConfigSource::FileNameStyle fileNameStyle)
{
    const Validator validator(name());
    validator.ensureNonEmptyTree(root_);
    validator.ensureSingleRootTree(root_);
    
    normalizeKeywords(root_, fileNameStyle);
    normalizeInstanceDelimiter(root_);
    
    validator.ensureNonEmptyTree(root_);
    validator.ensureTreeDoesNotHaveDataAndAttributeNodes(root_);
    validator.ensureNoSharedNodeDuplicates(root_);
    validator.ensureNoSharedSubnodeDuplicates(root_);
    validator.ensureNoAppNodeDuplicates(root_);
    validator.ensureNoInstanceNodeDuplicates(root_);
    validator.ensureNoInstanceSubnodeDuplicates(root_);
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

void ConfigSource::Impl::normalizeXmlTree(Tree& rawTree) const
{
    if (rawTree.empty())
        throw ConfigSource(str(
            boost::format("Config source '%1%' is empty") % name()));
    Tree::value_type& child(rawTree.front());
    
    normalizeXmlTreeImpl(child.first, child.second);
}

void ConfigSource::Impl::normalizeXmlTreeImpl(const Path& currentPath, Tree& rawTree) const
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

namespace
{
Iter renameNode(Tree& parent, const Iter& nodeIter, const std::string& newName)
{
    const Iter newNodeIter = parent.insert(nodeIter, Tree::value_type(newName, Tree()));
    nodeIter->second.swap(newNodeIter->second);
    parent.erase(nodeIter);
    return newNodeIter;
}
}//anonymous namespace

void ConfigSource::Impl::normalizeKeywords(
    Tree& root, ConfigSource::FileNameStyle fileNameStyle) const
{
    const std::string& rootName(root.front().first);
    if(ROOT_NODE_NAME == boost::to_lower_copy(rootName))
    {
        if(ROOT_NODE_NAME != rootName)
            renameNode(root, root.begin(), ROOT_NODE_NAME);
        Tree& configNode = root.front().second;
        for(Iter iter = configNode.begin(); configNode.end() != iter; ++iter)
        {
            iter = normalizeKeywordsImpl(configNode, iter, fileNameStyle);
        }
    }
    else
    {
        normalizeKeywordsImpl(root, root.begin(), fileNameStyle);
    }
}

Iter ConfigSource::Impl::normalizeKeywordsImpl(
    Tree& parent, const Iter& childIter, ConfigSource::FileNameStyle fileNameStyle) const
{
    if(SHARED_NODE_NAME == boost::to_lower_copy(childIter->first))
    {
        if(SHARED_NODE_NAME != childIter->first)
            return renameNode(parent, childIter, SHARED_NODE_NAME);
    }
    else
    {
        Tree& appNode = childIter->second;
        for(Iter iter = appNode.begin(); appNode.end() != iter; ++iter)
        {
            const std::string& nodeName = iter->first;
            if(INSTANCE_NODE_NAME == boost::to_lower_copy(nodeName))
            {
                if(INSTANCE_NODE_NAME != nodeName)
                    iter = renameNode(appNode, iter, INSTANCE_NODE_NAME);
            }
        }
        if(ConfigSource::CaseInsensitive == fileNameStyle)
        {
            const std::string lowCaseName = boost::to_lower_copy(childIter->first);
            if(childIter->first != lowCaseName)
                return renameNode(parent, childIter, lowCaseName);
        }
    }
    return childIter;
}

void ConfigSource::Impl::normalizeInstanceDelimiter(Tree& root) const
{
    const std::string& rootName(root.front().first);
    if(rootName == SHARED_NODE_NAME)
        return;
    if(rootName != ROOT_NODE_NAME)
    {
        normalizeInstanceDelimiterImpl(root, root.begin());
        return;
    }
    
    Tree& configNode(root.front().second);
    const Iter end(configNode.end());
    for(Iter iter(configNode.begin()); end != iter;)
    {
        iter = normalizeInstanceDelimiterImpl(configNode, iter);
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

Iter ConfigSource::Impl::normalizeInstanceDelimiterImpl(
    Tree& parent,
    const Iter& childIter) const
{
    const std::string& childName(childIter->first);
    const size_t pos = childName.find(INSTANCE_DELIMITER_CHAR);
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
            boost::format(
                "Invalid '"
                INSTANCE_DELIMITER_STR
                "' in element '%1%' in config source '%2%'. Expected format 'appName"
                INSTANCE_DELIMITER_STR
                "instanceName'") %
                childName %
                name()));
    if(boost::to_lower_copy(appName) == SHARED_NODE_NAME)
        throw ConfigError(str(
            boost::format("Shared node '%1%' can't have instance. Found in config source '%2%'") %
            childName %
            name()));
    const Iter newChildIter = findOrInsertChild(parent, childIter, appName);
    const Iter grandChildIter = findOrInsertChild(
        newChildIter->second, newChildIter->second.end(), INSTANCE_NODE_NAME);
    const AssocIter grandGrandChildIter = grandChildIter->second.find(instanceName);
    if(grandGrandChildIter != grandChildIter->second.not_found())
        throw ConfigError(str(
            boost::format("Duplicate node '%1%' in config source '%2%'") %
            childName %
            name()));
    childIter->second.swap(
        grandChildIter->second.push_back(
            Tree::value_type(instanceName, Tree()))->second);
    
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

ConfigSource::ConfigSource(
    const std::string& source,
    const std::string& name,
    Format format,
    FileNameStyle fileNameStyle) try
{
    std::stringstream strm;
    strm << source;
    impl_.reset(new Impl(strm, name, format, fileNameStyle));
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
    Format format,
    FileNameStyle fileNameStyle) try :
    impl_(new Impl(source, name, format, fileNameStyle))
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

ConfigSource ConfigSource::createFromFile(
    const std::string& filename, Format format, FileNameStyle fileNameStyle) try
{
    const boost::shared_ptr<Impl> impl(new Impl(filename, format, fileNameStyle));
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
