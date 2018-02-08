using System;
using System.Collections.Generic;
using System.Text;
using System.Linq;
using Microsoft.Deployment.WindowsInstaller;

namespace BracketsConfigurator
{
    public class BracketsConfiguratorClass
    {
        [CustomAction]
        public static ActionResult BracketsConfigurator(Session session)
        {
            session.Log("Begin BracketsConfigurator");

            //Set Install Directory if available
            string installRegistry = session["INSTALLDIRREGISTRY"];
            string installDirValue = "";
            if (!string.IsNullOrEmpty(installRegistry))
            {
                session.Log("Install Registry Value: " + installRegistry);
                string[] path = installRegistry.Split('\\');
                //Remove executable name
                path = path.Take(path.Count() - 1).ToArray();
                //Get path without executable
                installDirValue = string.Join("\\", path) + "\\";
                session["INSTALLDIR"] = installDirValue;
                session.Log("Updating Install Dir To: " + installDirValue);
            }
            else
            {
                session.Log("No Registry Value for installation directory");
            }

            //Set AddContextMenu if required
            string contextMenuRegistry = session["CONTEXTMENUREGISTRY"];
            if (!string.IsNullOrEmpty(contextMenuRegistry))
            {
                session.Log("Context Menu Registry Value: " + contextMenuRegistry);
                session["ADDCONTEXTMENU"] = "1";
                session.Log("Setting Context Menu Option");
            }
            else
            {
                session["ADDCONTEXTMENU"] = "0";
                session.Log("Not Adding to Context Menu");
            }

            //Set UpdatePath if required
            string currentPathValue = session["CURRENTPATH"];
            session.Log("Current Path Value: " + currentPathValue);
            if (!string.IsNullOrEmpty(currentPathValue) && currentPathValue.Contains(installDirValue))
            {
                session["UPDATEPATH"] = "1";
                session.Log("Updating Path for Brackets");
            }
            else
            {
                session["UPDATEPATH"] = "0";
                session.Log("Not Updating Path");
            }

            session.Log("End BracketsConfigurator");
            return ActionResult.Success;
        }
    }
}
