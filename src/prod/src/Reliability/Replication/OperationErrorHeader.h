// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        class OperationErrorHeader
            : public Transport::MessageHeader<Transport::MessageHeaderId::OperationError>
            , public Serialization::FabricSerializable
        {
        public:
            OperationErrorHeader() {}
            OperationErrorHeader(int errorCodeValue) :
                errorCodeValue_(errorCodeValue)
            {
            }
            
            virtual ~OperationErrorHeader() {}

            __declspec(property(get=get_ErrorCodeValue)) int ErrorCodeValue;
            int get_ErrorCodeValue() const { return errorCodeValue_; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const 
            {
                w << errorCodeValue_;
            }

            FABRIC_FIELDS_01(errorCodeValue_);

        private:
            int errorCodeValue_;
                        
        }; // end class OperationErrorHeader

    } // end namespace ReplicationComponent
} // end namespace Reliability
