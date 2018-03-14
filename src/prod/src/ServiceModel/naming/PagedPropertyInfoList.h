// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class PagedPropertyInfoList : public Common::IFabricJsonSerializable
    {
        DENY_COPY(PagedPropertyInfoList)

    public:
        PagedPropertyInfoList();

        PagedPropertyInfoList(
            EnumeratePropertiesResult && enumeratePropertiesResult,
            std::wstring && continuationToken,
            bool includeValues);

        __declspec(property(get=get_ContinuationToken)) std::wstring const & ContinuationToken;
        __declspec(property(get=get_IsConsistent)) bool IsConsistent;
        __declspec(property(get=get_Properties)) std::vector<PropertyInfo> const & Properties;

        std::wstring const & get_ContinuationToken() const { return continuationToken_; }
        bool get_IsConsistent() const { return isConsistent_; }
        std::vector<PropertyInfo> const & get_Properties() const { return properties_; }

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::ContinuationToken, continuationToken_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::IsConsistent, isConsistent_)
            SERIALIZABLE_PROPERTY(ServiceModel::Constants::Properties, properties_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring continuationToken_;
        bool isConsistent_;
        std::vector<PropertyInfo> properties_;
    };
}
