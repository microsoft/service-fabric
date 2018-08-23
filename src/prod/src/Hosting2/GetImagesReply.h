//------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class GetImagesReply : public Serialization::FabricSerializable
    {
    public:
        GetImagesReply();
        GetImagesReply(
            std::vector<wstring> const & images,
            Common::ErrorCode error);

        __declspec(property(get = get_Images)) std::vector<wstring> const & Images;
        std::vector<wstring> const & get_Images() const { return images_; }

        __declspec(property(get = get_Error)) Common::ErrorCode const & Error;
        Common::ErrorCode const & get_Error() const { return error_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_02(images_, error_);

    private:
        std::vector<wstring> images_;
        Common::ErrorCode error_;
    };
}