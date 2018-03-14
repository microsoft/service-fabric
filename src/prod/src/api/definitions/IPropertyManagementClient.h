// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Api
{
    class IPropertyManagementClient
    {
    public:
        virtual ~IPropertyManagementClient() {};

        virtual Common::AsyncOperationSPtr BeginCreateName(
            Common::NamingUri const & name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndCreateName(Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteName(
            Common::NamingUri const & name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndDeleteName(Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginInternalDeleteName(
            Common::NamingUri const & name,
            bool isExplicit,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndInternalDeleteName(Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginNameExists(
            Common::NamingUri const & name,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndNameExists(
            Common::AsyncOperationSPtr const & operation, 
            __out bool & doesExist) = 0;

        virtual Common::AsyncOperationSPtr BeginEnumerateSubNames(
            Common::NamingUri const & name,
            Naming::EnumerateSubNamesToken const & continuationToken,
            bool doRecursive,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndEnumerateSubNames(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::EnumerateSubNamesResult & result) = 0;

        virtual Common::AsyncOperationSPtr BeginEnumerateProperties(
            Common::NamingUri const & name,
            bool includeValues,
            Naming::EnumeratePropertiesToken const & continuationToken,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndEnumerateProperties(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::EnumeratePropertiesResult & result) = 0;

        virtual Common::AsyncOperationSPtr BeginGetPropertyMetadata(
            Common::NamingUri const & name,
            std::wstring const & propertyName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndGetPropertyMetadata(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::NamePropertyMetadataResult & result) = 0;

        virtual Common::AsyncOperationSPtr BeginGetProperty(
            Common::NamingUri const & name,
            std::wstring const & propertyName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndGetProperty(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::NamePropertyResult & result) = 0;

        virtual Common::AsyncOperationSPtr BeginSubmitPropertyBatch(
            Naming::NamePropertyOperationBatch && batch,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0; 
        
        virtual Common::ErrorCode EndSubmitPropertyBatch(
            Common::AsyncOperationSPtr const & operation,
            __inout IPropertyBatchResultPtr & result) = 0;

        virtual Common::ErrorCode EndSubmitPropertyBatch(
            Common::AsyncOperationSPtr const & operation,
            __inout Naming::NamePropertyOperationBatchResult & result) = 0;

        virtual Common::AsyncOperationSPtr BeginDeleteProperty(
            Common::NamingUri const & name,
            std::wstring const & propertyName,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndDeleteProperty(Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginPutProperty(
            Common::NamingUri const & name,
            std::wstring const & propertyName,
            FABRIC_PROPERTY_TYPE_ID storageTypeId,
            Common::ByteBuffer &&data,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndPutProperty(Common::AsyncOperationSPtr const & operation) = 0;

        virtual Common::AsyncOperationSPtr BeginPutCustomProperty(
            Common::NamingUri const & name,
            std::wstring const & propertyName,
            FABRIC_PROPERTY_TYPE_ID storageTypeId,
            Common::ByteBuffer &&data,
            std::wstring const & customTypeId,
            Common::TimeSpan const timeout,
            Common::AsyncCallback const & callback,
            Common::AsyncOperationSPtr const & parent) = 0;

        virtual Common::ErrorCode EndPutCustomProperty(Common::AsyncOperationSPtr const & operation) = 0;
    };
}
