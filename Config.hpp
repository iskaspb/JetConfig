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
#include <vector>

namespace jet
{

class ConfigNode
{
    class Impl;
    ConfigNode(
        const std::string& path,
        const boost::shared_ptr<Impl>& impl,
        const void* treeNode);
public:
    ConfigNode(const ConfigNode& copee);
    ConfigNode& operator=(const ConfigNode& copee);
    virtual ~ConfigNode();
    std::string name() const;//...name := appName [ ':' instanceName ] [ '.' path ]
    const std::string& appName() const;
    const std::string& instanceName() const;
    const std::string& path() const { return path_; }
    const std::string nodeName() const; //...this is the last part of 'path', e.g. for path 'long.path.to.element' nodeName equal to 'element'
    
    ConfigNode getNode(const std::string& path) const;
    boost::optional<ConfigNode> getNodeOptional(const std::string& path) const;
    std::vector<ConfigNode> getChildrenOf(const std::string& path = std::string()) const;
    
    std::string get(const std::string& attrName = std::string()) const;
    template<typename T>
    T get(const std::string& attrName = std::string()) const;
    
    boost::optional<std::string> getOptional(const std::string& attrName = std::string()) const;
    template<typename T>
    boost::optional<T> getOptional(const std::string& attrName = std::string()) const;
    
    std::string get(const std::string& attrName, const std::string& defaultValue) const;
    template<typename T>
    T get(const std::string& attrName, const T& defaultValue) const;
protected:
    ConfigNode(const std::string& appName, const std::string& instanceName);
    void merge(const ConfigSource& source);
    void lock();
    void print(std::ostream& os) const;
private:
    void throwValueConversionError(const std::string& attrName, const std::string& value) const;
    friend std::ostream& operator<<(std::ostream& os, const ConfigNode& config);
    //...
    std::string path_;
    boost::shared_ptr<Impl> impl_;
    const void* treeNode_;
};

typedef std::vector<ConfigNode> ConfigNodes;

extern std::ostream& operator<<(std::ostream& os, const ConfigNode& config);

struct ConfigLock {};
extern const ConfigLock lock;//...this is used to lock Config, that is to finish creation from ConfigSource. Efectively it makes Config immutable

class Config: public ConfigNode
{
public:
    explicit Config(
        const std::string& appName,
        const std::string& instanceName = std::string());

    Config& operator<<(const ConfigSource& source);
    void operator<<(ConfigLock);
};

template<typename T>
inline T ConfigNode::get(const std::string& attrName) const
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
inline boost::optional<T> ConfigNode::getOptional(const std::string& attrName) const
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
inline T ConfigNode::get(const std::string& attrName, const T& defaultValue) const
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

}//namespace jet

#endif /*JetConfig_Config_hpp*/
