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
            entity.Property(mapping => mapping.Name).HasMaxLength(64).IsRequired();
            entity.Property(mapping => mapping.ComPort).HasMaxLength(32).IsRequired();
            entity.Property(mapping => mapping.ApiHost).HasMaxLength(128).IsRequired();
        });
    }
}
