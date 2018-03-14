// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Transport
{
    class RejectFaultBody : public Serialization::FabricSerializable
    {
    public:
        RejectFaultBody()
            : errorDetails_()
        {
        }

        RejectFaultBody(Common::ErrorCode const & error) 
            : errorDetails_(error)
        {
        }

        RejectFaultBody(Common::ErrorCode && error) 
            : errorDetails_(std::move(error))
        {
        }

        bool HasErrorMessage() const { return errorDetails_.HasErrorMessage(); }
        Common::ErrorCode TakeError() { return errorDetails_.TakeError(); }

        FABRIC_FIELDS_01(errorDetails_);

    private:
        // Duplicates ErrorCodeValue from FaultHeader, but will 
        // be more extensible than using just ErrorCodeValue 
        // or ErrorCode.
        //
        Common::ErrorCodeDetails errorDetails_;
    };
}
