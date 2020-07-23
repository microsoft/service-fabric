// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace TestableService
{
    class CheckWorkStatusReponse : public Common::IFabricJsonSerializable
    {
    public:
        PROPERTY(WorkStatusEnum::Enum, workStatus_, WorkStatus)
        PROPERTY(NTSTATUS, statusCode_, StatusCode)

        BEGIN_JSON_SERIALIZABLE_PROPERTIES()
            SERIALIZABLE_PROPERTY_ENUM(L"WorkStatus", workStatus_)
            SERIALIZABLE_PROPERTY(L"StatusCode", statusCode_)
        END_JSON_SERIALIZABLE_PROPERTIES()

    private:
        NTSTATUS statusCode_;
        WorkStatusEnum::Enum workStatus_;
    };
}
