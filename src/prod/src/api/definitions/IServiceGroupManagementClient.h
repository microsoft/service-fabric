// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IServiceGroupManagementClient
    {
    public:
        virtual ~IServiceGroupManagementClient() {};

        virtual Common::AsyncOperationSPtr BeginCreateServiceGroup(
            ServiceModel::PartitionedServiceDescWrapper const &,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        
        virtual Common::ErrorCode EndCreateServiceGroup(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginCreateServiceGroupFromTemplate(
            ServiceModel::ServiceGroupFromTemplateDescription const & svcGroupFromTemplateDesc,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndCreateServiceGroupFromTemplate(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteServiceGroup(
            Common::NamingUri const &,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndDeleteServiceGroup(Common::AsyncOperationSPtr const &) = 0;

        virtual Common::AsyncOperationSPtr BeginGetServiceGroupDescription(
            Common::NamingUri const &,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;

        virtual Common::ErrorCode EndGetServiceGroupDescription(
            Common::AsyncOperationSPtr const &,
            __inout Naming::ServiceGroupDescriptor &) = 0; 

        virtual Common::AsyncOperationSPtr BeginUpdateServiceGroup(
            Common::NamingUri const & name,
            Naming::ServiceUpdateDescription const &,
            Common::TimeSpan const timeout, 
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const &) = 0;
        
        virtual Common::ErrorCode EndUpdateServiceGroup(Common::AsyncOperationSPtr const &) = 0;
    };
}
