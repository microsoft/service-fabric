// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace FabricDCA
{
    /// <summary>
    /// Defines the Syslog severity levels.
    /// </summary>
    public enum SyslogSeverity
    {
        /// <summary>
        /// System is unusable.
        /// </summary>
        Emergency = 0,

        /// <summary>
        /// Action must be taken immediately
        /// </summary>
        Alert = 1,

        /// <summary>
        /// Critical conditions
        /// </summary>
        Critical = 2,

        /// <summary>
        /// Error conditions
        /// </summary>
        Error = 3,

        /// <summary>
        /// Warning conditions
        /// </summary>
        Warning = 4,

        /// <summary>
        /// Normal but significant conditions
        /// </summary>
        Notice = 5,

        /// <summary>
        /// Informational messages
        /// </summary>
        Informational = 6,

        /// <summary>
        /// Debug-level messages
        /// </summary>
        Debug = 7
    };
}