using LinearActuator.Core;
using LinearActuator.Infrastructure;
using Microsoft.EntityFrameworkCore;

namespace LinearActuator.Tests;

public sealed class PortMappingRepositoryTests
{
    [Fact]
    public async Task LoadOrCreateDefaultsAsync_CreatesTenSerialMappings()
    {
        DbContextOptions<LinearActuatorDbContext> options = new DbContextOptionsBuilder<LinearActuatorDbContext>()
            .UseSqlite("Data Source=:memory:")
            .Options;

        await using LinearActuatorDbContext dbContext = new(options);
        await dbContext.Database.OpenConnectionAsync();

        PortMappingRepository repository = new(dbContext);

        List<PortMapping> mappings = await repository.LoadOrCreateDefaultsAsync();

        Assert.Equal(ActuatorConstants.ModuleCount, mappings.Count);
        Assert.Equal("M01", mappings[0].ModuleId);
        Assert.Equal("M10", mappings[^1].ModuleId);
        Assert.All(mappings, mapping =>
        {
            Assert.Equal(ActuatorConstants.DefaultBaudRate, mapping.BaudRate);
            Assert.False(mapping.SerialEnabled);
        });
    }
}
