// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Context that keeps track of the sequence stream that needs to be persisted.
        // When the sequence stream is processed, it must notify the requestor.
        class SequenceStreamRequestContext 
            : public RequestContext
        {
            DENY_COPY(SequenceStreamRequestContext);

        public:
            SequenceStreamRequestContext(
                Store::ReplicaActivityId const & replicaActivityId,
                Common::AsyncOperationSPtr const & owner,
                __in ISequenceStreamRequestContextProcessor & requestProcessor,
                SequenceStreamInformation && sequenceStream);

            SequenceStreamRequestContext(SequenceStreamRequestContext && other);
            SequenceStreamRequestContext & operator = (SequenceStreamRequestContext && other);

            virtual ~SequenceStreamRequestContext();

            __declspec(property(get=get_SequenceStream)) SequenceStreamInformation const & SequenceStream;
            SequenceStreamInformation const & get_SequenceStream() const { return sequenceStream_; }
       
            __declspec(property(get=get_Kind)) FABRIC_HEALTH_REPORT_KIND Kind;
            FABRIC_HEALTH_REPORT_KIND get_Kind() const { return sequenceStream_.Kind; }

            __declspec(property(get=get_EntityKind)) HealthEntityKind::Enum EntityKind;
            HealthEntityKind::Enum get_EntityKind() const { return HealthEntityKind::FromHealthInformationKind(sequenceStream_.Kind); }

            virtual ServiceModel::Priority::Enum get_Priority() const override { return ServiceModel::Priority::Critical; }

            void OnRequestCompleted() override;

            virtual void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
            virtual void WriteToEtw(uint16 contextSequenceId) const;

        protected:
            virtual size_t EstimateSize() const override;

        private:
            SequenceStreamInformation sequenceStream_;

            // Keep a reference to the request context processor.
            // It is kept alive by the shared ptr above.
            ISequenceStreamRequestContextProcessor & requestProcessor_;
        };
    }
}
