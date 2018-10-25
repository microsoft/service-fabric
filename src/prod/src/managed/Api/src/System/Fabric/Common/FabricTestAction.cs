// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common
{
    using System;

    internal abstract class FabricTestAction<TResult> : FabricTestAction
    {
        protected FabricTestAction()
            : base()
        {
        }

        public TResult Result { get; set; }
    }

    internal abstract class FabricTestAction
    {
        protected static readonly int DefaultActionTimeoutSec = 300;
        protected static readonly int DefaultRequestTimeoutSec = 60;

        protected FabricTestAction()
        {   
            this.ActionTimeout = TimeSpan.FromSeconds(DefaultActionTimeoutSec);
            this.RequestTimeout = TimeSpan.FromSeconds(DefaultRequestTimeoutSec);
        }

        public TimeSpan ActionTimeout
        {
            get;
            set;
        }

        public TimeSpan RequestTimeout
        {
            get;
            set;
        }

        internal abstract Type ActionHandlerType { get; }
    }
}