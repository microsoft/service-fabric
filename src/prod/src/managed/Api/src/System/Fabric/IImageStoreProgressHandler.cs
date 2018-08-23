// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric
{
    using System.Diagnostics.CodeAnalysis;

    /// <summary>
    /// <para>Defines the behavior that a progress handler must implement to process progress information from Image Store operations</para>
    /// </summary>
    public interface IImageStoreProgressHandler
    {
        /// <summary>
        /// <para>Retrieves the interval at which progress information is published for some details that use polling (such as application package upload and download progress). Returning zero will use the system default of 2 seconds.</para>
        /// </summary>
        /// <returns>
        /// <para>The <see cref="System.TimeSpan"/> representing the progress update interval.</para>
        /// </returns>
        TimeSpan GetUpdateInterval();

        /// <summary>
        /// <para>Reports the progress of the current operation</para>
        /// </summary>
        /// <param name="completedItems">The number of completed work items.</param>
        /// <param name="totalItems">The total number of work items.</param>
        /// <param name="itemType">The unit of measurement for each work item.</param> 
        /// <remarks>The total number of items is not always guaranteed to be known at the beginning of the operation. In some cases, this value can increase if more work is discovered during processing.</remarks>
        void UpdateProgress(long completedItems, long totalItems, ProgressUnitType itemType);
    }
}