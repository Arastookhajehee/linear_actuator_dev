using Microsoft.EntityFrameworkCore;

namespace LinearActuator.Infrastructure;

public sealed class LinearActuatorDbContext : DbContext
{
    public LinearActuatorDbContext(DbContextOptions<LinearActuatorDbContext> options)
        : base(options)
    {
    }

    public DbSet<PortMapping> PortMappings => Set<PortMapping>();

    protected override void OnModelCreating(ModelBuilder modelBuilder)
    {
        modelBuilder.Entity<PortMapping>(entity =>
        {
            entity.HasIndex(mapping => mapping.ModuleId).IsUnique();
            entity.Property(mapping => mapping.ModuleId).HasMaxLength(8).IsRequired();
            entity.Property(mapping => mapping.ComPort).HasMaxLength(32).IsRequired();
        });
    }
}
