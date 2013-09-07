//
//  ConfigSourceImpl.hpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 1/9/13.
//  This code belongs to public domain. You can do with it whatever you want without any guarantee.
//

#ifndef JetConfig_ConfigSourceImpl_hpp
#define JetConfig_ConfigSourceImpl_hpp

#include "ConfigSource.hpp"
#include <boost/property_tree/ptree.hpp>

#define ROOT_NODE_NAME     "config"
#define SHARED_NODE_NAME   "shared"
#define INSTANCE_NODE_NAME "instance"
#define INSTANCE_DELIMITER_CHAR ':'
#define INSTANCE_DELIMITER_STR ":"

namespace jet
{

class ConfigSource::Impl: boost::noncopyable
{
public:
    Impl(
        std::istream& input,
        const std::string& name,
        ConfigSource::Format format,
        ConfigSource::FileNameStyle fileNameStyle);
    Impl(
        const std::string& filename,
        ConfigSource::Format format,
        ConfigSource::FileNameStyle fileNameStyle);
    std::string toString(const bool pretty) const;
    const std::string& name() const { return name_; }
    const boost::property_tree::ptree& getRoot() const { return root_; }
private:
    void processRawTree(ConfigSource::FileNameStyle fileNameStyle);
    void normalizeXmlTree(boost::property_tree::ptree& rawTree) const;
    void normalizeXmlTreeImpl(
        const boost::property_tree::path& currentPath,
        boost::property_tree::ptree& rawTree) const;
    void normalizeInstanceDelimiter(boost::property_tree::ptree& rawTree) const;
    boost::property_tree::ptree::iterator normalizeInstanceDelimiterImpl(
        boost::property_tree::ptree& parent,
        const boost::property_tree::ptree::iterator& child) const;
    void copyUniqueChildren(
        const boost::property_tree::path& currentPath,
        const boost::property_tree::ptree& from,
        boost::property_tree::ptree& to) const;
    void normalizeKeywords(
        boost::property_tree::ptree& tree,
        ConfigSource::FileNameStyle fileNameStyle) const;
    boost::property_tree::ptree::iterator normalizeKeywordsImpl(
        boost::property_tree::ptree& tree,
        const boost::property_tree::ptree::iterator& child,
        ConfigSource::FileNameStyle fileNameStyle) const;
    //...
    boost::property_tree::ptree root_;
    std::string name_;
};

}//namespace jet

#endif /*JetConfig_ConfigSourceImpl_hpp*/
