// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class DeactivateCodePackageRequest : public Serialization::FabricSerializable
    {
    public:
        DeactivateCodePackageRequest();
        DeactivateCodePackageRequest(
            CodePackageInstanceIdentifier const & codePackageInstanceId,
            CodePackageActivationId const & activationId,
            Common::TimeSpan const timeout);

        __declspec(property(get = get_CodePackageInstanceId)) CodePackageInstanceIdentifier const & CodePackageInstanceId;
        inline CodePackageInstanceIdentifier const & get_CodePackageInstanceId() const { return codePackageInstanceId_; };

        __declspec(property(get=get_ActivationId)) CodePackageActivationId const & ActivationId;
        inline CodePackageActivationId const & get_ActivationId() const { return activationId_; };

        __declspec(property(get=get_Timeout)) Common::TimeSpan const & Timeout;
        inline Common::TimeSpan const & get_Timeout() const { return timeout_; };
        
        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        FABRIC_FIELDS_03(codePackageInstanceId_, activationId_, timeout_);

    private:
        CodePackageInstanceIdentifier codePackageInstanceId_;
        CodePackageActivationId activationId_;
        Common::TimeSpan timeout_;
    };
}
