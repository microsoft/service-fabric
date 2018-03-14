// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class GenerationProposalReplyMessageBody : public Serialization::FabricSerializable
    {
    public:
        GenerationProposalReplyMessageBody()
            : error_(Common::ErrorCodeValue::Success)
        {
        }

        GenerationProposalReplyMessageBody(
            GenerationNumber const & proposedGeneration,
            GenerationNumber const & currentGeneration)
            :   proposedGeneration_(proposedGeneration),
                currentGeneration_(currentGeneration),
                error_(Common::ErrorCodeValue::Success)
        {
        }

        __declspec (property(get = get_ErrorCode, put = set_ErrorCode)) Common::ErrorCode Error;
        Common::ErrorCode get_ErrorCode() const { return error_; }
        void set_ErrorCode(Common::ErrorCode error) { error_ = error; }

        __declspec (property(get = get_ProposedGeneration)) GenerationNumber const & ProposedGeneration;
        GenerationNumber const & get_ProposedGeneration() const { return proposedGeneration_; }

        __declspec (property(get=get_CurrentGeneration)) GenerationNumber const & CurrentGeneration;
        GenerationNumber const & get_CurrentGeneration() const { return currentGeneration_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const&) const
        {
            w.Write("{0}/{1} {2}", proposedGeneration_, currentGeneration_, error_);
        }

        FABRIC_FIELDS_03(proposedGeneration_, currentGeneration_, error_);

    private:
        GenerationNumber proposedGeneration_;
        GenerationNumber currentGeneration_;
        Common::ErrorCode error_;
    };
}
