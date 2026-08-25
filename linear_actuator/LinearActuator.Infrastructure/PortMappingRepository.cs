using LinearActuator.Core;
using Microsoft.EntityFrameworkCore;

namespace LinearActuator.Infrastructure;

public sealed class PortMappingRepository
{
    private readonly LinearActuatorDbContext dbContext;

    public PortMappingRepository(LinearActuatorDbContext dbContext)
    {
        this.dbContext = dbContext;
    }

    public async Task<PortMapping> LoadOrCreateDefaultAsync(CancellationToken cancellationToken = default)
    {
        await dbContext.Database.EnsureCreatedAsync(cancellationToken);

        PortMapping? existing = await dbContext.PortMappings
            .OrderBy(mapping => mapping.Id)
            .FirstOrDefaultAsync(cancellationToken);

        if (existing is not null)
        {
            return existing;
        }

        PortMapping mapping = new()
        {
            Name = "API01",
            ComPort = "COM4",
            ApiHost = ActuatorConstants.DefaultApiHost,
            ApiPort = ActuatorConstants.DefaultApiPort,
            BaudRate = ActuatorConstants.DefaultBaudRate,
            Enabled = true
        };

        dbContext.PortMappings.Add(mapping);
        await dbContext.SaveChangesAsync(cancellationToken);
        return mapping;
    }

    public async Task SaveAsync(PortMapping mapping, CancellationToken cancellationToken = default)
    {
        await dbContext.Database.EnsureCreatedAsync(cancellationToken);

        if (mapping.Id == 0)
        {
            dbContext.PortMappings.Add(mapping);
        }
        else
        {
            dbContext.PortMappings.Update(mapping);
        }

        await dbContext.SaveChangesAsync(cancellationToken);
    }
}
