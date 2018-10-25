// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Chaos.RandomActionGenerator
{
    using System;
    using System.Globalization;

    internal class RestartCodePackageStateTransitionAction : StateTransitionAction
    {
        public RestartCodePackageStateTransitionAction(
            string nodeName, 
            Uri applicationName,
            string serviceManifestName,
            string servicePackageActivationId,
            string codePackageName,
            Guid groupId)
            : base(StateTransitionActionType.CodePackageRestart, groupId, Guid.NewGuid())
        {
            this.NodeName = nodeName;
            this.ApplicationName = applicationName;
            this.ServiceManifestName = serviceManifestName;
            this.ServicePackageActivationId = servicePackageActivationId;
            this.CodePackageName = codePackageName;
        }

        public string NodeName { get; private set; }

        public Uri ApplicationName { get; private set; }

        public string ServiceManifestName { get; private set; }

        public string ServicePackageActivationId { get; private set; }

        public string CodePackageName { get; private set; }

        public override string ToString()
        {
            string result = string.Format(CultureInfo.InvariantCulture, "{0}, NodeName: {1}, ApplicationName: {2}, ServiceManifestName: {3}, CodePackageName: {4}.",
                base.ToString(),
                this.NodeName,
                this.ApplicationName,
                this.ServiceManifestName,
                this.CodePackageName);

            return !string.IsNullOrEmpty(this.ServicePackageActivationId)
                ? string.Format(CultureInfo.InvariantCulture, "{0}, ServicePackageActivationId: {1}", result, this.ServicePackageActivationId)
                : result;
        }
    }
}