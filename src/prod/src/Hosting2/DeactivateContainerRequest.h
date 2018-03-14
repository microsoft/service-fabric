// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class DeactivateContainerRequest : public Serialization::FabricSerializable
    {
    public:
        DeactivateContainerRequest();
        DeactivateContainerRequest(
            ContainerDeactivationArgs && deactivationArgs,
            int64 timeoutTicks);

        __declspec(property(get = get_DeactivationArgs)) ContainerDeactivationArgs const & DeactivationArgs;
        ContainerDeactivationArgs const & get_DeactivationArgs() const { return deactivationArgs_; }

        __declspec(property(get = get_TimeoutTicks)) int64 TimeoutInTicks;
        int64 get_TimeoutTicks() const { return timeoutTicks_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(deactivationArgs_, timeoutTicks_);

    private:
        ContainerDeactivationArgs deactivationArgs_;
        int64 timeoutTicks_;
    };
}
