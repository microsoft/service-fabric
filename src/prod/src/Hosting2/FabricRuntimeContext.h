// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class FabricRuntimeContext : public Serialization::FabricSerializable
    {
    public:
        FabricRuntimeContext();

        FabricRuntimeContext(
            FabricRuntimeContext const & other);
        
        FabricRuntimeContext(
            FabricRuntimeContext && other);

        FabricRuntimeContext(
            ApplicationHostContext const & hostContext,
            CodePackageContext const & codeContext);

        FabricRuntimeContext(
            std::wstring const & runtimeId,
            ApplicationHostContext const & hostContext,
            CodePackageContext const & codePackageContext);
        
        __declspec(property(get=get_RuntimeId)) std::wstring const & RuntimeId;
        inline std::wstring const & get_RuntimeId() const { return runtimeId_; };

        __declspec(property(get=get_HostContext)) ApplicationHostContext const & HostContext;
        inline ApplicationHostContext const & get_HostContext() const { return hostContext_; };

        __declspec(property(get=get_CodeContext, put=put_CodeContext)) CodePackageContext CodeContext;
        inline CodePackageContext const & get_CodeContext() const { return codeContext_; };
        inline void put_CodeContext(CodePackageContext const & value) { codeContext_ = value; };

        __declspec(property(get = get_ServicePackageActivationId)) ServicePackageInstanceIdentifier const & ServicePackageActivationId;
        inline ServicePackageInstanceIdentifier const & get_ServicePackageActivationId() const
        { 
            return codeContext_.CodePackageInstanceId.ServicePackageInstanceId; 
        }

        FabricRuntimeContext const & operator = (FabricRuntimeContext const & other);
        FabricRuntimeContext const & operator = (FabricRuntimeContext && other);

        bool operator == (FabricRuntimeContext const & other) const;
        bool operator != (FabricRuntimeContext const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        FABRIC_FIELDS_03(runtimeId_, hostContext_, codeContext_);

    private:
        std::wstring runtimeId_;
        ApplicationHostContext hostContext_;
        CodePackageContext codeContext_;
    };
}
