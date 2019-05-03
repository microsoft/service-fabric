// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

// CS1591 - Missing XML comment for publicly visible type or member 'Type_or_Member' is disabled in this file because it does not ship anymore.
#pragma warning disable 1591

namespace System.Fabric.Testability.Client.Structures
{
    using System;
    using System.Collections.Generic;
    using System.Globalization;
    using System.Text;
    using System.Fabric.Testability.Common;

    [Serializable]
    public class WinFabricNameEnumerationResult
    {
        public WinFabricNameEnumerationResult(int identifier, NameEnumerationResult result)
        {
            ThrowIf.Null(result, "result");

            this.Identifier = identifier;
            this.Result = result;
        }

        public int Identifier
        {
            get;
            private set;
        }

        public NameEnumerationResult Result
        {
            get;
            set;
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendFormat(CultureInfo.InvariantCulture, "Names: ");

            if (this.Result != null)
            {
                for (int i = 0; i < this.Result.Count; i++)
                {
                    sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "{0}: {1}", i + 1, this.Result[i]));
                }

                sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "HasMoreData: {0}", this.Result.HasMoreData));
                sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "IsBestEffort: {0}", this.Result.IsBestEffort));
                sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "IsConsistent: {0}", this.Result.IsConsistent));
                sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "IsFinished: {0}", this.Result.IsFinished));
                sb.AppendLine(string.Format(CultureInfo.InvariantCulture, "IsValid: {0}", this.Result.IsValid));
            }

            return sb.ToString();
        }
    }
}


#pragma warning restore 1591