// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class ITestManagementClientInternal
    {
    public:
        virtual ~ITestManagementClientInternal() {};

        virtual Common::AsyncOperationSPtr BeginAddUnreliableLeaseBehavior(
            std::wstring const & nodeName,
            std::wstring const & alias,
            std::wstring const & behaviorString,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndAddUnreliableLeaseBehavior(
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginRemoveUnreliableLeaseBehavior(
            std::wstring const & nodeName,
            std::wstring const & alias,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndRemoveUnreliableLeaseBehavior(
            Common::AsyncOperationSPtr const &) = 0;
    };
}
