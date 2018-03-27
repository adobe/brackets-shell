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
	static ExtensionString m_InstallerPath;
	static ExtensionString m_logFilePath;	//AUTOUPDATE_PRERELEASE
public:
	void static SetInstallerPath(ExtensionString &path);
	void static SetlaunchInstaller(bool val);
	void static SetLogFilePath(ExtensionString &logFilePath);	//AUTOUPDATE_PRERELEASE
	void static RunAppUpdate();
	void static SetUpdateArgs(ExtensionString &installerPath, ExtensionString &logFilePath);
	bool static IsAutoUpdateInProgress() { return m_blaunchInstaller; }
};

bool UpdateHelper::m_blaunchInstaller;
ExtensionString UpdateHelper::m_InstallerPath;
ExtensionString UpdateHelper::m_logFilePath;

// Runs the installer for app update
void UpdateHelper::RunAppUpdate() {
	if (m_blaunchInstaller ) {
		std::wstring commandInput = L"msiexec /i ";

		commandInput += m_InstallerPath;

		commandInput += L" /qr";

		//AUTOUPDATE_PRERELEASE
		if (!m_logFilePath.empty()) {
			commandInput += L" /l*V ";
			commandInput += m_logFilePath;
		}
		commandInput += L" LAUNCH_APPLICATION_SILENT=1 MSIFASTINSTALL=2";

		const wchar_t* input = commandInput.c_str();

		wchar_t cmd[MAX_PATH];
		size_t nSize = _countof(cmd);
		_wgetenv_s(&nSize, cmd, L"COMSPEC");
		wchar_t cmdline[MAX_PATH + 50];
		swprintf_s(cmdline, L"%s /c %s", cmd, input);

		STARTUPINFOW startInf;
		memset(&startInf, 0, sizeof startInf);
		startInf.cb = sizeof(startInf);


		PROCESS_INFORMATION procInfo;
		memset(&procInfo, 0, sizeof procInfo);

		//AutoUpdate : TODO : Error handling
		//Create the installer process
		CreateProcessW(NULL, cmdline, NULL, NULL, FALSE, CREATE_NO_WINDOW, NULL, NULL, &startInf, &procInfo);
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
void UpdateHelper::SetUpdateArgs(ExtensionString &installerPath, ExtensionString &logFilePath)
{
	SetInstallerPath(installerPath);
	SetLogFilePath(logFilePath);
}

// Sets the installer log file path
void UpdateHelper::SetLogFilePath(ExtensionString &logFilePath) {
	if (!logFilePath.empty()) {
		m_logFilePath = logFilePath;
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
		UpdateHelper::SetUpdateArgs(installerPath, logFilePath);
		argsDict = NULL;
	}
	return error;
}

//  Runs the app auto update
int32 RunAppUpdate() {
	UpdateHelper::RunAppUpdate();
	return NO_ERROR;
}

// Checks if the auto update is in progress
int32 IsAutoUpdateInProgress(bool &isAutoUpdateInProgress)
{
	isAutoUpdateInProgress = UpdateHelper::IsAutoUpdateInProgress();
	return NO_ERROR;
}