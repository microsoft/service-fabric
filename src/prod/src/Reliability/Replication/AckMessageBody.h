// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class AckMessageBody : public Serialization::FabricSerializable
        {
        public:
            AckMessageBody() {}

            explicit AckMessageBody(FABRIC_SEQUENCE_NUMBER sequenceNumber)
                : sequenceNumber_(sequenceNumber)
            {
            }
            
            __declspec(property(get=get_SequenceNumber)) FABRIC_SEQUENCE_NUMBER const & SequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_SequenceNumber() const { return sequenceNumber_; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const 
            {
                w.Write("{0}", sequenceNumber_);
            }

            FABRIC_FIELDS_01(sequenceNumber_);

        private:
            FABRIC_SEQUENCE_NUMBER sequenceNumber_;
                        
        }; // end class AckMessageBody

    } // end namespace ReplicationComponent
} // end namespace Reliability
