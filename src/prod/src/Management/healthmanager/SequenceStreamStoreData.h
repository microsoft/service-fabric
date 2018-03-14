// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        class SequenceStreamStoreData : public Store::StoreData
        {
        public:
            SequenceStreamStoreData();

            SequenceStreamStoreData(
                std::wstring const & sourceId,
                FABRIC_INSTANCE_ID instance,
                FABRIC_SEQUENCE_NUMBER upToLsn,
                FABRIC_SEQUENCE_NUMBER invalidateLsn,
                Common::DateTime invalidateTime,
                FABRIC_SEQUENCE_NUMBER highestLsn,
                Store::ReplicaActivityId const & activityId);

            SequenceStreamStoreData(SequenceStreamStoreData const & other);
            SequenceStreamStoreData & operator = (SequenceStreamStoreData const & other);

            SequenceStreamStoreData(SequenceStreamStoreData && other);
            SequenceStreamStoreData & operator = (SequenceStreamStoreData && other);

            virtual ~SequenceStreamStoreData();

            __declspec (property(get=get_SourceId)) std::wstring const & SourceId;
            std::wstring const & get_SourceId() const { return sourceId_; }

            __declspec (property(get=get_Instance)) FABRIC_INSTANCE_ID Instance;
            FABRIC_INSTANCE_ID get_Instance() const { return instance_; }

            __declspec (property(get=get_UpToLsn)) FABRIC_SEQUENCE_NUMBER UpToLsn;
            FABRIC_SEQUENCE_NUMBER get_UpToLsn() const { return upToLsn_; }

            __declspec(property(get=get_InvalidateLsn)) FABRIC_SEQUENCE_NUMBER InvalidateLsn;
            FABRIC_SEQUENCE_NUMBER get_InvalidateLsn() const { return invalidateLsn_; }

            __declspec(property(get=get_InvalidateTime, put = set_InvalidateTime)) Common::DateTime InvalidateTime;
            Common::DateTime get_InvalidateTime() const { return invalidateTime_; }
            void set_InvalidateTime(Common::DateTime value) { invalidateTime_ = value; }

            __declspec(property(get=get_HighestLsn, put = set_HighestLsn)) FABRIC_SEQUENCE_NUMBER HighestLsn;
            FABRIC_SEQUENCE_NUMBER get_HighestLsn() const { return highestLsn_; }
            void set_HighestLsn(FABRIC_SEQUENCE_NUMBER value) { highestLsn_ = value; }

            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            virtual void WriteToEtw(uint16 contextSequenceId) const;

            FABRIC_FIELDS_05(sourceId_, instance_, upToLsn_, invalidateLsn_, invalidateTime_);

        protected:

            // Unique identifier of the reporting component.
            // Text provided by the reporting component to identify itself
            std::wstring sourceId_;

            // Instance for the stream
            FABRIC_INSTANCE_ID instance_;

            // The last number in the range that is committed (exclusive)
            FABRIC_SEQUENCE_NUMBER upToLsn_;

            // LSN lower than this should be invalidated
            FABRIC_SEQUENCE_NUMBER invalidateLsn_;

            // The time invalidateLsn_ takes effect
            Common::DateTime invalidateTime_;

            // Highest sequence ever accepted after loading (not necessarily persisted)
            FABRIC_SEQUENCE_NUMBER highestLsn_;
        };

        typedef std::unique_ptr<SequenceStreamStoreData> SequenceStreamStoreDataUPtr;
    }
}
