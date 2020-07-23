// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using Threading.Tasks;

    /// <summary>
    /// Used for enabling MR learning mode. 
    /// <see cref="https://microsoft.sharepoint.com/teams/WindowsFabric/Team/Documentation/V2/Infrastructure%20Integration/MR%20Learning%20Mode.docx?d=w74eae1ab1f444034852a7baff21a02f8"/>
    /// for a general idea.
    /// </summary>
    internal interface IJobImpactManager
    {
        /// <summary>
        /// Evaluates the notification and determines the impact that the notification can have on the nodes
        /// </summary>
        /// <param name="notification">The notification from the MR SDK.</param>
        /// <returns>The impact that the notification's job has on the nodes.</returns>
        Task<JobImpactTranslationMode> EvaluateJobImpactAsync(IManagementNotificationContext notification);

        /// <summary>
        /// Resets the internal state.
        /// </summary>
        void Reset();
    }
}