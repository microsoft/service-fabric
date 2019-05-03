// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Threading;

namespace Microsoft.ServiceFabric.ReplicatedStore.Diagnostics
{
    internal class SamplingSuggester : ISamplingSuggester
    {
        private const int Yes = 1;
        private const int No = 0;

        // We use int instead of bool because Interlocked.CompareExchange doesn't work with bools.
        private int samplingSuggested = Yes;

        /// <summary>
        /// Advises this object to return true on next suggestion requested.
        /// </summary>
        public void ResetSampling()
        {
            this.samplingSuggested = Yes;
        }

        /// <summary>
        /// Returns a suggestion on whether a sample should be taken.
        /// </summary>
        /// <returns>True if sampling is suggested. False otherwise.</returns>
        public bool GetSamplingSuggestion()
        {
            var result = this.samplingSuggested;

            if (result == Yes)
            {
                result = Interlocked.CompareExchange(ref this.samplingSuggested, No, Yes);
            }

            return result == Yes;
        }
    }
}
