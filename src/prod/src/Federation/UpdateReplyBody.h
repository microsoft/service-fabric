// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class UpdateReplyBody : public Serialization::FabricSerializable
    {
    public:
        UpdateReplyBody() {}
        UpdateReplyBody(GlobalLease const & globalLease, NodeIdRange const & updateRange, bool isFromExponentialTarget)
            : globalLease_(globalLease), updateRange_(updateRange), isFromExponentialTarget_(isFromExponentialTarget)
        {
        }

        static bool ReadFromMessage(Transport::Message & message, UpdateReplyBody & updateMessageBody)
        {
            bool result = message.GetBody<UpdateReplyBody>(updateMessageBody);
            if (result)
            {
                updateMessageBody.GlobalLease.AdjustTime();
            }

            return result;
        }

        __declspec(property(get=getGlobalLease)) Federation::GlobalLease & GlobalLease;
        Federation::GlobalLease & getGlobalLease() { return globalLease_; }
        __declspec(property(get=getUpdateRange)) NodeIdRange const & UpdateRange;
        NodeIdRange const & getUpdateRange() const { return updateRange_; }
        __declspec(property(get=getIsFromExponentialTarget)) bool IsFromExponentialTarget;
        bool getIsFromExponentialTarget() const { return isFromExponentialTarget_; }

        FABRIC_FIELDS_03(globalLease_, updateRange_, isFromExponentialTarget_);

    private:
        Federation::GlobalLease globalLease_;
        NodeIdRange updateRange_;
        bool isFromExponentialTarget_;
    };
}
