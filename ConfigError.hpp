//
//  ConfigError.hpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 1/9/13.
//  Copyright (c) 2013 Alexey Tkachenko. All rights reserved.
//

#ifndef JetConfig_ConfigError_hpp
#define JetConfig_ConfigError_hpp

#include <exception>
#include <string>

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

}//namespace jet

#endif //JetConfig_ConfigError_hpp
