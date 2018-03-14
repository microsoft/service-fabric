// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class InvokeContainerApiRequest : public Serialization::FabricSerializable
    {
    public:
        InvokeContainerApiRequest();
        InvokeContainerApiRequest(
            ContainerApiExecutionArgs && apiExecArgs,
            int64 timeoutTicks);

        __declspec(property(get = get_ApiExecArgs)) ContainerApiExecutionArgs const & ApiExecArgs;
        ContainerApiExecutionArgs const & get_ApiExecArgs() const { return apiExecArgs_; }

        __declspec(property(get = get_TimeoutTicks)) int64 TimeoutInTicks;
        int64 get_TimeoutTicks() const { return timeoutTicks_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(apiExecArgs_, timeoutTicks_);

    private:
        ContainerApiExecutionArgs apiExecArgs_;
        int64 timeoutTicks_;
    };
}
