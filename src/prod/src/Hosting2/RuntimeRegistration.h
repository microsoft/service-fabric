// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class RuntimeRegistration
    {
        DENY_COPY(RuntimeRegistration)

    public:
        RuntimeRegistration(FabricRuntimeContext const & runtimeContext);

        __declspec(property(get=get_RuntimeContext)) FabricRuntimeContext const& RuntimeContext;
        inline FabricRuntimeContext const& get_RuntimeContext() const {return runtimeContext_;};

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const;

    private:
        FabricRuntimeContext runtimeContext_;
    };
}
