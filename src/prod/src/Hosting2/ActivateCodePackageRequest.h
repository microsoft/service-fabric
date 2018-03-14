// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ActivateCodePackageRequest : public Serialization::FabricSerializable
    {
    public:
        ActivateCodePackageRequest();
        ActivateCodePackageRequest(
            CodePackageContext const & codeContext,
            CodePackageActivationId const & activationId,
            Common::TimeSpan const timeout);

        __declspec(property(get=get_CodeContext)) CodePackageContext const & CodeContext;
        inline CodePackageContext const & get_CodeContext() const { return codeContext_; };

        __declspec(property(get = get_CodePackageInstanceId)) CodePackageInstanceIdentifier const & CodePackageInstanceId;
        inline CodePackageInstanceIdentifier const & get_CodePackageInstanceId() const { return codeContext_.CodePackageInstanceId; };

        __declspec(property(get=get_ActivationId)) CodePackageActivationId const & ActivationId;
        inline CodePackageActivationId const & get_ActivationId() const { return activationId_; };

        __declspec(property(get=get_Timeout)) Common::TimeSpan const & Timeout;
        inline Common::TimeSpan const & get_Timeout() const { return timeout_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        FABRIC_FIELDS_03(codeContext_, activationId_, timeout_);

    private:
        CodePackageContext codeContext_;
        CodePackageActivationId activationId_;
        Common::TimeSpan timeout_;
    };
}
