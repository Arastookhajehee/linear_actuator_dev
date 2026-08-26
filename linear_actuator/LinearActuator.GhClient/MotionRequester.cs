using System;
using System.Collections.Generic;
using System.Linq;
using Grasshopper.Kernel;
using Grasshopper.Kernel.Data;
using Grasshopper.Kernel.Types;
using LinearActuator.ClientAPI;
using LinearActuator.Core;
using Rhino;

namespace LinearActuator.GhClient
{
    public class MotionRequester : GH_Component
    {
        /// <summary>
        /// Each implementation of GH_Component must provide a public
        /// constructor without any arguments.
        /// Category represents the Tab in which the component will appear,
        /// Subcategory the panel. If you use non-existing tab or panel names,
        /// new tabs/panels will automatically be created.
        /// </summary>
        public MotionRequester()
            : base("MotionClient", "MC", "Description", "AGB Formwork", "Client") { }

        public ActuatorStateBundle currentState { get; private set; }

        /// <summary>
        /// Registers all the input parameters for this component.
        /// </summary>
        protected override void RegisterInputParams(GH_Component.GH_InputParamManager pManager)
        {
            pManager.AddTextParameter(
                "URL",
                "URL",
                "Linear Actuator REST API Base URL",
                GH_ParamAccess.item,
                "http://172.0.0.1:7500"
            );
            pManager.AddNumberParameter(
                "Targets",
                "T",
                "Formwork linear actuator target coordinates as tree structure",
                GH_ParamAccess.tree
            );
            pManager.AddBooleanParameter(
                "Send Targets",
                "ST",
                "Send Targets to the server",
                GH_ParamAccess.item,
                false
            );
            pManager.AddBooleanParameter(
                "Get Current",
                "GC",
                "Get the current linear actuator positions",
                GH_ParamAccess.item,
                false
            );
        }

        /// <summary>
        /// Registers all the output parameters for this component.
        /// </summary>
        protected override void RegisterOutputParams(GH_Component.GH_OutputParamManager pManager)
        {
            pManager.AddTextParameter(
                "Current State",
                "CS",
                "Current linear actuator positions",
                GH_ParamAccess.item
            );

            pManager.AddTextParameter(
                "Outgoing Bundle",
                "B",
                "The linear actuator 10 module target bundle",
                GH_ParamAccess.item
            );
        }

        /// <summary>
        /// This is the method that actually does the work.
        /// </summary>
        /// <param name="DA">The DA object can be used to retrieve data from input parameters and
        /// to store data in output parameters.</param>
        protected override void SolveInstance(IGH_DataAccess DA)
        {
            string url = "";
            GH_Structure<GH_Number> targets = new();
            bool sendTargets = false;
            bool fetchCurrent = false;
            if (!DA.GetData(0, ref url))
                return;
            if (!DA.GetDataTree<GH_Number>(1, out targets))
                return;
            if (!DA.GetData(2, ref sendTargets))
                return;
            if (!DA.GetData(3, ref fetchCurrent))
                return;

            ActuatorStateBundle targetBundle = TargetTreeToBundle(targets);

            LinearActuatorApiClient client = new(new()) { BaseUrl = url };

            if (sendTargets)
            {
                ActuatorStateBundle response = (ActuatorStateBundle)
                    client.PostActuatorBundlesAsync(targetBundle).GetAwaiter().GetResult();
                RhinoApp.WriteLine(response.ToJson());
            }

            if (fetchCurrent)
            {
                currentState = (ActuatorStateBundle)
                    client.GetActuatorBundlesAsync().GetAwaiter().GetResult();
                RhinoApp.WriteLine(currentState.ToJson());
            }

            DA.SetData(0, currentState);
            DA.SetData(1, targetBundle);
        }

        private ActuatorStateBundle TargetTreeToBundle(GH_Structure<GH_Number> targets)
        {
            Dictionary<string, string> tags = new()
            {
                { "0:0", "M01" },
                { "0:1", "M02" },
                { "0:2", "M03" },
                { "0:3", "M04" },
                { "0:4", "M05" },
                { "1:0", "M06" },
                { "1:1", "M07" },
                { "1:2", "M08" },
                { "1:3", "M09" },
                { "1:4", "M10" },
            };

            ActuatorStateBundle bundle = new() { Modules = new() };

            // Write your logic here
            foreach (GH_Path path in targets.Paths)
            {
                ActuatorState actState = new();
                List<double> branch = targets
                    .get_Branch(path)
                    .Cast<GH_Number>()
                    .Select(g => g.Value)
                    .ToList();
                string path_string = string.Join(':', path.Indices);
                for (int i = 0; i < branch.Count; i++)
                {
                    switch (i)
                    {
                        case 0:
                            actState.A1Target = branch[i];
                            break;
                        case 1:
                            actState.A2Target = branch[i];
                            break;
                        case 2:
                            actState.A3Target = branch[i];
                            break;
                        case 3:
                            actState.A4Target = branch[i];
                            break;
                    }
                }
                bundle.Modules.Add(tags[path_string], actState);
            }

            return bundle;
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
        public override Guid ComponentGuid => new Guid("46b17d6f-cfd3-41ca-b877-caafac2e4c8e");
    }
}
