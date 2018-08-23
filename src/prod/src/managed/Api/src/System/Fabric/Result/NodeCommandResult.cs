// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Result
{
    using System.Fabric.Interop;
    using System.Globalization;
    using System.Text;

    /// <summary>
    /// Returns node command result object.
    /// </summary>    
    public class NodeCommandResult : TestCommandResult
    {
        internal NodeCommandResult(NodeResult nodeResult, Exception exception)
        {
            this.NodeResult = nodeResult;
            this.Exception = exception;
        }

        internal NodeCommandResult(NodeResult nodeResult, int errorCode)
        {
            this.NodeResult = nodeResult;
            this.ErrorCode = errorCode;            
        }

        internal NodeCommandResult() { }

        /// <summary>
        /// Gets the NodeResult object, which contains the information about the target node.
        /// </summary>
        /// <value>The NodeResult object.</value>
        public NodeResult NodeResult { get; private set; }
        
        internal int ErrorCode { get; set; }

        /// <summary>
        /// Returns the string representation of the Exception, if the command failed.
        /// </summary>
        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();

            sb.AppendFormat(CultureInfo.InvariantCulture, "NodeResult: {0}", this.NodeResult);
            if (this.Exception != null)
            {
                sb.AppendFormat(", Exception : {0}", this.Exception);
            }

            return sb.ToString();
        }

        internal unsafe void CreateFromNative(IntPtr pointer)
        {
            NativeTypes.NODE_TRANSITION_RESULT nativeResult = *(NativeTypes.NODE_TRANSITION_RESULT*)pointer;            
            this.NodeResult = NodeResult.CreateFromNative(nativeResult.NodeResult);
            this.ErrorCode = nativeResult.ErrorCode;
            try
            {
                this.Exception = InteropHelpers.TranslateError(this.ErrorCode);
            }
            catch (FabricException)
            {
                // Exception could not be translated.  To the user, it will look like whole FabricClient API itself failed, 
                // instead of just the translation above, and they won't be able to determine what happened.  So 
                // instead, suppress this.  The user will still have this.ErrorCode.  We will still  
                // be able to catch and fix situations that hit this case in test automation because 
                // there be a check on this.Exception there.
            }
        }

        internal unsafe IntPtr ToNative(PinCollection pinCollection)
        {
            var nativeResult = new NativeTypes.NODE_TRANSITION_RESULT();
            if (this.NodeResult != null)
            {
                nativeResult.NodeResult = this.NodeResult.ToNative(pinCollection);
            }
            
            nativeResult.ErrorCode = this.ErrorCode;

            return pinCollection.AddBlittable(nativeResult);
        }
    }
}