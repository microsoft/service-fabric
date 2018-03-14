// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    class DeleteServiceMessageBody : public BasicFailoverRequestMessageBody
    {
    public:
        DeleteServiceMessageBody()
            : isForce_(false)
            , instanceId_(0)
        {
        }

        DeleteServiceMessageBody(
            std::wstring const & serviceName,
            bool const isForce,
            uint64 instanceId)
            : BasicFailoverRequestMessageBody(serviceName)
            , isForce_(isForce)
            , instanceId_(instanceId)
        {
        }

        __declspec (property(get=get_InstanceId)) uint64 InstanceId;
        __declspec (property(get=get_IsForce)) bool IsForce;

        uint64 get_InstanceId() const { return instanceId_; }
        bool get_IsForce() const { return isForce_; }

        void WriteTo(Common::TextWriter& w, Common::FormatOptions const&) const
        {
            w.Write("{0}, IsForce {1} ({2})", this->Value, isForce_, instanceId_);
        }

        FABRIC_FIELDS_02(instanceId_, isForce_);

    private:
        uint64 instanceId_;
        bool isForce_;
    };
}

