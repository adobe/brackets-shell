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

#include "update.h"
#include "windows.h"
#include <string>

//Helper class for app auto update
class UpdateHelper {
private:
	static bool m_blaunchInstaller;
	static ExtensionString m_InstallerPath;	 //Path to the installer file
	static ExtensionString m_logFilePath;	// Path to the installer log file //AUTOUPDATE_PRERELEASE
	static ExtensionString m_installStatusFilePath;	// Path to the log file which logs errors which prevent the installer from running
	
public:
	void static SetInstallerPath(ExtensionString &path);
	void static SetInstallStatusFilePath(ExtensionString &path);
	void static SetlaunchInstaller(bool val);
	void static SetLogFilePath(ExtensionString &logFilePath);	//AUTOUPDATE_PRERELEASE
	void static RunAppUpdate();
	void static SetUpdateArgs(ExtensionString &installerPath, ExtensionString &installStatusFilePath, ExtensionString &logFilePath);
};

bool UpdateHelper::m_blaunchInstaller;
ExtensionString UpdateHelper::m_InstallerPath;
ExtensionString UpdateHelper::m_logFilePath;
ExtensionString UpdateHelper::m_installStatusFilePath;

// Runs the installer for app update
void UpdateHelper::RunAppUpdate() {
	if (m_blaunchInstaller ) {

		std::wstring commandInput = L"msiexec /i " + m_InstallerPath + L" /qr";
		//AUTOUPDATE_PRERELEASE
		if (!m_logFilePath.empty()) {
			commandInput += L" /li " + m_logFilePath;
		}
		commandInput += L" LAUNCH_APPLICATION_SILENT=1 MSIFASTINSTALL=2";

		const TCHAR* input = commandInput.c_str();
		TCHAR cmd[MAX_UNC_PATH];
		size_t nSize = _countof(cmd);
		errno_t comspecEnv = _wgetenv_s(&nSize, cmd, L"COMSPEC");

		// fp is a pointer to log file, which logs any errors encountered due to which installer could not be launched
		FILE *fp = NULL;
		if (comspecEnv == 0 && nSize != 0)
		{
			TCHAR cmdline[3*MAX_UNC_PATH];
			swprintf_s(cmdline, 3*MAX_UNC_PATH, L"%s /c %s", cmd, input);

			STARTUPINFOW startInf;
			memset(&startInf, 0, sizeof startInf);
			startInf.cb = sizeof(startInf);

			PROCESS_INFORMATION procInfo;
			memset(&procInfo, 0, sizeof procInfo);

			//Create the installer process
			BOOL installerProcess = CreateProcessW(NULL, cmdline, NULL, NULL, FALSE, CREATE_NEW_PROCESS_GROUP | CREATE_NO_WINDOW, NULL, NULL, &startInf, &procInfo);
			if (!installerProcess && !m_installStatusFilePath.empty()) {
				// Process could not be created successfully
				DWORD err = GetLastError();
				fp = _wfopen(m_installStatusFilePath.c_str(), L"a+");
				if (fp != NULL) {
					std::wstring errStr = std::to_wstring(err);
					fwprintf(fp, L"Installer process could not be created successfully. ERROR: %s : BA_05\n", errStr.c_str());
				}
			}
		}
		else {
			if (!m_installStatusFilePath.empty())
			{
				fp = _wfopen(m_installStatusFilePath.c_str(), L"a+");
				if (comspecEnv != 0)
				{
					//Command line interpreter could not be fetched
					if (fp != NULL) {
						std::wstring comspecStr = std::to_wstring(comspecEnv);
						fwprintf(fp, L"Command line interpreter could not be fetched from the current environment. ERROR: %s : BA_06\n", comspecStr.c_str());
					}
				}
				else
				{
					//COMSPEC variable not found
					if (fp != NULL) {
						std::wstring nSizeStr = std::to_wstring(nSize);
						fwprintf(fp, L"COMSPEC not found in the current environment. ERROR: %s : BA_07\n", nSizeStr.c_str());
					}
				}
			}
		}
		if(fp != NULL)
			fclose(fp);
	}
}

// Sets the Brackets installer path
void UpdateHelper::SetInstallerPath(ExtensionString &path) {
	if (!path.empty()) {
		SetlaunchInstaller(true);
		m_InstallerPath = path;
	}
}

//Sets the command line arguments to installer
void UpdateHelper::SetUpdateArgs(ExtensionString &installerPath, ExtensionString &installStatusFilePath ,ExtensionString &logFilePath)
{
	SetInstallerPath(installerPath);
	SetLogFilePath(logFilePath);
	SetInstallStatusFilePath(installStatusFilePath);
}

// Sets the installer log file path
void UpdateHelper::SetLogFilePath(ExtensionString &logFilePath) {
	if (!logFilePath.empty()) {
		m_logFilePath = logFilePath;
	}
}

// Sets the install status log file path, which logs any errors encountered due to which installer could not be run
void UpdateHelper::SetInstallStatusFilePath(ExtensionString &installStatusFilePath) {
	if (!installStatusFilePath.empty()) {
		m_installStatusFilePath = installStatusFilePath;
	}
}
// Sets the boolean for conditional launch of Installer
void UpdateHelper::SetlaunchInstaller(bool launchInstaller) {
	m_blaunchInstaller = launchInstaller;
	if (!launchInstaller) {
		m_InstallerPath.clear();
	}
}

// Sets the command line arguments to the installer
int32 SetInstallerCommandLineArgs(CefString &updateArgs) {
	CefRefPtr<CefDictionaryValue> argsDict;
	int32 error = ParseCommandLineParamsJSON(updateArgs, argsDict);
	if (error == NO_ERROR && argsDict.get())
	{
		ExtensionString installerPath = argsDict->GetString("installerPath").ToWString();
		ExtensionString logFilePath = argsDict->GetString("logFilePath").ToWString();	//AUTOUPDATE_PRERELEASE
		ExtensionString installStatusFilePath = argsDict->GetString("installStatusFilePath").ToWString();
		UpdateHelper::SetUpdateArgs(installerPath, installStatusFilePath, logFilePath);
		argsDict = NULL;
	}
	return error;
}

//  Runs the app auto update
int32 RunAppUpdate() {
	UpdateHelper::RunAppUpdate();
	return NO_ERROR;
}