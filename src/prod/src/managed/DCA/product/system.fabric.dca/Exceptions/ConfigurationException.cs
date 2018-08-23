// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Dca.Exceptions
{
    /// <summary>
    /// Exception class to capture any configuration related exceptions (mostly originating from cluster manifest) happening in fabric data collection agent.
    /// It is recommended that any known exception which can be self-diagnosed by user be surfaced using this exception.
    /// Sufficient context in the form of ExceptionMessage is recommended when raising this type of exception
    /// </summary>
    internal class ConfigurationException : Exception
    {
        public ConfigurationException(string message)
            : base(message)
        {
        }

        public ConfigurationException(string message, Exception inner)
            : base(message, inner)
        {
        }
    }
}