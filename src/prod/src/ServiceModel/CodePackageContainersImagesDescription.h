// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    struct CodePackageContainersImagesDescription : public Serialization::FabricSerializable
    {
    public:
        CodePackageContainersImagesDescription();
        CodePackageContainersImagesDescription(CodePackageContainersImagesDescription const & other);
        CodePackageContainersImagesDescription(CodePackageContainersImagesDescription && other);

        CodePackageContainersImagesDescription const & operator = (CodePackageContainersImagesDescription const & other);
        CodePackageContainersImagesDescription const & operator = (CodePackageContainersImagesDescription && other);

        bool operator == (CodePackageContainersImagesDescription const & other) const;
        bool operator != (CodePackageContainersImagesDescription const & other) const;

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        std::wstring ToString() const;

        void SetContainersImages(std::vector<std::wstring> const& containersImages);

        __declspec (property(get = get_ContainersImages, put = put_ContainersImages)) std::vector<std::wstring> ContainersImages;
        std::vector<std::wstring> get_ContainersImages() const { return containersImages_; }
        void put_ContainersImages(std::vector<std::wstring> containersImages) { containersImages_ = containersImages; }

        FABRIC_FIELDS_01(containersImages_);

    private:
        std::vector<std::wstring> containersImages_;
    };
}

DEFINE_USER_MAP_UTILITY(ServiceModel::ServicePackageIdentifier, ServiceModel::CodePackageContainersImagesDescription);
