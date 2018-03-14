// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class DeactivateProcessRequest : public Serialization::FabricSerializable
    {
    public:
        DeactivateProcessRequest();
        DeactivateProcessRequest(
            std::wstring const & parentId,
            std::wstring const & appServiceId,
            int64 timeoutTicks);
            
        __declspec(property(get=get_ParentId)) std::wstring const & ParentId;
        std::wstring const & get_ParentId() const { return parentId_; }

        __declspec(property(get=get_AppServiceId)) std::wstring const & ApplicationServiceId;
        std::wstring const & get_AppServiceId() const { return appServiceId_; }

        __declspec(property(get=get_TimeoutTicks)) int64 TimeoutInTicks;
        int64 get_TimeoutTicks() const { return ticks_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_03(parentId_, appServiceId_, ticks_);

    private:
        std::wstring parentId_;
        std::wstring appServiceId_;
        int64 ticks_;
    };
}
