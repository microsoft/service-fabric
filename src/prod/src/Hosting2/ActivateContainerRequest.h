// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class ActivateContainerRequest : public Serialization::FabricSerializable
    {
    public:
        ActivateContainerRequest();
        ActivateContainerRequest(
            ContainerActivationArgs && activationArgs,
            int64 timeoutTicks);

        __declspec(property(get=get_ActivationArgs)) ContainerActivationArgs const & ActivationArgs;
        ContainerActivationArgs const & get_ActivationArgs() const { return activationArgs_; }

        __declspec(property(get=get_TimeoutTicks)) int64 TimeoutInTicks;
        int64 get_TimeoutTicks() const { return timeoutTicks_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(activationArgs_, timeoutTicks_);

    private:
        ContainerActivationArgs activationArgs_;
        int64 timeoutTicks_;
    };
}
