// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Query
{
    class QueryAddressGenerator
        : public Common::TextTraceComponent<Common::TraceTaskCodes::Query>
    {
    public:        
        QueryAddressGenerator(
            std::string const & addressString, 
            std::wstring const & arg0 = L"",
            std::wstring const & arg1 = L"",
            std::wstring const & arg2 = L"",
            std::wstring const & arg3 = L"",
            std::wstring const & arg4 = L"",
            std::wstring const & arg5 = L"",
            std::wstring const & arg6 = L"",
            std::wstring const & arg7 = L"",
            std::wstring const & arg8 = L"",
            std::wstring const & arg9 = L"");
        
        Common::ErrorCode GenerateQueryAddress(
            Common::ActivityId const & activityId,
            ServiceModel::QueryArgumentMap const & queryArgs,
            _Out_ std::wstring & address);

        __declspec(property(get=get_AddressString)) std::string const & AddressString;
        std::string const & get_AddressString() const { return addressString_; }

    private:
        Common::ErrorCode ValidateAddressArguments(
            Common::ActivityId const & activityId,
            ServiceModel::QueryArgumentMap const & queryArgs);

        std::string addressString_;
        std::vector<std::wstring> queryArgValues_;
    };
}
