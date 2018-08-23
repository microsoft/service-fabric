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

        public bool Cancel
        {
            get;
            set;
        }
    }
}