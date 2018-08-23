// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationTypeQueryDescription
    {
        DENY_COPY(ApplicationTypeQueryDescription)

    public:
        ApplicationTypeQueryDescription();

        ApplicationTypeQueryDescription(ApplicationTypeQueryDescription && other) = default;
        ApplicationTypeQueryDescription & operator = (ApplicationTypeQueryDescription && other) = default;

        virtual ~ApplicationTypeQueryDescription();

        Common::ErrorCode FromPublicApi(PAGED_FABRIC_APPLICATION_TYPE_QUERY_DESCRIPTION const & pagedAppTypeQueryDesc);

        __declspec(property(get = get_ApplicationTypeNameFilter, put = set_ApplicationTypeNameFilter)) std::wstring const & ApplicationTypeNameFilter;
        __declspec(property(get = get_ApplicationTypeVersionFilter, put = set_ApplicationTypeVersionFilter)) std::wstring const & ApplicationTypeVersionFilter;
        __declspec(property(get = get_ApplicationTypeDefinitionKindFilter, put = set_ApplicationTypeDefinitionKindFilter)) DWORD const ApplicationTypeDefinitionKindFilter;
        __declspec(property(get = get_ExcludeApplicationParameters, put = set_ExcludeApplicationParameters)) bool ExcludeApplicationParameters;
        __declspec(property(get = get_MaxResults, put = set_MaxResults)) int64 MaxResults;
        __declspec(property(get = get_ContinuationToken, put = set_ContinuationToken)) std::wstring const & ContinuationToken;

        std::wstring const & get_ApplicationTypeNameFilter() const { return applicationTypeNameFilter_; }
        std::wstring const & get_ApplicationTypeVersionFilter() const { return applicationTypeVersionFilter_; }
        DWORD const get_ApplicationTypeDefinitionKindFilter() const { return applicationTypeDefinitionKindFilter_; }
        bool get_ExcludeApplicationParameters() const { return excludeApplicationParameters_; }
        int64 get_MaxResults() const { return maxResults_; }
        std::wstring const & get_ContinuationToken() const { return continuationToken_; }

        void set_ExcludeApplicationParameters(bool const value) { excludeApplicationParameters_ = value; }
        void set_MaxResults(int64 const value) { maxResults_ = value; }

        void set_ApplicationTypeNameFilter(std::wstring && value) { applicationTypeNameFilter_ = move(value); }
        void set_ApplicationTypeVersionFilter(std::wstring && value) { applicationTypeVersionFilter_ = move(value); }
        void set_ApplicationTypeDefinitionKindFilter(DWORD const value) { applicationTypeDefinitionKindFilter_ = value; }
        void set_ContinuationToken(std::wstring && value) { continuationToken_ = move(value); }

    private:
        std::wstring applicationTypeNameFilter_;
        std::wstring applicationTypeVersionFilter_;
        DWORD applicationTypeDefinitionKindFilter_;
        bool excludeApplicationParameters_;
        int64 maxResults_;
        std::wstring continuationToken_;
    };
}
