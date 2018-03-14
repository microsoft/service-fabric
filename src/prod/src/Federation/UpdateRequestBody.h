// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Federation
{
    class UpdateRequestBody : public Serialization::FabricSerializable
    {
    public:
        UpdateRequestBody() {}
        UpdateRequestBody(Common::StopwatchTime requestTime, bool isToExponentialTarget, NodeIdRange const & range)
            : requestTime_(requestTime), isToExponentialTarget_(isToExponentialTarget), range_(range)
        {
        }

        __declspec(property(get=getRequestTime)) Common::StopwatchTime RequestTime;
        Common::StopwatchTime getRequestTime() const { return requestTime_; }
        __declspec(property(get=getIsToExponentialTarget)) bool IsToExponentialTarget;
        bool getIsToExponentialTarget() const { return isToExponentialTarget_; }
        __declspec(property(get = getRange)) NodeIdRange const & Range;
        NodeIdRange const & getRange() const { return range_; }

        FABRIC_FIELDS_03(requestTime_, isToExponentialTarget_, range_);

    private:
        Common::StopwatchTime requestTime_;
        bool isToExponentialTarget_;
        NodeIdRange range_;
    };
}
