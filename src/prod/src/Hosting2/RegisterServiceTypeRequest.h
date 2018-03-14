// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class RegisterServiceTypeRequest : public Serialization::FabricSerializable
    {
    public:
        RegisterServiceTypeRequest();
        RegisterServiceTypeRequest(
            FabricRuntimeContext const & runtimeContext,
            ServiceTypeInstanceIdentifier const & serviceTypeInstanceId);

        __declspec(property(get=get_RuntimeContext)) FabricRuntimeContext const & RuntimeContext;
        inline FabricRuntimeContext const & get_RuntimeContext() const { return runtimeContext_; };

        __declspec(property(get = get_ServiceTypeInstanceId)) ServiceTypeInstanceIdentifier const & ServiceTypeInstanceId;
        inline ServiceTypeInstanceIdentifier const & get_ServiceTypeInstanceId() const { return serviceTypeInstanceId_; };

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(runtimeContext_, serviceTypeInstanceId_);

    private:
        FabricRuntimeContext runtimeContext_;
        ServiceTypeInstanceIdentifier serviceTypeInstanceId_;
    };
}
