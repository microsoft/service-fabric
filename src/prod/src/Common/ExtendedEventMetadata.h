// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Common
{
    class ExtendedEventMetadata
    {
        DENY_COPY(ExtendedEventMetadata);

    public:
        ExtendedEventMetadata(std::wstring const& publicEventName, std::wstring const& category);

        static std::unique_ptr<ExtendedEventMetadata> Create(std::wstring const& publicEventName, std::wstring const& category);

        static std::string AddMetadataFields(TraceEvent & traceEvent, size_t & fieldsCount);

        std::wstring const& GetPublicEventNameField() const
        {
            return publicEventName_;
        }

        std::wstring const& GetCategoryField() const
        {
            return category_;
        }

        void AppendFields(TraceEventContext & context) const;
        void AppendFields(StringWriter & writer) const;

    private:
        static StringLiteral const& GetMetadataFieldsFormat();

        std::wstring publicEventName_;
        std::wstring category_;
    };

}
