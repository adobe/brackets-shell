using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Deployment.WindowsInstaller;
using System.IO;

namespace CustomAction2
{
    public class CustomActions
    {
        [CustomAction]
        public static ActionResult CustomAction2(Session session)
        {
            session.Log("Begin CustomAction2");
            string bracketsInstallLocation = session["INSTALLDIRREGISTRY"];
            if (!string.IsNullOrEmpty(bracketsInstallLocation))
            {
                session.Log("Received Brackets Install Location: " + bracketsInstallLocation);
                if (File.Exists(bracketsInstallLocation))
                {
                    session.Log("Running brackets");
                    System.Diagnostics.Process.Start(bracketsInstallLocation);
                }
            }
            session.Log("Begin CustomAction2");

            return ActionResult.Success;
        }
    }
}
