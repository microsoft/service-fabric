// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ConfigureSecurityPrincipalReply : public Serialization::FabricSerializable
    {
    public:
        ConfigureSecurityPrincipalReply();
        ConfigureSecurityPrincipalReply(
            std::vector<SecurityPrincipalInformationSPtr> const & principalsInfo,
            Common::ErrorCode const & error);

        __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        __declspec(property(get=get_PrincipalsInformation)) std::vector<SecurityPrincipalInformation> const & PrincipalInformation;
        std::vector<SecurityPrincipalInformation> const & get_PrincipalsInformation() const { return principalsInformation_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(error_, principalsInformation_);

    private:
        Common::ErrorCode error_;
        std::vector<SecurityPrincipalInformation> principalsInformation_;
    };
}
