// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class EnumerateSubNamesRequest : public ServiceModel::ClientServerMessageBody
    {
    public:
        EnumerateSubNamesRequest()
            : name_()
            , continuationToken_()
            , doRecursive_(false)
        {
        }

        EnumerateSubNamesRequest(
            Common::NamingUri const & name,
            EnumerateSubNamesToken const & continuationToken,
            bool doRecursive = false)
            : name_(name)
            , continuationToken_(continuationToken)
            , doRecursive_(doRecursive)
        {
        }

        __declspec(property(get=get_NamingUri)) Common::NamingUri const & NamingUri;
        __declspec(property(get=get_ContinuationToken)) EnumerateSubNamesToken const & ContinuationToken;
        __declspec(property(get=get_DoRecursive)) bool DoRecursive;

        Common::NamingUri const & get_NamingUri() const { return name_; }
        EnumerateSubNamesToken const & get_ContinuationToken() const { return continuationToken_; }
        bool get_DoRecursive() const { return doRecursive_; }

        FABRIC_FIELDS_03(name_, continuationToken_, doRecursive_);

    private:
        Common::NamingUri name_;
        EnumerateSubNamesToken continuationToken_;
        bool doRecursive_;
    };
}
