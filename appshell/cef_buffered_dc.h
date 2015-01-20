#pragma once
/*
 * Copyright (c) 2014 Adobe Systems Incorporated. All rights reserved.
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
#include "cef_window.h"


class cef_buffered_dc 
{
public:
    cef_buffered_dc(cef_window* window, HDC hdc = NULL) 
        : mWnd(window)
        , mWindowDC(hdc)
        , mDcMem(NULL)
        , mBitmap(NULL)
        , mBmOld(NULL)
        , mWidth(0)
        , mHeight(0)
        , mReleaseDcOnDestroy(false)
    {
        CommonInit();
    }

    operator HDC() 
    {
        return mDcMem;
    }

    HDC GetWindowDC() 
    {
        return mWindowDC;
    }

    ~cef_buffered_dc() 
    {
        Destroy();
    }

private:
    cef_buffered_dc()
    {
    }

    cef_buffered_dc(const cef_buffered_dc& other)
    {
    }

protected:
    void CommonInit() 
    {
        if (mWnd) {
            if (mWindowDC == NULL) {
                mWindowDC = mWnd->GetDC();
                mReleaseDcOnDestroy = true;
            }

            RECT rectWindow;
            mWnd->GetWindowRect(&rectWindow);
            mWidth = ::RectWidth(rectWindow);
            mHeight = ::RectHeight(rectWindow);

            mDcMem = ::CreateCompatibleDC(mWindowDC);
            mBitmap = ::CreateCompatibleBitmap(mWindowDC, mWidth, mHeight);
            mBmOld = ::SelectObject(mDcMem, mBitmap);
        }
    }

    void Destroy() 
    {
        if (mWnd) {
            ::BitBlt(mWindowDC, 0, 0, mWidth, mHeight, mDcMem, 0, 0, SRCCOPY);

            ::SelectObject(mDcMem, mBmOld);
            ::DeleteObject(mBitmap);
            ::DeleteDC(mDcMem);
        
            if (mReleaseDcOnDestroy) {
                mWnd->ReleaseDC(mWindowDC);
            }
        }
    }

    cef_window* mWnd;
    HDC         mWindowDC;
    HDC         mDcMem;
    HBITMAP     mBitmap;
    HGDIOBJ     mBmOld;
    int         mWidth; 
    int         mHeight;
    bool        mReleaseDcOnDestroy;
};