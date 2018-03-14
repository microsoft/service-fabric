// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class CodePackageTerminationNotificationRequest : public Serialization::FabricSerializable
    {
    public:
        CodePackageTerminationNotificationRequest();
        CodePackageTerminationNotificationRequest(
            CodePackageInstanceIdentifier const & codePackageId, 
            CodePackageActivationId const & activationId);

        __declspec(property(get = get_CodePackageInstanceId)) CodePackageInstanceIdentifier const & CodePackageInstanceId;
        inline CodePackageInstanceIdentifier const & get_CodePackageInstanceId() const { return codePackageInstanceId_; };

        __declspec(property(get=get_ActivationId)) CodePackageActivationId const & ActivationId;
        inline CodePackageActivationId const & get_ActivationId() const { return activationId_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        FABRIC_FIELDS_02(codePackageInstanceId_, activationId_);

    private:
        CodePackageInstanceIdentifier codePackageInstanceId_;
        CodePackageActivationId activationId_;
    };
}
