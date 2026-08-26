using System;
using System.Collections.Generic;
using Grasshopper;
using Grasshopper.Kernel;
using Rhino.Geometry;

namespace LinearActuator.GhClient
{
    public class TwoSideSurfaceExtend : GH_Component
    {
        /// <summary>
        /// Each implementation of GH_Component must provide a public
        /// constructor without any arguments.
        /// Category represents the Tab in which the component will appear,
        /// Subcategory the panel. If you use non-existing tab or panel names,
        /// new tabs/panels will automatically be created.
        /// </summary>
        public TwoSideSurfaceExtend()
            : base("TwoSideSurfaceExtend", "TSSE", "Description", "AGB Formwork", "Data") { }

        /// <summary>
        /// Registers all the input parameters for this component.
        /// </summary>
        protected override void RegisterInputParams(GH_Component.GH_InputParamManager pManager)
        {
            pManager.AddGeometryParameter("Sruface", "S", "", GH_ParamAccess.item);
            pManager.AddNumberParameter("Amount","A","",GH_ParamAccess.item,100);
        }

        /// <summary>
        /// Registers all the output parameters for this component.
        /// </summary>
        protected override void RegisterOutputParams(GH_Component.GH_OutputParamManager pManager)
        {
            pManager.AddGeometryParameter("Sruface", "S", "", GH_ParamAccess.item);
        }

        /// <summary>
        /// This is the method that actually does the work.
        /// </summary>
        /// <param name="DA">The DA object can be used to retrieve data from input parameters and
        /// to store data in output parameters.</param>
        protected override void SolveInstance(IGH_DataAccess DA)
        {
            Surface x = null;
            double amount = 100;
            DA.GetData(0, ref x);
            DA.GetData(1, ref amount);
            var srv01 = x.Extend(IsoStatus.East, amount, true).Extend(IsoStatus.West, amount, true);
            var srv02 = srv01.Extend(IsoStatus.North, amount, true).Extend(IsoStatus.South, amount, true);
            DA.SetData(0, srv02);
        }

        /// <summary>
        /// Provides an Icon for every component that will be visible in the User Interface.
        /// Icons need to be 24x24 pixels.
        /// You can add image files to your project resources and access them like this:
        /// return Resources.IconForThisComponent;
        /// </summary>
        protected override System.Drawing.Bitmap Icon => null;

        /// <summary>
        /// Each component must have a unique Guid to identify it.
        /// It is vital this Guid doesn't change otherwise old ghx files
        /// that use the old ID will partially fail during loading.
        /// </summary>
        public override Guid ComponentGuid => new Guid("64404f97-fd44-4ed4-bc47-96f0b7a05af4");
    }
}
