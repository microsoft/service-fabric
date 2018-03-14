// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    template <Common::TraceTaskCodes::Enum TraceType>
    class ActivityTracedComponent
        : public Common::TextTraceComponent<TraceType>
    {
    public:
        ActivityTracedComponent::ActivityTracedComponent(
            std::wstring const & baseTraceId,
            Transport::FabricActivityHeader const & activityHeader)
            : baseTraceId_(baseTraceId) 
            , traceId_()
            , activityHeader_(activityHeader)
        {
            StringWriter writer(traceId_);
            writer.Write(
                "[{0}{1}{2}]",
                baseTraceId_,
                Constants::TracingTokenDelimiter,
                activityHeader_);
        }

    protected:
        __declspec(property(get=get_BaseTraceId)) std::wstring const & BaseTraceId;
        __declspec(property(get=get_TraceId)) std::wstring const & TraceId;
        __declspec(property(get=get_ActivityHeader)) Transport::FabricActivityHeader const & ActivityHeader;

        std::wstring const & get_BaseTraceId() const { return baseTraceId_; }
        std::wstring const & get_TraceId() const { return traceId_; }
        Transport::FabricActivityHeader const & get_ActivityHeader() const { return activityHeader_; }

    private:
        std::wstring baseTraceId_;
        std::wstring traceId_;
        Transport::FabricActivityHeader activityHeader_;
    };
}
