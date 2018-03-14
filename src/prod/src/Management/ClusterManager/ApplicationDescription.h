// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace ClusterManager
    {
        class ApplicationDescription : public Serialization::FabricSerializable
        {
        public:

            ApplicationDescription();
            
            ApplicationDescription(
                Common::NamingUri const &applicationName,
                std::wstring const & applicationTypeName,
                std::wstring const & applicationTypeVersion,
                std::map<std::wstring, std::wstring> const & applicationParameters);

            ApplicationDescription(ApplicationDescription &&);

            __declspec(property(get=get_ApplicationName)) Common::NamingUri const & ApplicationName;
            __declspec(property(get=get_ApplicationTypeName)) std::wstring const & ApplicationTypeName;
            __declspec(property(get=get_ApplicationTypeVersion)) std::wstring const & ApplicationTypeVersion;
            __declspec(property(get=get_ApplicationParameters)) std::map<std::wstring, std::wstring> const & ApplicationParameters;

            Common::NamingUri const & get_ApplicationName() const { return applicationName_; }
            std::wstring const & get_ApplicationTypeName() const { return applicationTypeName_; }
            std::wstring const & get_ApplicationTypeVersion() const { return applicationTypeVersion_; }
            std::map<std::wstring, std::wstring> const & get_ApplicationParameters() const { return applicationParameters_; }

            Common::ErrorCode FromPublicApi(FABRIC_APPLICATION_DESCRIPTION const &);
            void ToPublicApi(__in Common::ScopedHeap &, __out FABRIC_APPLICATION_DESCRIPTION &) const;

            bool operator == (ApplicationDescription const & other) const;
            bool operator != (ApplicationDescription const & other) const;

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;

        private:
            Common::NamingUri applicationName_;
            std::wstring applicationTypeName_;
            std::wstring applicationTypeVersion_;
            std::map<std::wstring, std::wstring> applicationParameters_;
        };
    }
}
