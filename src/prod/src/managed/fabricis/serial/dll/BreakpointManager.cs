// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.InfrastructureService
{
    using System;

    internal sealed class BreakpointManager
    {
        private readonly static TraceType TraceType = new TraceType("Breakpoint");

        public string LastHitBreakpoint { get; private set; }
        public string ContinuePastBreakpoint { get; private set; }

        public bool StopAtBreakpoint(string breakpointName)
        {
            breakpointName.Validate("breakpointName");

            this.LastHitBreakpoint = breakpointName;

            if (this.ShouldContinue())
            {
                TraceType.WriteInfo("Continuing past breakpoint: {0}", this.LastHitBreakpoint);
                this.LastHitBreakpoint = null;
                return false;
            }

            TraceType.WriteInfo("Stopping at breakpoint: {0}", this.LastHitBreakpoint);
            return true;
        }

        public void SetBreakpointContinue(string breakpointName)
        {
            this.ContinuePastBreakpoint = breakpointName;

            if (string.IsNullOrEmpty(this.ContinuePastBreakpoint))
            {
                TraceType.WriteInfo("Cleared breakpoint continue");
            }
            else
            {
                TraceType.WriteInfo("Set breakpoint continue: {0}", this.ContinuePastBreakpoint);
            }

            if (this.ShouldContinue())
            {
                // This allows the LastHitBreakpoint to be cleared by the operator in the
                // case that the system stopped once at this breakpoint and then continued
                // on its own, and another breakpoint is never hit.
                this.LastHitBreakpoint = null;
            }
        }

        private bool ShouldContinue()
        {
            return string.Equals(
                this.LastHitBreakpoint,
                this.ContinuePastBreakpoint,
                StringComparison.Ordinal);
        }
    }
}