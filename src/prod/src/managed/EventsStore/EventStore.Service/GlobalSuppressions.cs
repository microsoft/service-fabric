
// This file is used by Code Analysis to maintain SuppressMessage 
// attributes that are applied to this project.
// Project-level suppressions either have no target or are given 
// a specific target and scoped to a namespace, type, member, etc.

[assembly: System.Diagnostics.CodeAnalysis.SuppressMessage("Usage", "CC0057:Unused parameters", Justification = "<Pending>", Scope = "module")]
[assembly: System.Diagnostics.CodeAnalysis.SuppressMessage("Style", "CC0001:You should use 'var' whenever possible.", Justification = "<Pending>", Scope = "module")]
[assembly: System.Diagnostics.CodeAnalysis.SuppressMessage("Design", "CC0021:Use nameof", Justification = "<Pending>", Scope = "module")]
[assembly: System.Diagnostics.CodeAnalysis.SuppressMessage("Style", "CC0105:You should use 'var' whenever possible.", Justification = "<Pending>", Scope = "module")]
[assembly: System.Diagnostics.CodeAnalysis.SuppressMessage("Design", "CC0031:Check for null before calling a delegate", Justification = "Already Verified in earlier Assert.", Scope = "member", Target = "~M:EventStore.Service.Cache.InMemorySortedStore`2.RemoveSelectedItemsAsync(System.Func{`0,`1,System.Boolean},System.Threading.CancellationToken)~System.Threading.Tasks.Task{System.Int32}")]
[assembly: System.Diagnostics.CodeAnalysis.SuppressMessage("Design", "CC0004:Catch block cannot be empty", Justification = "It's a best effort operation and nothing is impacted if it fails.", Scope = "member", Target = "~M:EventStore.Service.Cache.AzureTableCachedStorageAccess.WriteTelemetryAsync(System.Threading.CancellationToken)~System.Threading.Tasks.Task")]

