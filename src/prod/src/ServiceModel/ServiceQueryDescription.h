// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ServiceQueryDescription
    {
        DENY_COPY(ServiceQueryDescription)

    public:
        ServiceQueryDescription();

        ServiceQueryDescription(
            Common::NamingUri && applicationName,
            Common::NamingUri && serviceNameFilter,
            std::wstring && serviceTypeNameFilter,
            std::wstring && continuationToken);

        Common::ErrorCode FromPublicApi(FABRIC_SERVICE_QUERY_DESCRIPTION const & svcQueryDesc);

        __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
        __declspec(property(get=get_ServiceNameFilter)) Common::NamingUri const & ServiceNameFilter;
        __declspec(property(get=get_ServiceTypeNameFilter)) std::wstring const & ServiceTypeNameFilter;
        __declspec(property(get=get_ContinuationToken)) std::wstring const & ContinuationToken;

        Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
        Common::NamingUri const & get_ServiceNameFilter() const { return serviceNameFilter_; }
        std::wstring const & get_ServiceTypeNameFilter() const { return serviceTypeNameFilter_; }
        std::wstring const & get_ContinuationToken() const { return continuationToken_; }

    private:
        Common::NamingUri applicationName_;
        Common::NamingUri serviceNameFilter_;
        std::wstring serviceTypeNameFilter_;
        std::wstring continuationToken_;
    };
}
