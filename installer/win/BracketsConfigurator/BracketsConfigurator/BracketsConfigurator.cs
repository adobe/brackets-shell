/*
 * Copyright (c) 2018 - present Adobe Systems Incorporated. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 */

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
