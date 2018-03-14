// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        // Used to populate trace message vector by SecondaryReplicatorTraceHandler
        class SecondaryReplicatorTraceInfo
        {
            DENY_COPY(SecondaryReplicatorTraceInfo);

        public:
            SecondaryReplicatorTraceInfo(SecondaryReplicatorTraceInfo && other);
            SecondaryReplicatorTraceInfo(
                __in FABRIC_SEQUENCE_NUMBER lowLSN,
                __in FABRIC_SEQUENCE_NUMBER highLSN);

            __declspec(property(get = get_LowLsn)) LONG64 LowLsn;
            LONG64 get_LowLsn() const { return lowLSN_; }

            __declspec(property(get = get_HighLsn)) LONG64 HighLsn;
            LONG64 get_HighLsn() const { return highLSN_; }

            // Update the element value
            void Update(
                __in FABRIC_SEQUENCE_NUMBER const & lowLSN,
                __in FABRIC_SEQUENCE_NUMBER const & highLSN);

            // Reset the element value
            // Values are reset after batch trace summary is generated
            void Reset();

            void WriteTo(Common::TextWriter&, Common::FormatOptions const &) const;
            void WriteToEtw(uint16 contextSequenceId) const;

        private:
            // Lowest and highest LSN received in batch operation
            // These can be the same when batch size is 1, trace output formatted accordingly
            FABRIC_SEQUENCE_NUMBER lowLSN_;
            FABRIC_SEQUENCE_NUMBER highLSN_;
            Common::DateTime receiveTime_;
        };
    } // end namespace ReplicationComponent
} // end namespace Reliability
