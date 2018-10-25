//----------------------------------------------------------------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
//----------------------------------------------------------------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class NodeAvailableImage : public Common::IFabricJsonSerializable
    {
    public:
        NodeAvailableImage() = default;
        ~NodeAvailableImage() = default;

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(NodeAvailableImage::RepoTagsParameter, AvailableImages_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    public:
        std::vector<wstring> AvailableImages_;

        static Common::WStringLiteral const RepoTagsParameter;
    };
}