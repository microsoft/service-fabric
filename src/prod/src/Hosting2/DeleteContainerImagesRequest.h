// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{

    class DeleteContainerImagesRequest : public Serialization::FabricSerializable
    {
    public:
        DeleteContainerImagesRequest();
        DeleteContainerImagesRequest(
            std::vector<std::wstring> const & containerImages,
            int64 timeoutTicks);
            
        __declspec(property(get = get_ContainerImages)) std::vector<std::wstring> const & Images;
        std::vector<std::wstring> const & get_ContainerImages() const { return containerImages_; }

        __declspec(property(get = get_TimeoutTicks)) int64 TimeoutInTicks;
        int64 get_TimeoutTicks() const { return timeoutTicks_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;
        
        FABRIC_FIELDS_02(containerImages_, timeoutTicks_);

    private:
        std::vector<std::wstring> containerImages_;
        int64 timeoutTicks_;
    };
}
DEFINE_USER_ARRAY_UTILITY(Hosting2::DeleteContainerImagesRequest);
