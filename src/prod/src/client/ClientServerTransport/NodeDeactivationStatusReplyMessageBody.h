// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class NodeDeactivationStatusReplyMessageBody : public Serialization::FabricSerializable
    {
    public:

        NodeDeactivationStatusReplyMessageBody()
        {
        }

        NodeDeactivationStatusReplyMessageBody(
            Common::ErrorCodeValue::Enum error,
            NodeDeactivationStatus::Enum deactivationStatus,
            std::vector<NodeProgress> && progressDetails)
            : error_(error)
            , deactivationStatus_(deactivationStatus)
            , progressDetails_(std::move(progressDetails))
        {
        }

        __declspec (property(get=get_Error)) Common::ErrorCodeValue::Enum Error;
        Common::ErrorCodeValue::Enum get_Error() const { return error_; }

        __declspec (property(get=get_DeactivationStatus)) NodeDeactivationStatus::Enum DeactivationStatus;
        NodeDeactivationStatus::Enum get_DeactivationStatus() const { return deactivationStatus_; }

        __declspec (property(get = get_ProgressDetails)) std::vector<NodeProgress> const& ProgressDetails;
        std::vector<NodeProgress> const& get_ProgressDetails() const { return progressDetails_; }

        FABRIC_FIELDS_03(error_, deactivationStatus_, progressDetails_);

    private:

        Common::ErrorCodeValue::Enum error_;
        NodeDeactivationStatus::Enum deactivationStatus_;
        std::vector<NodeProgress> progressDetails_;
    };
}
