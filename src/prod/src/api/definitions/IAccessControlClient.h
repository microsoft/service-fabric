// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IAccessControlClient
    {
    public:
        virtual ~IAccessControlClient() {};

        virtual Common::AsyncOperationSPtr BeginSetAcl(
            AccessControl::ResourceIdentifier const &resource,
            AccessControl::FabricAcl const & acl,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndSetAcl(
            Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginGetAcl(
            AccessControl::ResourceIdentifier const &resource,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const &callback,
            Common::AsyncOperationSPtr const &parent) = 0;

        virtual Common::ErrorCode EndGetAcl(
            Common::AsyncOperationSPtr const & operation,
            _Out_ AccessControl::FabricAcl & result) = 0;
    };
}
