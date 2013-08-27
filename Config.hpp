//
//  Config.hpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 26/8/13.
//  Copyright (c) 2013 Alexey Tkachenko. All rights reserved.
//

#ifndef JetConfig_Config_hpp
#define JetConfig_Config_hpp

#include <iosfwd>
#include <boost/shared_ptr.hpp>
#include <exception>

namespace jet
{

class ConfigError: public std::exception
{
public:
    explicit ConfigError(const std::string& msg): msg_(msg)
    {
        msg_.c_str();
    }
    ~ConfigError() throw() {}
    const char* what() const throw() { return msg_.c_str(); }
private:
    std::string msg_;
};

class ConfigSource
{
    class Impl;
    explicit ConfigSource(const boost::shared_ptr<Impl>& impl);
public:
    enum Format{ xml, json, linear };
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

class Config
{
};

}//namespace jet

#endif /*JetConfig_Config_hpp*/
