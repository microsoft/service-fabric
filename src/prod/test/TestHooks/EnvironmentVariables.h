// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestHooks
{

#define START_GET_FABRIC_TEST_ENVIRONMENT_VARIABLE( EnvName ) \
{ \
    wstring envValue; \
    if (Common::Environment::GetEnvironmentVariableW(TestHooks::EnvironmentVariables::FabricTest_##EnvName, envValue, Common::NOTHROW())) \
    { \
        Trace.WriteInfo("TestHooks", "Read environment variable '{0}' = '{1}'", TestHooks::EnvironmentVariables::FabricTest_##EnvName, envValue); \

#define END_GET_FABRIC_TEST_ENVIRONMENT_VARIABLE( ConfigClass, ConfigName, EnvName, EnvValue ) \
        ConfigClass::GetConfig().ConfigName = EnvValue; \
    } \
    else \
    { \
        Trace.WriteInfo("TestHooks", "Environment variable '{0}' not found", TestHooks::EnvironmentVariables::FabricTest_##EnvName); \
    } \
} \

#define GET_FABRIC_TEST_BOOLEAN_ENV( EnvName, ConfigClass, ConfigName ) \
    START_GET_FABRIC_TEST_ENVIRONMENT_VARIABLE( EnvName ) \
    END_GET_FABRIC_TEST_ENVIRONMENT_VARIABLE( ConfigClass, ConfigName, EnvName, (envValue == L"true") ) \

#define GET_FABRIC_TEST_BOOL_TO_INT_ENV( EnvName, ConfigClass, ConfigName ) \
    START_GET_FABRIC_TEST_ENVIRONMENT_VARIABLE( EnvName ) \
    END_GET_FABRIC_TEST_ENVIRONMENT_VARIABLE( ConfigClass, ConfigName, EnvName, (envValue == L"true" ? 1 : 0) ) \

#define GET_FABRIC_TEST_INT_ENV( EnvName, ConfigClass, ConfigName ) \
    START_GET_FABRIC_TEST_ENVIRONMENT_VARIABLE( EnvName ) \
    END_GET_FABRIC_TEST_ENVIRONMENT_VARIABLE( ConfigClass, ConfigName, EnvName, Int32_Parse(envValue) ) \

    class EnvironmentVariables
    {
    public:
        static Common::GlobalWString FabricTest_EnableTStore;
        static Common::GlobalWString FabricTest_EnableHashSharedLogId;
        static Common::GlobalWString FabricTest_EnableSparseSharedLogs;
        static Common::GlobalWString FabricTest_SharedLogSize;
        static Common::GlobalWString FabricTest_SharedLogMaximumRecordSize;
    };
}
