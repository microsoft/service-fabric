// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    using System;
    
    // This class implements the exception that is thrown when a failure is not
    // resolved even after reaching the maximum retry count.
    class MaxRetriesException : Exception
    {
        internal MaxRetriesException() : base()
        {
        }
        
        internal MaxRetriesException(string message) : base(message)
        {
        }
        
        internal MaxRetriesException(string message, Exception innerException) : base(message, innerException)
        {
        }
    }
}