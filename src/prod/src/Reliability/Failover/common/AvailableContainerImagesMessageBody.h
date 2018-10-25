//----------------------------------------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

namespace Reliability
{
    class AvailableContainerImagesMessageBody : public Serialization::FabricSerializable
    {
        DENY_COPY(AvailableContainerImagesMessageBody);

    public:
        AvailableContainerImagesMessageBody()
        {
        }

        AvailableContainerImagesMessageBody(
            std::wstring const & nodeId,
            std::vector<wstring> const & availableImages)
            : nodeId_(nodeId),
            availableImages_(availableImages)
        {
        }

        __declspec (property(get = get_NodeId)) std::wstring const & NodeId;
        std::wstring const & get_NodeId() const { return nodeId_; }
        
        __declspec (property(get = get_AvailableImages)) std::vector<wstring> const & AvailableImages;
        std::vector<wstring> const & get_AvailableImages() const { return availableImages_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const
        {
            w.WriteLine("{0} {1}", nodeId_, availableImages_);
        }

        FABRIC_FIELDS_02(nodeId_, availableImages_);

    private:
        std::wstring nodeId_;
        std::vector<wstring> availableImages_;
    };
}