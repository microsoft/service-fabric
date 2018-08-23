// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class SendAvailableContainerImagesEventArgs
    {
    public:
        SendAvailableContainerImagesEventArgs(
            std::vector<wstring> const & availableImages,
            std::wstring const & nodeId);

        SendAvailableContainerImagesEventArgs(SendAvailableContainerImagesEventArgs const & other);
        SendAvailableContainerImagesEventArgs(SendAvailableContainerImagesEventArgs && other);

        __declspec(property(get=get_AvailableImages)) std::vector<wstring> const & AvailableImages;
        inline std::vector<wstring> const & get_AvailableImages() const { return availableImages_; };

        __declspec(property(get=get_NodeId)) std::wstring const & NodeId;
        inline std::wstring const & get_NodeId() const { return nodeId_; };

        SendAvailableContainerImagesEventArgs const & operator = (SendAvailableContainerImagesEventArgs const & other);
        SendAvailableContainerImagesEventArgs const & operator = (SendAvailableContainerImagesEventArgs && other);

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

    private:
        std::vector<wstring> availableImages_;
        std::wstring nodeId_;
    };
}
