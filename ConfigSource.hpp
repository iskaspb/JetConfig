//
//  ConfigSource.hpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 1/9/13.
//  Copyright (c) 2013 Alexey Tkachenko. All rights reserved.
//

#ifndef JetConfig_ConfigSource_hpp
#define JetConfig_ConfigSource_hpp

#include <boost/shared_ptr.hpp>
#include <string>
#include <iosfwd>

namespace jet
{

class ConfigSource
{
    class Impl;
    explicit ConfigSource(const boost::shared_ptr<Impl>& impl);
public:
    enum Format{ xml, json };
    enum OutputType { Pretty, OneLine };
    explicit ConfigSource(
        const std::string& source,
        const std::string& name = "unknown",
        Format format = xml);
    explicit ConfigSource(
        std::istream& source,
        const std::string& name = "unknown",
        Format format = xml);
    ~ConfigSource();
    static ConfigSource createFromFile(const std::string& filename, Format format = xml);
    const std::string& name() const;
    std::string toString(OutputType outputType = Pretty) const;
private:
    boost::shared_ptr<Impl> impl_;
    friend class Config;
};

}//namespace jet

#endif /*JetConfig_ConfigSource_hpp*/