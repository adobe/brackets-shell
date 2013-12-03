#pragma once
/*
 * Copyright (c) 2013 Adobe Systems Incorporated. All rights reserved.
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
 */
#include "cef_dark_window.h"

#define CDW_UPDATEMENU WM_USER+1004

// Dark window themed window Wrapper
//  Instantiate this class and subclass any HWND to give the window a dark look
class cef_dark_aero_window : public cef_dark_window
{
public:
    // Construction/Destruction - Public Members
    cef_dark_aero_window();
    virtual ~cef_dark_aero_window();

    virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);

    virtual BOOL GetBrowserRect(RECT& r) const;

protected:
    // Window Message Handlers
    BOOL HandleActivate();
    BOOL HandleCreate();
    BOOL HandlePaint();
    BOOL HandleNcCalcSize(BOOL calcValidRects, NCCALCSIZE_PARAMS* lpncsp, LRESULT* lr);
    BOOL HandleSysCommand(UINT command);

    int HandleNcHitTest(LPPOINT ptHit);

    void PaintCustomFrame(HDC hdc);

    void UpdateMenuBar();

    void ComputeMenuBarRect(RECT& rect);


    // Teardown
    void DoFinalCleanup();

    LRESULT DwpCustomFrameProc(UINT message, WPARAM wParam, LPARAM lParam, bool* pfCallDefWindowProc);


private:
    bool mReady;
};
