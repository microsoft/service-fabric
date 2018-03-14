// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace ServiceModel
{
    class InternalDeletedApplicationsQueryObject
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
    public:
        InternalDeletedApplicationsQueryObject();
        InternalDeletedApplicationsQueryObject(std::vector<std::wstring> const & applicationIds);
        InternalDeletedApplicationsQueryObject(InternalDeletedApplicationsQueryObject && other);

        void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        __declspec(property(get=get_ApplicationIds)) std::vector<std::wstring> const &ApplicationIds;
        std::vector<std::wstring> const& get_ApplicationIds() const { return applicationIds_; }

        InternalDeletedApplicationsQueryObject const & operator= (InternalDeletedApplicationsQueryObject && other);

        FABRIC_FIELDS_01(applicationIds_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::ApplicationIds, applicationIds_)
        END_JSON_SERIALIZABLE_PROPERTIES()     


    private:
        std::vector<std::wstring> applicationIds_;
    };
}
