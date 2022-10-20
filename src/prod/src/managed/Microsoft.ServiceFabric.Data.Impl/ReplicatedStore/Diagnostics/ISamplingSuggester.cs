// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.ReplicatedStore.Diagnostics
{
    internal interface ISamplingSuggester
    {
        /// <summary>
        /// Advises this object to return true on next suggestion requested.
        /// </summary>
        void ResetSampling();

        /// <summary>
        /// Returns a suggestion on whether a sample should be taken.
        /// </summary>
        /// <returns>True if sampling is suggested. False otherwise.</returns>
        bool GetSamplingSuggestion();
    }
}
