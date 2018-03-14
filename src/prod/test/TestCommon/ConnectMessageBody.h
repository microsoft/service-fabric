// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestCommon
{
    class ConnectMessageBody : public Serialization::FabricSerializable
    {
    public:
        ConnectMessageBody()
        {
        }

        ConnectMessageBody(DWORD processId)
            : processId_(processId)
        {
        }

        __declspec(property(get=get_ProcessId)) DWORD ProcessId;
        DWORD get_ProcessId() { return processId_; }

        FABRIC_FIELDS_01(processId_);

    private:
        DWORD processId_;
    };
};
