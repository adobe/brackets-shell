#pragma once
/*************************************************************************
 *
 * ADOBE CONFIDENTIAL
 * ___________________
 *
 *  Copyright 2013 Adobe Systems Incorporated
 *  All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains
 * the property of Adobe Systems Incorporated and its suppliers,
 * if any.  The intellectual and technical concepts contained
 * herein are proprietary to Adobe Systems Incorporated and its
 * suppliers and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Adobe Systems Incorporated.
 **************************************************************************/
#include "cef_window.h"

class cef_main_window : public cef_window
{
public:
    cef_main_window(void);
    virtual ~cef_main_window(void);

	static HWND SafeGetCefBrowserHwnd();

	BOOL Create();
	
	void ShowHelp();
	void ShowAbout();

    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

protected:
	void SaveWindowRect();
	void RestoreWindowRect(int& left, int& top, int& width, int& height, int& showCmd);
	void RestoreWindowPlacement(int showCmd);

	BOOL HandleEraseBackground();
	BOOL HandleCreate();
	BOOL HandleSetFocus(HWND hLosingFocus);
	BOOL HandleDestroy();
	BOOL HandleClose();
	BOOL HandleSize(BOOL bMinimize);
	BOOL HandleInitMenuPopup(HMENU hMenuPopup);
	BOOL HandleCommand(UINT commandId);
	BOOL HandleExitCommand();
    BOOL HandlePaint();
    BOOL HandleGetMinMaxInfo(LPMINMAXINFO mmi);

	virtual void PostNonClientDestory();

private:
	static ATOM RegisterWndClass();
};

