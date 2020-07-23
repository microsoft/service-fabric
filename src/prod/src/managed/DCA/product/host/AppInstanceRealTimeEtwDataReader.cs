// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA 
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Fabric.Common;
    using System.Fabric.Dca.Utility;

    // This class reads events from the real-time ETW session that contains 
    // information about the application instances on the node
    class AppInstanceRealTimeEtwDataReader : IDisposable
    {
        // Constants
        private const string TraceType = "AppInstanceRealTimeEtwDataReader";

        // The timestamp from which ETW events should be read
        DateTime startTime;

        // Method to invoke when an application instance needs to be added or updated
        AppInstanceTableManager.AddOrUpdateHandler addOrUpdateAppInstanceHandler;

        // Method to invoke when an application instance needs to be added or updated
        AppInstanceTableManager.RemoveHandler removeAppInstanceHandler;

        // Whether or not the object has been disposed
        private bool disposed;

        internal AppInstanceRealTimeEtwDataReader(
                     DateTime startTime,
                     AppInstanceTableManager.AddOrUpdateHandler addOrUpdateAppInstanceHandler,
                     AppInstanceTableManager.RemoveHandler removeAppInstanceHandler)
        {
            this.startTime = startTime;
            this.addOrUpdateAppInstanceHandler = addOrUpdateAppInstanceHandler;
            this.removeAppInstanceHandler = removeAppInstanceHandler;
        }

        public void Dispose()
        {
            if (this.disposed)
            {
                return;
            }
            this.disposed = true;
        }
    }
}