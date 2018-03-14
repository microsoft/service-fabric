// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace HealthManager
    {
        class SequenceStreamResult
            : public Serialization::FabricSerializable
        {
        public:
            SequenceStreamResult();

            SequenceStreamResult(
                FABRIC_HEALTH_REPORT_KIND kind,
                std::wstring const & sourceId);
            
            SequenceStreamResult(
                FABRIC_HEALTH_REPORT_KIND kind,
                std::wstring const & sourceId,
                FABRIC_INSTANCE_ID instance,
                FABRIC_SEQUENCE_NUMBER upToLsn);

            SequenceStreamResult(
                FABRIC_HEALTH_REPORT_KIND kind,
                std::wstring && sourceId,
                FABRIC_INSTANCE_ID instance,
                FABRIC_SEQUENCE_NUMBER upToLsn);

            SequenceStreamResult(SequenceStreamResult const & other);
            SequenceStreamResult & operator = (SequenceStreamResult const & other);

            SequenceStreamResult(SequenceStreamResult && other);
            SequenceStreamResult & operator = (SequenceStreamResult && other);
            
            __declspec(property(get=get_Kind)) FABRIC_HEALTH_REPORT_KIND Kind;
            FABRIC_HEALTH_REPORT_KIND get_Kind() const { return kind_; }

            __declspec (property(get=get_SourceId)) std::wstring const & SourceId;
            std::wstring const & get_SourceId() const { return sourceId_; }

            __declspec (property(get=get_Instance, put=set_Instance)) FABRIC_INSTANCE_ID Instance;
            FABRIC_INSTANCE_ID get_Instance() const { return instance_; }
            void set_Instance(FABRIC_INSTANCE_ID value) { instance_ = value; }
            
            __declspec(property(get=get_UpToLsn, put=set_UpToLsn)) FABRIC_SEQUENCE_NUMBER UpToLsn;
            FABRIC_SEQUENCE_NUMBER get_UpToLsn() const { return upToLsn_; }
            void set_UpToLsn(FABRIC_SEQUENCE_NUMBER value) { upToLsn_ = value; }

            void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;

            FABRIC_FIELDS_04(kind_, sourceId_, instance_, upToLsn_);

        private:
            FABRIC_HEALTH_REPORT_KIND kind_;
            std::wstring sourceId_;
            FABRIC_INSTANCE_ID instance_;
            FABRIC_SEQUENCE_NUMBER upToLsn_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::HealthManager::SequenceStreamResult);
