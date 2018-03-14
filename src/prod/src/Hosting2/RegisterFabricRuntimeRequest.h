// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class RegisterFabricRuntimeRequest : public Serialization::FabricSerializable
    {
    public:
        RegisterFabricRuntimeRequest();
        RegisterFabricRuntimeRequest(FabricRuntimeContext const & runtimeContext);

        __declspec(property(get=get_RuntimeContext)) FabricRuntimeContext const & RuntimeContext;
        inline  FabricRuntimeContext const & get_RuntimeContext() const { return runtimeContext_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(runtimeContext_);

    private:
        FabricRuntimeContext runtimeContext_;
    };
}
