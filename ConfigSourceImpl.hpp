//
//  ConfigSourceImpl.hpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 1/9/13.
//  Copyright (c) 2013 Alexey Tkachenko. All rights reserved.
//

#ifndef JetConfig_ConfigSourceImpl_hpp
#define JetConfig_ConfigSourceImpl_hpp

#include "ConfigSource.hpp"
#include <boost/property_tree/ptree.hpp>

namespace jet
{

class ConfigSource::Impl: boost::noncopyable
{
public:
    Impl(std::istream& input, const std::string& name, ConfigSource::Format format);
    Impl(const std::string& filename, ConfigSource::Format format);
    std::string toString(const bool pretty) const;
    const std::string& name() const { return name_; }
    const boost::property_tree::ptree& getRoot() const { return root_; }
private:
    void normalizeXmlTree(boost::property_tree::ptree& rawTree);
    void normalizeXmlTreeImpl(
        const boost::property_tree::path& currentPath,
        boost::property_tree::ptree& rawTree);
    void copyUniqueChildren(
        const boost::property_tree::path& currentPath,
        const boost::property_tree::ptree& from,
        boost::property_tree::ptree& to) const;
    void validateTree(const boost::property_tree::ptree& tree) const;
    void validateTreeImpl(
        const boost::property_tree::path& currentPath,
        const boost::property_tree::ptree& tree) const;
    //...
    boost::property_tree::ptree root_;
    std::string name_;
};

}//namespace jet

#endif /*JetConfig_ConfigSourceImpl_hpp*/
