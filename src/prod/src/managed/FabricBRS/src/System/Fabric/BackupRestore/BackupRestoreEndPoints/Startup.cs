// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

using System.Fabric.BackupRestore.BackupRestoreEndPoints.Handlers;

namespace System.Fabric.BackupRestore.BackupRestoreEndPoints
{
    using System.Fabric.BackupRestore.BackupRestoreEndPoints.Controllers;
    using System.Web.Http;
    using System.Threading;
    using Owin;
    using Microsoft.ServiceFabric.Services.Runtime;
    using System.Net;
    using System.Fabric.BackupRestore.View.JsonConverter;

    public class Startup
    {
        private ControllerSetting controllerSetting;
        private SecuritySetting fabricBrsSecuritySetting;

        internal Startup(StatefulService statefulService, SecuritySetting securitySetting, CancellationToken cancellationToken)
        {
            this.controllerSetting = new ControllerSetting(statefulService, cancellationToken);
            this.fabricBrsSecuritySetting = securitySetting;
        }

        public void Configuration(IAppBuilder appBuilder)
        {
            // Configure Web API for self-host. 
            HttpConfiguration config = new HttpConfiguration();
            config.DependencyResolver = new ControllerDependencyResolver<BaseController>(this.controllerSetting);
            switch (this.fabricBrsSecuritySetting.ServerAuthCredentialType)
            {
                case CredentialType.X509:
                    config.MessageHandlers.Add(new ClientCertAuthorizationHandler(this.fabricBrsSecuritySetting));
                    break;

                case CredentialType.Windows:
                    HttpListener listener = (HttpListener)appBuilder.Properties["System.Net.HttpListener"];
                    listener.AuthenticationSchemes = AuthenticationSchemes.IntegratedWindowsAuthentication;
                    config.MessageHandlers.Add(new ClientIdentityAuthorizationHandler(this.fabricBrsSecuritySetting));
                    break;
            }

            config.MessageHandlers.Add(new XRoleHeaderHandler(this.fabricBrsSecuritySetting.ServerAuthCredentialType));
            config.MessageHandlers.Add(new XContentTypeHandler());

            config.MapHttpAttributeRoutes();
            appBuilder.UseWebApi(config);
        }

    }
}