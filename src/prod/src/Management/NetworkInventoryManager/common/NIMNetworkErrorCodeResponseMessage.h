// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace NetworkInventoryManager
    {
        // Message to request message loop test.
        class NetworkErrorCodeResponseMessage : public Serialization::FabricSerializable
        {
        public:
            NetworkErrorCodeResponseMessage()
            {}

            NetworkErrorCodeResponseMessage(
                uint64 sequenceNumber,
                const Common::ErrorCode & errorCode)
                : sequenceNumber_(sequenceNumber),
                errorCode_(errorCode)
            {
            }
            
            __declspec(property(get=get_SequenceNumber)) uint64 SequenceNumber;
            uint64 get_SequenceNumber() const { return sequenceNumber_; }

            __declspec(property(get = get_ErrorCode, put = set_ErrorCode)) Common::ErrorCode ErrorCode;
            Common::ErrorCode get_ErrorCode() const { return errorCode_; }
            void set_ErrorCode(const Common::ErrorCode value) { errorCode_ = value; }

            void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

            static const std::wstring ActionName;

            FABRIC_FIELDS_02(sequenceNumber_, errorCode_);

        private:
            uint64 sequenceNumber_;
            Common::ErrorCode errorCode_;
        };
    }
}

