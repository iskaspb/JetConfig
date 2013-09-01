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
#include <boost/scoped_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>

namespace jet
{

class Config: boost::noncopyable
{
public:
    enum Type { Unknown, System, Application, Instance, Module };
    explicit Config(
        const ConfigSource& source, const std::string& name = std::string());
    explicit Config(const std::string& name = std::string());
    ~Config();
    const std::string& name() const;
    Type type() const;
    std::string get(const std::string& attrName) const;
    template<typename T>
    T get(const std::string& attrName) const
    {
        return boost::lexical_cast<T>(get(attrName));
    }
    Config& operator<<(const ConfigSource& source);
private:
    class Impl;
    boost::scoped_ptr<Impl> impl_;
};

}//namespace jet

#endif /*JetConfig_Config_hpp*/
