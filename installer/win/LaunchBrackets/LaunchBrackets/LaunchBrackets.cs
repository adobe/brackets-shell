using System;
using System.Collections.Generic;
using System.Text;
using Microsoft.Deployment.WindowsInstaller;
using System.IO;

namespace LaunchBrackets
{
    public class LaunchBracketsClass
    {
        [CustomAction]
        public static ActionResult LaunchBrackets(Session session)
        {
            session.Log("Begin LaunchBrackets");
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
            session.Log("Begin LaunchBrackets");

            return ActionResult.Success;
        }
    }
}
