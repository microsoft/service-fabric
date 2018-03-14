// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    class ApplicationTypeQueryContinuationToken
        : public Serialization::FabricSerializable
        , public Common::IFabricJsonSerializable
    {
        DENY_COPY(ApplicationTypeQueryContinuationToken)

    public:

        ApplicationTypeQueryContinuationToken();

        ApplicationTypeQueryContinuationToken(
            std::wstring const & applicationTypeName,
            std::wstring const & applicationTypeVersion);

        ApplicationTypeQueryContinuationToken(ApplicationTypeQueryContinuationToken && other) = default;
        ApplicationTypeQueryContinuationToken & operator =(ApplicationTypeQueryContinuationToken && other) = default;
        virtual ~ApplicationTypeQueryContinuationToken();

        __declspec(property(get = get_ApplicationTypeName)) std::wstring const & ApplicationTypeName;
        std::wstring const & get_ApplicationTypeName() const { return applicationTypeName_; }

        __declspec(property(get = get_ApplicationTypeVersion)) std::wstring const & ApplicationTypeVersion;
        std::wstring const & get_ApplicationTypeVersion() const { return applicationTypeVersion_; }

        bool IsValid() const;

        void TakeContinuationTokenComponents(__out std::wstring & typeName, __out std::wstring & typeVersion);

        FABRIC_FIELDS_02(
            applicationTypeName_,
            applicationTypeVersion_)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY(Constants::Name, applicationTypeName_)
            SERIALIZABLE_PROPERTY(Constants::Version, applicationTypeVersion_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        std::wstring applicationTypeName_;
        std::wstring applicationTypeVersion_;
    };
}
