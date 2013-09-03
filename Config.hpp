//
//  Config.hpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 26/8/13.
//  This code belongs to public domain. You can do with it whatever you want without any guarantee.
//

#ifndef JetConfig_Config_hpp
#define JetConfig_Config_hpp

#include "ConfigSource.hpp"
#include <boost/lexical_cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

namespace jet
{

struct ConfigLock {};

class Config
{
public:
    explicit Config(
        const std::string& name,
        const std::string& instanceName = std::string());
    Config(const Config& other);
    const Config& operator=(const Config& other);
    ~Config();

    Config& operator<<(const ConfigSource& source);
    void operator<<(ConfigLock);
    std::string toString() const;//TODO: finish

    std::string name() const;
    const std::string& appName() const;
    const std::string& instanceName() const;
    
    std::string get(const std::string& attrName) const;//TODO: get() without default value should raise exception
    template<typename T>
    T get(const std::string& attrName) const
    {//TODO: translate bad_lexical_cast into ConfigError
        return boost::lexical_cast<T>(get(attrName));
    }
    boost::optional<std::string> getOptional(const std::string& attrName) const;
    template<typename T>
    boost::optional<T> getOptional(const std::string& attrName) const
    {//TODO: translate bad_lexical_cast into ConfigError
        const boost::optional<std::string> value(getOptional(attrName));
        if(value)
            return boost::lexical_cast<T>(*value);
        return boost::none;
    }
    std::string get(const std::string& attrName, const std::string& defaultValue);//TODO: finish
    template<typename T>
    T get(const std::string& attrName, const T& defaultValue)
    {
        const std::string strDefaultValue(boost::lexical_cast<std::string>(defaultValue));//TODO: translate bad_lexical_cast into ConfigError
        return boost::lexical_cast<T>(get(attrName, strDefaultValue));//TODO: translate bad_lexical_cast into ConfigError
    }
private:
    class Impl;
    boost::shared_ptr<Impl> impl_;
};

extern const ConfigLock lock;

}//namespace jet

#endif /*JetConfig_Config_hpp*/
