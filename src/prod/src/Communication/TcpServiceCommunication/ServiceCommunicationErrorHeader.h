// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class ServiceCommunicationErrorHeader
            : public Transport::MessageHeader<Transport::MessageHeaderId::ServiceCommunicationError>,
            public Serialization::FabricSerializable
        {

        public:
            ServiceCommunicationErrorHeader(int errorCodeValue)
                :errorCodeValue_(errorCodeValue)
            {
            }
            ServiceCommunicationErrorHeader()
                :errorCodeValue_(Common::ErrorCodeValue::Success)
            {
            }

            virtual ~ServiceCommunicationErrorHeader() {};

            __declspec(property(get = get_ErrorCodeValue)) int ErrorCodeValue;
            int get_ErrorCodeValue() const { return errorCodeValue_; }

            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
            {
                w << errorCodeValue_;
            }

            FABRIC_FIELDS_01(errorCodeValue_);

        private:
            int errorCodeValue_;
        };
    }
}
