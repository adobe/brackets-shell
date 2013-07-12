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
extern void GetKey(LPCWSTR pBase, LPCWSTR pGroup, LPCWSTR pApp, LPCWSTR pFolder, LPWSTR pRet);
extern bool GetRegistryInt(LPCWSTR pFolder, LPCWSTR pEntry, int* pDefault, int& ret);
extern bool WriteRegistryInt (LPCWSTR pFolder, LPCWSTR pEntry, int val);
