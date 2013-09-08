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
extern const ConfigLock lock;//...this is used to lock Config, that is to finish creation from ConfigSource. Efectively it makes Config immutable

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
    friend std::ostream& operator<<(std::ostream& os, const Config& config);

    std::string name() const;
    const std::string& appName() const;
    const std::string& instanceName() const;
    
    std::string get(const std::string& attrName) const;
    template<typename T>
    T get(const std::string& attrName) const;
    
    boost::optional<std::string> getOptional(const std::string& attrName) const;
    template<typename T>
    boost::optional<T> getOptional(const std::string& attrName) const;
    
    std::string get(const std::string& attrName, const std::string& defaultValue) const;
    template<typename T>
    T get(const std::string& attrName, const T& defaultValue) const;
private:
    void throwValueConversionError(const std::string& attrName, const std::string& value) const;
    class Impl;
    boost::shared_ptr<Impl> impl_;
};

template<typename T>
inline T Config::get(const std::string& attrName) const
{
    const std::string value(get(attrName));
    try
    {
        return boost::lexical_cast<T>(value);
    }
    catch(const boost::bad_lexical_cast&)
    {
        throwValueConversionError(attrName, value);
        throw;
    }
}

template<typename T>
inline boost::optional<T> Config::getOptional(const std::string& attrName) const
{
    const boost::optional<std::string> value(getOptional(attrName));
    if(!value)
        return boost::none;
    try
    {
        return boost::lexical_cast<T>(*value);
    }
    catch(const boost::bad_lexical_cast&)
    {
        throwValueConversionError(attrName, *value);
        throw;
    }
}

template<typename T>
inline T Config::get(const std::string& attrName, const T& defaultValue) const
{
    boost::optional<std::string> value(getOptional(attrName));
    if(!value)
        return defaultValue;
    try
    {
        return boost::lexical_cast<T>(*value);
    }
    catch(const boost::bad_lexical_cast&)
    {
        throwValueConversionError(attrName, *value);
        throw;
    }
}

extern std::ostream& operator<<(std::ostream& os, const Config& config);

}//namespace jet

#endif /*JetConfig_Config_hpp*/
