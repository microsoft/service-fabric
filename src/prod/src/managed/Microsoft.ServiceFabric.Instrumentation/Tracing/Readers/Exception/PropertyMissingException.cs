// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Instrumentation.Tracing.Readers.Exception
{
    using System;
    using System.Globalization;
    using Microsoft.ServiceFabric.Instrumentation.Tracing.Core.Definition;

    public class PropertyMissingException : Exception
    {
        public PropertyMissingException(string propertyName)
        {
            Assert.IsNotNull(propertyName, "propertyName != null");
            this.PropertyName = propertyName;
        }

        public string PropertyName { get; }

        public override string Message
        {
            get { return string.Format(CultureInfo.InvariantCulture, "Property: {0} is Missing", this.PropertyName); }
        }
    }
}