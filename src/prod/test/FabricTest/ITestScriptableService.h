// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace FabricTest
{
    class ITestScriptableService
    {
    public:
        
        virtual Common::ErrorCode ExecuteCommand(
            __in std::wstring const & command, 
            __in Common::StringCollection const & params,
            __out std::wstring & result) = 0;

        virtual std::wstring const & GetServiceLocation() = 0;

        virtual Federation::NodeId const & GetNodeId() = 0;

        virtual std::wstring const & GetServiceName() = 0;
    };

    typedef std::shared_ptr<ITestScriptableService> ITestScriptableServiceSPtr;
    typedef std::weak_ptr<ITestScriptableService> ITestScriptableServiceWPtr;

#define BEGIN_TEST_COMMAND_DISPATCH() \
    Common::ErrorCode ExecuteCommand( \
        __in std::wstring const & command,  \
        __in Common::StringCollection const & params, \
        __out std::wstring & result) \
    { \

#define TEST_COMMAND(methodName) \
    if (command.compare(L ## #methodName) == 0) \
    { \
       return methodName (params, result); \
    } else \

#define END_TEST_COMMAND_DISPATCH() \
    { \
        return ErrorCode(Common::ErrorCodeValue::InvalidOperation); \
    } \
    } \

}
