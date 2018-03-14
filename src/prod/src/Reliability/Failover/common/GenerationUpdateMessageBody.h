// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
	//
	// This is only used during FM rebuild. The FMServiceEpoch is the epoch of the
	// FM Service primary doing the rebuild. When a node receives this, any old FM
	// Service replicas are dropped.
	//
    class GenerationUpdateMessageBody : public Serialization::FabricSerializable
    {
    public:

        GenerationUpdateMessageBody()
        {
		}
		
        explicit GenerationUpdateMessageBody(Reliability::Epoch fmServiceEpoch)
            : fmServiceEpoch_(fmServiceEpoch)
        {
        }

        __declspec (property(get=get_FMServiceEpoch)) Reliability::Epoch FMServiceEpoch;
        Reliability::Epoch get_FMServiceEpoch() const { return fmServiceEpoch_; }

        FABRIC_FIELDS_01(fmServiceEpoch_);

    private:

        Reliability::Epoch fmServiceEpoch_;
    };
}
