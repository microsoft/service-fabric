// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

using namespace Transport;

namespace ReliableMessaging
{
	class ReliableSessionParametersHeader : public Transport::MessageHeader<Transport::MessageHeaderId::ReliableMessagingSessionParams>,
		                                public Serialization::FabricSerializable
    {
	public:

        __declspec (property(get=get_windowSize)) ULONG WindowSize;
        inline ULONG get_windowSize() const { return windowSize_; }


		// default constructor needed for Headers.TryReadFirst functionality
		ReliableSessionParametersHeader() {}

        ReliableSessionParametersHeader(
				ULONG windowSize
				) 
				:   windowSize_(windowSize)
		{}

        FABRIC_FIELDS_01(windowSize_);

	private:

		LONG windowSize_;
	};

    class ReliableSessionHeader : public Transport::MessageHeader<Transport::MessageHeaderId::ReliableMessagingSession>,
		                                public Serialization::FabricSerializable
    {
    public:
		// default constructor needed for Headers.TryReadFirst functionality
		ReliableSessionHeader() {}

        ReliableSessionHeader(
				Common::Guid const &sessionId,
				ProtocolAction::Code actionCode,
				FABRIC_SEQUENCE_NUMBER sequenceNumber
				) 
				:   sessionId_(sessionId),
					actionCode_(actionCode),
					sequenceNumber_(sequenceNumber)
		{}

        ~ReliableSessionHeader() {}

        __declspec (property(get=get_SessionId)) Common::Guid const &SessionId;
        Common::Guid get_SessionId() const { return sessionId_; }

		__declspec (property(get=get_ActionCode)) ProtocolAction::Code const &ActionCode;
        ProtocolAction::Code get_ActionCode() const { return actionCode_; }

		__declspec (property(get=get_SequenceNumber)) FABRIC_SEQUENCE_NUMBER const &SequenceNumber;
        FABRIC_SEQUENCE_NUMBER get_SequenceNumber() const { return sequenceNumber_; }

        FABRIC_FIELDS_03(sessionId_, actionCode_, sequenceNumber_);

    private:
        Common::Guid sessionId_;
		ProtocolAction::Code actionCode_;
        FABRIC_SEQUENCE_NUMBER sequenceNumber_;
    };

	class ReliableMessagingProtocolResponseHeader : public Transport::MessageHeader<Transport::MessageHeaderId::ReliableMessagingProtocolResponse>,
		                                public Serialization::FabricSerializable
    {
    public:
		// default constructor needed for Headers.TryReadFirst functionality
		ReliableMessagingProtocolResponseHeader() {}

        ReliableMessagingProtocolResponseHeader(
				Common::ErrorCode response
				) 
				:   protocolResponse_(response.ReadValue())
		{}

 		__declspec (property(get=get_ProtocolResponse)) Common::ErrorCode const &ProtocolResponse;
        Common::ErrorCode get_ProtocolResponse() const { return protocolResponse_; }

       FABRIC_FIELDS_01(protocolResponse_);

	private:

		Common::ErrorCodeValue::Enum protocolResponse_;
	};


	template <Transport::MessageHeaderId::Enum kind>
    class ReliableSessionServicePartitionHeader : 
		public Transport::MessageHeader<kind>, 
		public Serialization::FabricSerializable
    {
    public:

		// default constructor needed for Headers.TryReadFirst functionality
		ReliableSessionServicePartitionHeader() : kind_(kind)
		{
			VerifyKind(kind);
		}

		// This constructor is for use by outbound protocol actions
		// Used with a "this" pointer in const functions so be careful with the const decls for the parameter 
		// Note: inbound transport deserialization directly initializes member variables, does not call any constructor
        ReliableSessionServicePartitionHeader(
			Common::Uri const &serviceInstanceName,
			ReliableMessaging::PartitionKey const &partitionKey
        ) 
        :   kind_(kind),
			serviceInstanceName_(serviceInstanceName),
			keyType_(partitionKey.KeyType)
		{
			VerifyKind(kind);

			integerRangeKey_.IntegerKeyLow = -1;
			integerRangeKey_.IntegerKeyHigh = -1;
			stringKey_ = L"";

			switch (keyType_)
			{
				case FABRIC_PARTITION_KEY_TYPE_INT64: integerRangeKey_ = partitionKey.Int64RangeKey; break;
				case FABRIC_PARTITION_KEY_TYPE_STRING: stringKey_ = partitionKey.StringKey; break;
			}
		}

        ~ReliableSessionServicePartitionHeader() {}

        __declspec (property(get=get_ServiceInstanceName)) Common::Uri const &ServiceInstanceName;
        Common::Uri get_ServiceInstanceName() const { return serviceInstanceName_; }

		__declspec (property(get=get_PartitionKey)) PartitionKey const &ServicePartitionKey;
        PartitionKey get_PartitionKey() const 
		{ 
			switch (keyType_)
			{
			case FABRIC_PARTITION_KEY_TYPE_NONE: return PartitionKey();
			case FABRIC_PARTITION_KEY_TYPE_INT64: return PartitionKey(integerRangeKey_);
			case FABRIC_PARTITION_KEY_TYPE_STRING: return PartitionKey(stringKey_);
			}						

			Common::Assert::CodingError("Invalid Partition Key in ReliableSessionServicePartitionHeader");
		}

        FABRIC_FIELDS_05(serviceInstanceName_, keyType_, integerRangeKey_.IntegerKeyLow, integerRangeKey_.IntegerKeyHigh , stringKey_);

    private:

		void VerifyKind(Transport::MessageHeaderId::Enum kind)
		{
			ASSERT_IF(kind != Transport::MessageHeaderId::ReliableMessagingSource &&
										kind != Transport::MessageHeaderId::ReliableMessagingTarget,
										"Unknown ReliableSessionServicePartitionHeader Kind Used in Template"
										);
		}

		// TODO: this should really be const but for now we don't want to preclude the assignment operator
		Transport::MessageHeaderId::Enum kind_;

        FABRIC_PARTITION_KEY_TYPE keyType_;
        INTEGER_PARTITION_KEY_RANGE integerRangeKey_;
        std::wstring stringKey_;    

        Common::Uri serviceInstanceName_;
	};


	typedef ReliableSessionServicePartitionHeader<Transport::MessageHeaderId::ReliableMessagingSource> ReliableMessagingSourceHeader;
	typedef ReliableSessionServicePartitionHeader<Transport::MessageHeaderId::ReliableMessagingTarget> ReliableMessagingTargetHeader;

	typedef std::shared_ptr<ReliableMessagingSourceHeader> ReliableMessagingSourceHeaderSPtr;
	typedef std::shared_ptr<ReliableMessagingTargetHeader> ReliableMessagingTargetHeaderSPtr;

} // end namespace Reliability


     
