// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    namespace ApiMonitoring
    {
        // Represents an Api Name when invoked
        // Example Interface = IStatefulServiceReplica, ApiName = ChangeRole, Metadata = Primary
        // Metadata can be empty
        // Provides a standard way for tracing/generating health event property names/instrumentation etc
        class ApiNameDescription : public Serialization::FabricSerializable
        {

        DEFAULT_COPY_CONSTRUCTOR(ApiNameDescription)
        DEFAULT_COPY_ASSIGNMENT(ApiNameDescription)

        public:
            ApiNameDescription() :
                interfaceName_(InterfaceName::Invalid),
                apiName_(ApiName::Invalid)
            {
            }

            ApiNameDescription(
                InterfaceName::Enum interfaceName,
                ApiName::Enum apiName,
                std::wstring metadata)
                : interfaceName_(interfaceName),
                  apiName_(apiName),
                  metadata_(std::move(metadata))
            {
            }

            ApiNameDescription(ApiNameDescription && other)
                : interfaceName_(std::move(other.interfaceName_)),
                  apiName_(std::move(other.apiName_)),
                  metadata_(std::move(other.metadata_))
            {
            }

            ApiNameDescription& operator=(ApiNameDescription && other)
            {
                if (this != &other)
                {
                    interfaceName_ = std::move(other.interfaceName_);
                    apiName_ = std::move(other.apiName_);
                    metadata_ = std::move(other.metadata_);
                }

                return *this;
            }

            __declspec(property(get = get_ApiName)) ApiName::Enum Api;
            ApiName::Enum get_ApiName() const { return apiName_; }

            __declspec(property(get = get_Interface)) InterfaceName::Enum Interface;
            InterfaceName::Enum get_Interface() const { return interfaceName_; }

            __declspec(property(get = get_Metadata)) std::wstring const & Metadata;
            std::wstring const & get_Metadata() const { return metadata_; }

            __declspec(property(get = get_IsInvalid)) bool IsInvalid;
            bool get_IsInvalid() const { return interfaceName_ == InterfaceName::Invalid; }

			__declspec(property(get = get_IsValid)) bool IsValid;
			bool get_IsValid() const { return interfaceName_ != InterfaceName::Invalid;; }

            static std::string AddField(Common::TraceEvent & traceEvent, std::string const & name);
            void FillEventData(Common::TraceEventContext & context) const;
            void WriteTo(Common::TextWriter & writer, Common::FormatOptions const &) const;
            std::wstring ToString() const;

            FABRIC_FIELDS_03(apiName_, interfaceName_, metadata_);

        private:
            ApiName::Enum apiName_;
            InterfaceName::Enum interfaceName_;
            std::wstring metadata_;
        };
    }
}


