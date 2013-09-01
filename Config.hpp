//
//  Config.hpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 26/8/13.
//  Copyright (c) 2013 Alexey Tkachenko. All rights reserved.
//

#ifndef JetConfig_Config_hpp
#define JetConfig_Config_hpp

#include "ConfigSource.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

namespace jet
{

struct ConfigLock {};

class Config
{
public:
    Config(
        const ConfigSource& source,
        const std::string& name);
    explicit Config(const std::string& name);
    Config(const Config& other);
    const Config& operator=(const Config& other);
    ~Config();

    const std::string& name() const;
    const std::string& appName() const;
    const std::string& instanceName() const;
    std::string get(const std::string& attrName) const;
    template<typename T>
    T get(const std::string& attrName) const
    {
        return boost::lexical_cast<T>(get(attrName));
    }
    Config& operator<<(const ConfigSource& source);
    void operator<<(ConfigLock);
private:
    class Impl;
    boost::shared_ptr<Impl> impl_;
};

const ConfigLock lock = {};

}//namespace jet

#endif /*JetConfig_Config_hpp*/
