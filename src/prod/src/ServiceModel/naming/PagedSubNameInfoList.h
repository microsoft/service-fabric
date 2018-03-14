// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PagedSubNameInfoList : public EnumerateSubNamesResult
    {
        DENY_COPY(PagedSubNameInfoList)

    public:
        PagedSubNameInfoList();

        PagedSubNameInfoList(
            EnumerateSubNamesResult && enumerateSubNamesResult,
            std::wstring && continuationTokenString);

        __declspec(property(get=get_ContinuationTokenString)) std::wstring const & ContinuationTokenString;
        __declspec(property(get=get_IsConsistent)) bool const & IsConsistent;

        std::wstring const & get_ContinuationTokenString() const { return continuationTokenString_; }
        bool const & get_IsConsistent() const { return isConsistent_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ContinuationToken, continuationTokenString_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::IsConsistent, isConsistent_)
            SERIALIZABLE_PROPERTY_CHAIN()
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring continuationTokenString_;
        bool isConsistent_;
    };
}
