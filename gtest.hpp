//
//  gtest.hpp
//  JetConfig
//
//  Created by Alexey Tkachenko on 1/9/13.
//  This code belongs to public domain. You can do with it whatever you want without any guarantee.
//
#ifndef JET_CONFIG_GTEST_HEADER_GUARD
#define JET_CONFIG_GTEST_HEADER_GUARD

#include <gtest/gtest.h>
#include <string>

#define EXPECT_EXCEPTION(EXPRESSION, EXCEPTION, MESSAGE) \
{ \
    const std::string message(MESSAGE); \
    try \
    { \
        do { EXPRESSION; } while(0); \
        FAIL() << "Negative test: exception " #EXCEPTION " is expected"; \
    } \
    catch(const EXCEPTION& ex) \
    { \
        const std::string exceptionMessage(std::string(ex.what()).substr(0, message.size())); \
        EXPECT_EQ(message, exceptionMessage); \
    } \
}

#define CONFIG_ERROR(EXPRESSION, MESSAGE) \
EXPECT_EXCEPTION(EXPRESSION, jet::ConfigError, MESSAGE);

//#define CONFIG_TEST(NAME)


#endif /*JET_CONFIG_GTEST_HEADER_GUARD*/
