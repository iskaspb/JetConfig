//
//  ConfigSource.hpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 1/9/13.
//  This code belongs to public domain. You can do with it whatever you want without any guarantee.
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
    enum FileNameStyle { CaseSensitive, CaseInsensitive };
    //...
    explicit ConfigSource(
        const std::string& source,
        const std::string& name = "unknown",
        Format format = xml,
        FileNameStyle = CaseSensitive);
    explicit ConfigSource(
        std::istream& source,
        const std::string& name = "unknown",
        Format format = xml,
        FileNameStyle = CaseSensitive);
    ~ConfigSource();
    static ConfigSource createFromFile(
        const std::string& filename,
        Format format = xml,
        FileNameStyle = CaseSensitive);
    const std::string& name() const;
    std::string toString(OutputType outputType = Pretty) const;
private:
    boost::shared_ptr<Impl> impl_;
    friend class ConfigNode;
};

}//namespace jet

#endif /*JetConfig_ConfigSource_hpp*/
