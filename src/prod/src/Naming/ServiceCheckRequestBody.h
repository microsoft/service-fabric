// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class ServiceCheckRequestBody : public Serialization::FabricSerializable
    {
    public:
        ServiceCheckRequestBody()
            : isPrefixMatch_(false)
            , name_()
        {
        }

        ServiceCheckRequestBody(
            bool isPrefixMatch,
            Common::NamingUri const & name)
            : isPrefixMatch_(isPrefixMatch)
            , name_(name)
        {
        }

        __declspec(property(get=get_IsPrefixMatch)) bool IsPrefixMatch;
        bool get_IsPrefixMatch() const { return isPrefixMatch_; }

        Common::NamingUri const & get_Name() const { return name_; }
        __declspec(property(get=get_Name)) Common::NamingUri const & Name;

        FABRIC_FIELDS_02(isPrefixMatch_, name_);

    private:
        bool isPrefixMatch_;
        Common::NamingUri name_;
    };
}
