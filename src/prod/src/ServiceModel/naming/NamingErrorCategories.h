// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Naming
{
    class NamingErrorCategories
    {
    public:
        static bool IsRetryableAtGateway(Common::ErrorCode const & error);
        static Common::ErrorCode ToClientError(Common::ErrorCode && error);
        static bool ShouldRevertCreateService(Common::ErrorCode const & error);
    };
}
