// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

namespace ReliableMessaging
{
	// TODO: add compare to Common::Guid to avoid Guid copying here
	static LONG guidCompare(const Common::Guid& lhs, const Common::Guid& rhs)
	{
		::GUID first = lhs.AsGUID();
		::GUID second = rhs.AsGUID();
        return ::memcmp(&first, &second, sizeof(first));
	}

	template <class Comparator>
	static LONG simpleCompare(Comparator first, Comparator second)
	{
        return ::memcmp(&first, &second, sizeof(first));
	}

	class ReliableMessageId
	{
		friend class ReliableSessionService;

	public:
        __declspec (property(get=get_sessionId)) Common::Guid const &SessionId;
        inline Common::Guid const &get_sessionId() const { return sessionId_; }

        __declspec (property(get=get_sequenceNumber)) FABRIC_SEQUENCE_NUMBER const &SequenceNumber;
        inline FABRIC_SEQUENCE_NUMBER const &get_sequenceNumber() const { return sequenceNumber_; }

		ReliableMessageId() : sequenceNumber_(0)
		{
			memset(&sessionId_,0,sizeof(sessionId_));
		}

		ReliableMessageId(Common::Guid const &sessionId, FABRIC_SEQUENCE_NUMBER sequenceNumber)
			: sessionId_(sessionId), sequenceNumber_(sequenceNumber)
		{}

		static LONG compare(ReliableMessageId const &first, ReliableMessageId const &second)
		{
			LONG sessionIdcomparison = guidCompare(first.SessionId,second.SessionId);

			if (sessionIdcomparison != 0) return sessionIdcomparison;
			else return simpleCompare(first.SequenceNumber,second.SequenceNumber);
		}

		// needed for use in maps
		bool operator<(ReliableMessageId const & other) const
		{
			if (SessionId < other.SessionId) return true;
			else if (SessionId == other.SessionId && SequenceNumber < other.SequenceNumber) return true;
			else return false;
		}

		// needed for use in maps
		bool operator==(ReliableMessageId const & other) const
		{
			if (SessionId == other.SessionId && SequenceNumber == other.SequenceNumber) return true;
			else return false;
		}

	private:
		Common::Guid sessionId_;
		FABRIC_SEQUENCE_NUMBER sequenceNumber_;
	};

	class ProtocolAckKey
	{

	public:
        __declspec (property(get=get_sessionId)) Common::Guid const &SessionId;
        inline Common::Guid const &get_sessionId() const { return sessionId_; }

        __declspec (property(get=get_protocolCode)) ProtocolAction::Code const &ProtocolCode;
        inline ProtocolAction::Code const &get_protocolCode() const { return protocolCode_; }

		ProtocolAckKey(
			Common::Guid const &sessionId,
			ProtocolAction::Code protocolCode) : 
							sessionId_(sessionId),
							protocolCode_(protocolCode)
		{}


		static LONG compare(ProtocolAckKey const &first, ProtocolAckKey const &second)
		{
			LONG sessionIdcomparison = guidCompare(first.SessionId,second.SessionId);

			if (sessionIdcomparison != 0) return sessionIdcomparison;
			else return simpleCompare(first.ProtocolCode,second.ProtocolCode);
		}

		// needed for use in maps
		bool operator<(ProtocolAckKey const & other) const
		{
			if (SessionId < other.SessionId) return true;
			else if (SessionId == other.SessionId && ProtocolCode < other.ProtocolCode) return true;
			else return false;
		}

		// needed for use in maps
		bool operator==(ProtocolAckKey const & other) const
		{
			if (SessionId == other.SessionId && ProtocolCode == other.ProtocolCode) return true;
			else return false;
		}

	private:
		Common::Guid sessionId_;
		ProtocolAction::Code protocolCode_;
	};
} // end namespace ReliableMessaging

