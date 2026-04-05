using Xunit;

namespace AIL_Tests
{
    /// <summary>
    /// Serialises all tests that touch the process-wide
    /// <see cref="Artemis_IL.Globals"/> singleton (console, DebugMode, ParentVM).
    /// Without this, concurrent test classes could interfere with each other's
    /// captured output.
    /// </summary>
    [CollectionDefinition("VM")]
    public sealed class VmCollection : ICollectionFixture<object> { }
}
