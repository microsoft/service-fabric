// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.BackupRestore
{
    internal class RetriableOperationExceptionHandler
    {
        internal RetriableOperationExceptionHandler(Signature handler)
        {
            this.Handler = handler;
        }

        // Delegate representing the exception handler for exceptions that might occur during a retriable operation
        internal delegate Response Signature(Exception e);

        // Possible responses to an exception that occurs during a retriable operation
        internal enum Response
        {
            Abort,
            Retry
        }

        // Exception handler associated with this object
        internal Signature Handler { get; private set; }
    }
}