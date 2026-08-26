using Grasshopper;
using Grasshopper.Kernel;
using System;
using System.Drawing;

namespace LinearActuator.GhClient
{
    public class LinearActuator_GhClientInfo : GH_AssemblyInfo
    {
        public override string Name => "Dynamic Formwork GhClient";

        //Return a 24x24 pixel bitmap to represent this GHA library.
        public override Bitmap Icon => null;

        //Return a short string describing the purpose of this GHA library.
        public override string Description => "";

        public override Guid Id => new Guid("60d76878-febb-4e74-af32-e6bc7bdc9791");

        //Return a string identifying you or your company.
        public override string AuthorName => "";

        //Return a string representing your preferred contact details.
        public override string AuthorContact => "";

        //Return a string representing the version.  This returns the same version as the assembly.
        public override string AssemblyVersion => GetType().Assembly.GetName().Version.ToString();
    }
}