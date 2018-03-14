// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once 

namespace Management
{
    namespace HealthManager
    {
        class SequenceStreamInformation
            : public Serialization::FabricSerializable
            , public Common::ISizeEstimator
        {
        public:
            SequenceStreamInformation();

            SequenceStreamInformation(
                FABRIC_HEALTH_REPORT_KIND kind,
                std::wstring const & sourceId,
                FABRIC_INSTANCE_ID instance,
                FABRIC_SEQUENCE_NUMBER fromLsn,
                FABRIC_SEQUENCE_NUMBER upToLsn,
                uint64 reportCount,
                FABRIC_SEQUENCE_NUMBER invalidateLsn);

            SequenceStreamInformation(
                FABRIC_HEALTH_REPORT_KIND kind,
                std::wstring const & sourceId,
                FABRIC_INSTANCE_ID instance);

            virtual ~SequenceStreamInformation();

            SequenceStreamInformation(SequenceStreamInformation const & other);
            SequenceStreamInformation & operator = (SequenceStreamInformation const & other);

            SequenceStreamInformation(SequenceStreamInformation && other);
            SequenceStreamInformation & operator = (SequenceStreamInformation && other);
        
            __declspec(property(get=get_Kind)) FABRIC_HEALTH_REPORT_KIND Kind;
            FABRIC_HEALTH_REPORT_KIND get_Kind() const { return kind_; }

            __declspec (property(get=get_SourceId)) std::wstring const & SourceId;
            std::wstring const & get_SourceId() const { return sourceId_; }

            __declspec (property(get=get_Instance)) FABRIC_INSTANCE_ID Instance;
            FABRIC_INSTANCE_ID get_Instance() const { return instance_; }

            __declspec(property(get=get_FromLsn)) FABRIC_SEQUENCE_NUMBER FromLsn;
            FABRIC_SEQUENCE_NUMBER get_FromLsn() const { return fromLsn_; }

            __declspec(property(get=get_UpToLsn)) FABRIC_SEQUENCE_NUMBER UpToLsn;
            FABRIC_SEQUENCE_NUMBER get_UpToLsn() const { return upToLsn_; }

            __declspec(property(get=get_ReportCount)) uint64 ReportCount;
            uint64 get_ReportCount() const { return reportCount_; }

            __declspec(property(get=get_InvalidateLsn)) FABRIC_SEQUENCE_NUMBER InvalidateLsn;
            FABRIC_SEQUENCE_NUMBER get_InvalidateLsn() const { return invalidateLsn_; }

            bool IsHealthInformationRelevant(ServiceModel::HealthReport const & report) const { return kind_ == report.Kind && sourceId_ == report.SourceId; }

            bool IsForInstanceUpdate() const;

            void WriteTo(__in Common::TextWriter&, Common::FormatOptions const &) const;
            std::wstring ToString() const;

            FABRIC_FIELDS_07(kind_, sourceId_, instance_, fromLsn_, upToLsn_, reportCount_, invalidateLsn_);
            
            BEGIN_DYNAMIC_SIZE_ESTIMATION()
                DYNAMIC_SIZE_ESTIMATION_MEMBER(sourceId_)
                DYNAMIC_ENUM_ESTIMATION_MEMBER(kind_)
            END_DYNAMIC_SIZE_ESTIMATION()

        private:
            FABRIC_HEALTH_REPORT_KIND kind_;
            std::wstring sourceId_;
            FABRIC_INSTANCE_ID instance_;
            FABRIC_SEQUENCE_NUMBER fromLsn_;
            FABRIC_SEQUENCE_NUMBER upToLsn_;
            FABRIC_SEQUENCE_NUMBER invalidateLsn_;

            // The number of reports that are generated in this range [fromLsn, upToLsn)
            uint64 reportCount_;
        };
    }
}

DEFINE_USER_ARRAY_UTILITY(Management::HealthManager::SequenceStreamInformation);
