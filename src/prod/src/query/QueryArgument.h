// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Query
{
    class QueryArgument
    {
    public:
        explicit QueryArgument(std::wstring const & queryArgument, bool isRequired = true, std::wstring const & argumentSets = L"")
            : queryArgumentName_(queryArgument), isRequired_(isRequired), argumentSets_()
        {
            Common::StringUtility::Split<std::wstring>(argumentSets, argumentSets_, L",");
        }

        __declspec(property(get=get_Name)) std::wstring const & Name;
        std::wstring const & get_Name() const { return queryArgumentName_; }

        __declspec(property(get=get_IsRequired)) bool IsRequired;
        bool get_IsRequired() const { return isRequired_; }

        __declspec(property(get=get_ArgumentSets)) std::vector<std::wstring> const & ArgumentSets;
        std::vector<std::wstring> const & get_ArgumentSets() const { return argumentSets_; }

    private:
        std::wstring queryArgumentName_;
        bool isRequired_;
        std::vector<std::wstring> argumentSets_;
    };
}
