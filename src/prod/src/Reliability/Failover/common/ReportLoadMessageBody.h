// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class ReportLoadMessageBody : public Serialization::FabricSerializable
    {
        DENY_COPY(ReportLoadMessageBody);

    public:
        ReportLoadMessageBody()
        {
        }

        ReportLoadMessageBody(ReportLoadMessageBody && other) :
            reportsNew_(std::move(other.reportsNew_)),
            senderTime_(other.senderTime_),
            reports_(std::move(other.reports_))
        {
        }

        ReportLoadMessageBody(std::vector<LoadBalancingComponent::LoadOrMoveCostDescription> && reports, Common::StopwatchTime senderTime) :
            reportsNew_(std::move(reports)), senderTime_(senderTime)
        {
        }

        ReportLoadMessageBody & operator=(ReportLoadMessageBody && other)
        {
            if (this != &other)
            {
                reportsNew_ = std::move(other.reportsNew_);
                senderTime_ = other.senderTime_;
            }

            return *this;
        }

        __declspec (property(get=get_Reports)) std::vector<LoadBalancingComponent::LoadOrMoveCostDescription> & Reports;
        std::vector<LoadBalancingComponent::LoadOrMoveCostDescription> const & get_Reports() const { return reportsNew_; }
        std::vector<LoadBalancingComponent::LoadOrMoveCostDescription> & get_Reports() { return reportsNew_; }

        __declspec (property(get=get_SenderTime)) Common::StopwatchTime SenderTime;
        Common::StopwatchTime get_SenderTime() const { return senderTime_; }

        void WriteTo(Common::TextWriter& writer, Common::FormatOptions const&) const
        {
            writer.Write("{0} {1}", reportsNew_, senderTime_);
        }

        FABRIC_FIELDS_03(reports_, reportsNew_, senderTime_);

    private:
        std::vector<ReportLoadInfo> reports_;
        std::vector<LoadBalancingComponent::LoadOrMoveCostDescription> reportsNew_;
        Common::StopwatchTime senderTime_;
    };
}
