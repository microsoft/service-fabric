// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Tools.EtlReader
{
    using System;

    public class EventRecordEventArgs : EventArgs
    {
        public EventRecordEventArgs(EventRecord record)
        {
            this.Record = record;
        }

        public EventRecord Record
        {
            get;
            private set;
        }

#if DotNetCoreClrLinux
        // TODO - Following code will be removed once fully transitioned to structured traces in Linux
        // This is needed to allow the processing of unstructured and structured traces at the same time
        public string UnstructuredRecord
        {
            get;
            set;
        }

        public bool IsUnstructured
        {
            get;
            set;
        }

        public string TaskNameEventName
        {
            get;
            set;
        }
#endif

        public bool Cancel
        {
            get;
            set;
        }
    }
}
