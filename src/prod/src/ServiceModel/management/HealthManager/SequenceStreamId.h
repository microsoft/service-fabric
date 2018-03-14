// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace HealthManager
    {
        class SequenceStreamId
            : public Serialization::FabricSerializable
        {
        public:
            SequenceStreamId();

            SequenceStreamId(
                FABRIC_HEALTH_REPORT_KIND kind,
                std::wstring const & sourceId);

            SequenceStreamId(SequenceStreamId const & other);
            SequenceStreamId & operator = (SequenceStreamId const & other);

            SequenceStreamId(SequenceStreamId && other);
            SequenceStreamId & operator = (SequenceStreamId && other);

            bool operator == (SequenceStreamId const & other) const;
            bool operator != (SequenceStreamId const & other) const;
            bool operator < (SequenceStreamId const & other) const;
        
            __declspec(property(get=get_Kind)) FABRIC_HEALTH_REPORT_KIND Kind;
            FABRIC_HEALTH_REPORT_KIND get_Kind() const { return kind_; }

            __declspec (property(get=get_SourceId)) std::wstring const & SourceId;
            std::wstring const & get_SourceId() const { return sourceId_; }

            void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;
            std::wstring ToString() const;

            FABRIC_FIELDS_02(kind_, sourceId_);

        private:
            FABRIC_HEALTH_REPORT_KIND kind_;
            std::wstring sourceId_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::HealthManager::SequenceStreamId);
