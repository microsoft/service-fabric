// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace ServiceModel
{
    namespace ModelV2
    {
        class ApplicationProperties
            : public ModelType
        {
        public:
            bool operator==(ApplicationProperties const &) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES()
                SERIALIZABLE_PROPERTY(L"description", description_)
                SERIALIZABLE_PROPERTY(L"debugParams", debugParams_)
                SERIALIZABLE_PROPERTY_IF(L"services", Services, (Services.size() != 0))
                SERIALIZABLE_PROPERTY(L"diagnostics", diagnostics_)
            END_JSON_SERIALIZABLE_PROPERTIES()

            FABRIC_FIELDS_04(description_, Services, debugParams_, diagnostics_);

            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(description_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(debugParams_)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(Services)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(diagnostics_)
            END_DYNAMIC_SIZE_ESTIMATION()

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            void WriteTo(Common::TextWriter &, Common::FormatOptions const &) const
            {
                return;
            }

            std::wstring description_;
            std::wstring debugParams_;
            std::vector<ContainerServiceDescription> Services;
            std::shared_ptr<DiagnosticsDescription2> diagnostics_;
        };

        class ApplicationDescription
            : public ResourceDescription
        {
        public:

            ApplicationDescription()
                : ResourceDescription(*ServiceModel::Constants::ModelV2ReservedDnsNameCharacters)
                , ApplicationUri()
                , Properties()
            {
            }

            bool operator==(ApplicationDescription const & other) const;

            __declspec(property(get=get_Description)) std::wstring const & Description;
            std::wstring const & get_Description() const { return Properties.description_; }

            __declspec(property(get=get_DebugParams)) std::wstring const & DebugParams;
            std::wstring const & get_DebugParams() const { return Properties.debugParams_;}

            __declspec(property(get=get_Services, put=put_Services)) std::vector<ContainerServiceDescription> & Services;
            std::vector<ContainerServiceDescription> & get_Services() { return Properties.Services; }
            void put_Services(std::vector<ContainerServiceDescription> const & value) { Properties.Services = value; }

            __declspec(property(get=get_Diagnostics)) std::shared_ptr<DiagnosticsDescription2> Diagnostics;
            std::shared_ptr<DiagnosticsDescription2> get_Diagnostics() const { return Properties.diagnostics_; }

            Common::ErrorCode TryValidate(std::wstring const & traceId) const override;

            bool CanUpgrade(ApplicationDescription & other);

            void WriteTo(__in Common::TextWriter &, Common::FormatOptions const &) const;

            BEGIN_JSON_SERIALIZABLE_PROPERTIES() 
                SERIALIZABLE_PROPERTY_CHAIN()
                SERIALIZABLE_PROPERTY(Constants::properties, Properties) 
            END_JSON_SERIALIZABLE_PROPERTIES() 

            BEGIN_DYNAMIC_SIZE_ESTIMATION() 
                DYNAMIC_SIZE_ESTIMATION_MEMBER(ApplicationUri)
                DYNAMIC_SIZE_ESTIMATION_MEMBER(Properties) 
            END_DYNAMIC_SIZE_ESTIMATION() 

            FABRIC_FIELDS_02(ApplicationUri, Properties) 

        public:
            Common::NamingUri ApplicationUri;
            ApplicationProperties Properties; 
        };
    }
}
