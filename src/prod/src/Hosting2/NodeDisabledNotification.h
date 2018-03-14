// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Hosting2
{
    class NodeDisabledNotification : public Serialization::FabricSerializable
    {
    public:
        NodeDisabledNotification();
        NodeDisabledNotification(std::wstring const & taskId);
            
        __declspec(property(get=get_TaskId)) std::wstring const & TaskId;
        std::wstring const & get_TaskId() const { return taskId_; }

        void WriteTo(Common::TextWriter & w, Common::FormatOptions const &) const;

        FABRIC_FIELDS_01(taskId_);

    private:
        std::wstring taskId_;
    };
}
