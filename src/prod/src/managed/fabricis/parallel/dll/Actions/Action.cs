// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService.Parallel
{
    using Threading.Tasks;

    internal abstract class Action : IAction
    {
        protected Action(CoordinatorEnvironment environment, ActionType actionType)
        {
            this.Environment = environment.Validate("environment");
            this.ActionType = actionType;
        }

        protected CoordinatorEnvironment Environment { get; private set; }

        public string Name {  get { return this.GetType().Name; } }

        public ActionType ActionType { get; private set; }

        public virtual Task ExecuteAsync(Guid activityId)
        {
            return Task.FromResult(0);
        }

        public override string ToString()
        {
            string text = "{0}, ActionType: {1}".ToString(this.GetType().Name, ActionType);
            return text;
        }
    }
}