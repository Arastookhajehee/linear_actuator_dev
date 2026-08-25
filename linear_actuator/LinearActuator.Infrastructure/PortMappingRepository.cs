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

    public async Task<List<PortMapping>> LoadOrCreateDefaultsAsync(CancellationToken cancellationToken = default)
    {
        await dbContext.Database.EnsureCreatedAsync(cancellationToken);

        List<PortMapping> mappings = await dbContext.PortMappings
            .AsNoTracking()
            .OrderBy(mapping => mapping.ModuleId)
            .ToListAsync(cancellationToken);

        bool changed = false;
        for (int i = 1; i <= ActuatorConstants.ModuleCount; i++)
        {
            string moduleId = ActuatorConstants.FormatModuleId(i);
            if (mappings.Any(mapping => mapping.ModuleId == moduleId))
            {
                continue;
            }

            PortMapping mapping = new()
            {
                ModuleId = moduleId,
                ComPort = string.Empty,
                BaudRate = ActuatorConstants.DefaultBaudRate,
                SerialEnabled = false
            };

            dbContext.PortMappings.Add(mapping);
            mappings.Add(mapping);
            changed = true;
        }

        if (changed)
        {
            await dbContext.SaveChangesAsync(cancellationToken);
            dbContext.ChangeTracker.Clear();
            mappings = await dbContext.PortMappings
                .AsNoTracking()
                .OrderBy(mapping => mapping.ModuleId)
                .ToListAsync(cancellationToken);
        }

        return mappings.OrderBy(mapping => mapping.ModuleId).ToList();
    }

    public async Task SaveAsync(IEnumerable<PortMapping> mappings, CancellationToken cancellationToken = default)
    {
        await dbContext.Database.EnsureCreatedAsync(cancellationToken);
        dbContext.ChangeTracker.Clear();

        foreach (PortMapping mapping in mappings)
        {
            if (mapping.Id == 0)
            {
                dbContext.PortMappings.Add(mapping);
            }
            else
            {
                dbContext.PortMappings.Update(mapping);
            }
        }

        await dbContext.SaveChangesAsync(cancellationToken);
    }
}
