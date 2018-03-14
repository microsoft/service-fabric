// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Hosting2
{

    class GetContainerInfoReply : public Serialization::FabricSerializable
    {
    public:
        GetContainerInfoReply();
        GetContainerInfoReply(
            std::wstring const & containerInfo,
            Common::ErrorCode error);            

        __declspec(property(get=get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        __declspec(property(get = get_ContainerInfo)) std::wstring const & ContainerInfo;
        std::wstring const & get_ContainerInfo() const { return containerInfo_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(containerInfo_, error_);

    private:
        std::wstring containerInfo_;
        Common::ErrorCode error_;
    };
}
