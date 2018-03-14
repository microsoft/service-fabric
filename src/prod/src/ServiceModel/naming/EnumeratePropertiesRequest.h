// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EnumeratePropertiesRequest : public ServiceModel::ClientServerMessageBody
    {
    public:
        EnumeratePropertiesRequest()
            : name_()
            , includeValues_()
            , continuationToken_()
        {
        }

        EnumeratePropertiesRequest(
            Common::NamingUri const & name,
            bool includeValues,
            EnumeratePropertiesToken const & continuationToken)
            : name_(name)
            , includeValues_(includeValues)
            , continuationToken_(continuationToken)
        {
        }

        __declspec(property(get=get_NamingUri)) Common::NamingUri const & NamingUri;
        __declspec(property(get=get_IncludeValues)) bool IncludeValues;
        __declspec(property(get=get_ContinuationToken)) EnumeratePropertiesToken const & ContinuationToken;

        Common::NamingUri const & get_NamingUri() const { return name_; }
        bool get_IncludeValues() const { return includeValues_; }
        EnumeratePropertiesToken const & get_ContinuationToken() const { return continuationToken_; }

        FABRIC_FIELDS_03(name_, continuationToken_, includeValues_);

    private:
        Common::NamingUri name_;
        bool includeValues_;
        EnumeratePropertiesToken continuationToken_;
    };
}
