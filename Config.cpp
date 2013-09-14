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
inline std::string composeName(
    const std::string& appName,
    const std::string& instanceName = std::string(),
    const std::string& path = std::string())
{
    std::string res(appName);
    if(!instanceName.empty())
    {
        res += INSTANCE_DELIMITER_CHAR;
        res += instanceName;
    }
    if(!path.empty())
    {
        res += '.';//TODO: change this to configurable delimiter
        res += path;
    }
    return res;
}
}//anonymous namespace

const ConfigLock lock = {};

class ConfigNode::Impl: boost::noncopyable
{
public:
    Impl(const std::string& appName, const std::string& instanceName):
        appName_(appName),
        instanceName_(instanceName),
        isLocked_(false),
        config_(0)
    {
        if(appName_.empty())
            throw ConfigError("Empty config name");
        root_.push_back(ValueType(ROOT_NODE_NAME, Tree()));
        config_ = &root_.front().second;
        if(!instanceName_.empty())
            config_->push_back(ValueType(instanceName_, Tree()));
        config_->push_back(ValueType(appName_, Tree()));
        config_->push_back(ValueType(SHARED_NODE_NAME, Tree()));
    }
    const std::string& appName() const { return appName_; }
    const std::string& instanceName() const { return instanceName_; }
    void merge(const ConfigSource::Impl& source)
    {
        if(isLocked_)
            throw ConfigError(str(boost::format("Config '%1%' is locked") % name()));
        const Tree& otherConfig(source.getRoot().front().second);
        
        {//...merge shared attributes (if found)
            const CAssocIter sharedIter = otherConfig.find(SHARED_NODE_NAME);
            if(otherConfig.not_found() != sharedIter)
                merge(getSharedNode(), sharedIter->second);
        }
        const CAssocIter appIter = otherConfig.find(appName());
        if(appIter == otherConfig.not_found())
            return;
        merge(getSelfNode(), appIter->second);
        if(!instanceName().empty())
        {//...merge instance
            const CAssocIter instanceRootIter = appIter->second.find(INSTANCE_NODE_NAME);
            if(appIter->second.not_found() != instanceRootIter)
            {
                const CAssocIter instanceIter = instanceRootIter->second.find(instanceName());
                if(instanceRootIter->second.not_found() != instanceIter)
                {
                    merge(getInstanceNode(), instanceIter->second);
                }
            }
        }
    }
    void lock()
    {
        if(isLocked_)
            return;
        //...merge self node and (optionally) instance node into shared node
        merge(getSharedNode(), getSelfNode());
        if(!instanceName().empty())
            merge(getSharedNode(), getInstanceNode());
        //...swap self node and shared node to move result into self node
        getSharedNode().swap(getSelfNode());
        //...erase shared and (optionally) instance node
        config_->erase(SHARED_NODE_NAME);
        config_->erase(instanceName());
        isLocked_ = true;
    }
    const Tree& getConfigNode() const
    {
        if(!isLocked_)
            throw ConfigError(str(
                boost::format("Initialization of config '%1%' is not finished") % name()));
        return *config_;
    }
    void print(std::ostream& os) const
    {
        PT::write_xml(os, root_, PT::xml_writer_make_settings(' ', 2));
    }
    std::string name() const { return composeName(appName(), instanceName()); }
private:
    static void merge(Tree& to, const Tree& from)
    {
        BOOST_FOREACH(const ValueType& node, from)
        {
            const std::string& mergeName(node.first);
            if(INSTANCE_NODE_NAME == mergeName)
                continue;
            const Tree& mergeTree(node.second);
            const AssocIter iter(to.find(mergeName));
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
    Tree& getInstanceNode()
    {
        assert(!isLocked_);
        assert(!instanceName().empty());
        assert(config_->size() == 3);
        Tree& instance(config_->front().second);
        return instance;
    }
    Tree& getSelfNode()
    {
        assert(!isLocked_);
        if(instanceName().empty())
        {
            assert(config_->size() == 2);
            Tree& self(config_->front().second);
            return self;
        }
        else
        {
            assert(config_->size() == 3);
            Iter iter(config_->begin());
            ++iter;
            Tree& self(iter->second);
            return self;
        }
    }
    Tree& getSharedNode()
    {
        assert(!isLocked_);
        Tree& shared(config_->back().second);
        return shared;
    }
    //...
    const std::string appName_, instanceName_;
    bool isLocked_;
    Tree root_;
    Tree* config_;
};

ConfigNode::ConfigNode(const std::string& iappName, const std::string& iinstanceName):
    impl_(new Impl(iappName, iinstanceName)),
    treeNode_(0)
{}

ConfigNode::ConfigNode(
    const std::string& path,
    const boost::shared_ptr<Impl>& impl,
    const void* treeNode):
    path_(path),
    impl_(impl),
    treeNode_(treeNode)
{
}

ConfigNode::ConfigNode(const ConfigNode& other):
    path_(other.path_),
    impl_(other.impl_),
    treeNode_(other.treeNode_)
{
}

ConfigNode& ConfigNode::operator=(const ConfigNode& other)
{
    if(&other == this)
        return *this;
    impl_ = other.impl_;
    treeNode_ = other.treeNode_;
    return *this;
}

ConfigNode::~ConfigNode() {}

std::string ConfigNode::name() const { return composeName(appName(), instanceName(), path()); }

const std::string& ConfigNode::appName() const { return impl_->appName(); }

const std::string& ConfigNode::instanceName() const { return impl_->instanceName(); }

void ConfigNode::merge(const ConfigSource& source)
{
    impl_->merge(*source.impl_);
}

void ConfigNode::lock()
{
    impl_->lock();
    treeNode_ = static_cast<const Tree*>(&(impl_->getConfigNode().front().second));
}

void ConfigNode::print(std::ostream& os) const
{
    if(treeNode_)
    {
        os << '<' << name() << ">\n";
        {
            std::stringstream strm;
            PT::write_xml(
                strm,
                *static_cast<const Tree*>(treeNode_),
                PT::xml_writer_make_settings(' ', 2));
            std::string firstString;
            std::getline(strm, firstString);
            if(firstString.size() > 2 && //...get rid of the first line with <?xml ...?>
                '<' == firstString[0] && '?' == firstString[1])
                os << strm.str().substr(firstString.size() + 1);
            else
                os << strm.str();
        }
        os << "</" << name() << ">\n";
    }
    else
        impl_->print(os);
}


Config::Config(const std::string& appName, const std::string& instanceName):
    ConfigNode(appName, instanceName)
{
}

std::string ConfigNode::getImpl(const std::string& attrName) const
{
    boost::optional<std::string> value(getOptionalImpl(attrName));
    if(value)
        return *value;
    throw ConfigError(str(
        boost::format("Can't find property '%1%' in config '%2%'") %
        attrName %
        name()));
}

boost::optional<std::string> ConfigNode::getOptionalImpl(const std::string& attrName) const
{
    impl_->getConfigNode();//...just to check locked state
    const boost::optional<const Tree&> attrNode(
        static_cast<const Tree*>(treeNode_)->get_child_optional(attrName));
    if(attrNode)
        return attrNode->get_value<std::string>();
    return boost::none;
}

std::string ConfigNode::getImpl(const std::string& attrName, const std::string& defaultValue) const
{
    boost::optional<std::string> value(getOptionalImpl(attrName));
    if(value)
        return *value;
    return defaultValue;
}

ConfigNode ConfigNode::getChild(const std::string& path) const
{
    boost::optional<ConfigNode> optChild(getChildOptional(path));
    if(optChild)
        return *optChild;
    throw ConfigError(str(
        boost::format("Config '%1%' doesn't have child '%2%'") %
        name() %
        path));
}

boost::optional<ConfigNode> ConfigNode::getChildOptional(const std::string& path) const
{
    impl_->getConfigNode();//...just to check locked state
    const boost::optional<const Tree&> node(
        static_cast<const Tree*>(treeNode_)->get_child_optional(path));
    if(node)
    {
        std::string newPath(path_);
        if(!newPath.empty())
            newPath += '.';
        newPath += path;
        return ConfigNode(
            newPath,
            impl_,
            &(*node));
    }
    return boost::none;
}

std::vector<ConfigNode> ConfigNode::getChildren(const std::string& path) const
{
    impl_->getConfigNode();//...just to check locked state
    //BOOST_FOREACH(const ,
    //TODO: finish
    throw std::runtime_error("todo");
}

std::string ConfigNode::getValue() const
{
    impl_->getConfigNode();//...just to check locked state
    //TODO: finish
    throw std::runtime_error("todo");
}

std::ostream& operator<<(std::ostream& os, const ConfigNode& config)
{
    config.print(os);
    return os;
}

Config& Config::operator<<(const ConfigSource& source)
{
    merge(source);
    return *this;
}

void Config::operator<<(ConfigLock)
{
    lock();
}

void ConfigNode::throwValueConversionError(const std::string& attrName, const std::string& value) const
{
    throw ConfigError(str(
        boost::format("Can't convert value '%1%' of a property '%2%' in config '%3%'") %
        value %
        attrName %
        name()));
}

} //namespace jet
