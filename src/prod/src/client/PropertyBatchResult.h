// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Client
{
    class PropertyBatchResult
        : public Api::IPropertyBatchResult
        , public Common::ComponentRoot
    {
    public: 
        PropertyBatchResult(Naming::NamePropertyOperationBatchResult &&);
        virtual ~PropertyBatchResult() {};

        ULONG GetFailedOperationIndexInRequest();

        Common::ErrorCode GetError();

        Common::ErrorCode GetProperty(
            __in ULONG operationIndexInRequest, 
            __out Naming::NamePropertyResult &);

    private:
        std::map<ULONG, Naming::NamePropertyResult> properties_;
        std::vector<ULONG> propertiesNotFound_;
        ULONG failedOperationIndex_;
        Common::ErrorCode error_;
    };
}
