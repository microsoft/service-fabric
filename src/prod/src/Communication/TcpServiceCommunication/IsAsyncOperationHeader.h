// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Communication
{
    namespace TcpServiceCommunication
    {
        class IsAsyncOperationHeader : public Transport::MessageHeader<Transport::MessageHeaderId::IsAsyncOperationHeader>, public Serialization::FabricSerializable
        {

        public:
            IsAsyncOperationHeader(bool isAsyncOperation)
                :isAsync_(isAsyncOperation)
            {
            }
            IsAsyncOperationHeader()
            {
            }

            __declspec(property(get = get_IsAsyncOperation)) bool IsAsyncOperation;
            bool get_IsAsyncOperation() const { return isAsync_; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            FABRIC_FIELDS_01(isAsync_);

        private:
            bool isAsync_;
        };
    };

}
