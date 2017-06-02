/*
 * Copyright (c) 2012 - present Adobe Systems Incorporated. All rights reserved.
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

#include "appshell_extensions.h"

#include "appshell_extensions_platform.h"
#include "native_menu_model.h"
#include "appshell_node_process.h"
#include "config.h"

#include <algorithm>

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;

#ifndef _WINDOWS
//#include "machine_id.h"

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <sys/types.h>
#include <sys/ioctl.h>

#define DARWIN OS_MACOSX

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if_dl.h>
#include <ifaddrs.h>
#include <net/if_types.h>
#else //!DARWIN
#include <linux/if.h>
#include <linux/sockios.h>
#endif //!DARWIN

#include <sys/resource.h>
#include <sys/utsname.h>


void getMacHash(u16& mac1, u16& mac2);
u16 getVolumeHash();
u16 getCpuHash();
const char* getMachineName();

#ifndef WINDOWS
//#include "machine_id.h"

#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>

#include <sys/types.h>
#include <sys/ioctl.h>

//#define DARWIN MAC

#ifdef DARWIN
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if_dl.h>
#include <ifaddrs.h>
#include <net/if_types.h>
#else //!DARWIN
#include <linux/if.h>
#include <linux/sockios.h>
#endif //!DARWIN

#include <sys/resource.h>
#include <sys/utsname.h>

//---------------------------------get MAC addresses ---------------------------------
// we just need this for purposes of unique machine id. So any one or two
// mac's is fine.
u16 hashMacAddress( u8* mac )
{
    u16 hash = 0;
    
    for ( u32 i = 0; i < 6; i++ )
    {
        hash += ( mac[i] << (( i & 1 ) * 8 ));
    }
    return hash;
}

void getMacHash( u16& mac1, u16& mac2 )
{
    mac1 = 0;
    mac2 = 0;
    
#ifdef DARWIN
    
    struct ifaddrs* ifaphead;
    if ( getifaddrs( &ifaphead ) != 0 )
        return;
    
    // iterate over the net interfaces
    bool foundMac1 = false;
    struct ifaddrs* ifap;
    for ( ifap = ifaphead; ifap; ifap = ifap->ifa_next )
    {
        struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifap->ifa_addr;
        if ( sdl && ( sdl->sdl_family == AF_LINK ) && ( sdl->sdl_type == IFT_ETHER ))
        {
            if ( !foundMac1 )
            {
                foundMac1 = true;
                mac1 = hashMacAddress( (u8*)(LLADDR(sdl))); //sdl->sdl_data) + sdl->sdl_nlen) );
            } else {
                mac2 = hashMacAddress( (u8*)(LLADDR(sdl))); //sdl->sdl_data) + sdl->sdl_nlen) );
                break;
            }
        }
    }
    
    freeifaddrs( ifaphead );
    
#else // !DARWIN
    
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP );
    if ( sock < 0 ) return;
    
    // enumerate all IP addresses of the system
    struct ifconf conf;
    char ifconfbuf[ 128 * sizeof(struct ifreq)  ];
    memset( ifconfbuf, 0, sizeof( ifconfbuf ));
    conf.ifc_buf = ifconfbuf;
    conf.ifc_len = sizeof( ifconfbuf );
    if ( ioctl( sock, SIOCGIFCONF, &conf ))
    {
        assert(0);
        return;
    }
    
    // get MAC address
    bool foundMac1 = false;
    struct ifreq* ifr;
    for ( ifr = conf.ifc_req; (s8*)ifr < (s8*)conf.ifc_req + conf.ifc_len; ifr++ )
    {
        if ( ifr->ifr_addr.sa_data == (ifr+1)->ifr_addr.sa_data )
            continue;  // duplicate, skip it
        
        if ( ioctl( sock, SIOCGIFFLAGS, ifr ))
            continue;  // failed to get flags, skip it
        if ( ioctl( sock, SIOCGIFHWADDR, ifr ) == 0 )
        {
            if ( !foundMac1 )
            {
                foundMac1 = true;
                mac1 = hashMacAddress( (u8*)&(ifr->ifr_addr.sa_data));
            } else {
                mac2 = hashMacAddress( (u8*)&(ifr->ifr_addr.sa_data));
                break;
            }
        }
    }
    
    close( sock );
    
#endif // !DARWIN
    
    // sort the mac addresses. We don't want to invalidate
    // both macs if they just change order.
    if ( mac1 > mac2 )
    {
        u16 tmp = mac2;
        mac2 = mac1;
        mac1 = tmp;
    }
}

u16 getVolumeHash()
{
    // we don't have a 'volume serial number' like on windows.
    // Lets hash the system name instead.
    u8* sysname = (u8*)getMachineName();
    u16 hash = 0;
    
    for ( u32 i = 0; sysname[i]; i++ )
        hash += ( sysname[i] << (( i & 1 ) * 8 ));
    
    return hash;
}

#ifdef DARWIN
#include <mach-o/arch.h>
u16 getCpuHash()
{
    const NXArchInfo* info = NXGetLocalArchInfo();
    u16 val = 0;
    val += (u16)info->cputype;
    val += (u16)info->cpusubtype;
    return val;
}

#else // !DARWIN

static void getCpuid( u32* p, u32 ax )
{
    __asm __volatile
    (   "movl %%ebx, %%esi\n\t"
     "cpuid\n\t"
     "xchgl %%ebx, %%esi"
     : "=a" (p[0]), "=S" (p[1]),
     "=c" (p[2]), "=d" (p[3])
     : "0" (ax)
     );
}

u16 getCpuHash()
{
    u32 cpuinfo[4] = { 0, 0, 0, 0 };
    getCpuid( cpuinfo, 0 );
    u16 hash = 0;
    u32* ptr = (&cpuinfo[0]);
    for ( u32 i = 0; i < 4; i++ )
        hash += (ptr[i] & 0xFFFF) + ( ptr[i] >> 16 );
    
    return hash;
}
#endif // !DARWIN

const char* getMachineName()
{
    static struct utsname u;
    
    if ( uname( &u ) < 0 )
    {
        assert(0);
        return "unknown";
    }
    
    return u.nodename;
}

#endif


#ifdef _WINDOWS
#include <windows.h>
#include <intrin.h>
#include <iphlpapi.h>

typedef uint8_t   u8;
typedef uint16_t  u16;
typedef uint32_t  u32;



 // we just need this for purposes of unique machine id. 
 // So any one or two mac's is fine.
u16 hashMacAddress(PIP_ADAPTER_INFO info)
{
	u16 hash = 0;
	for (u32 i = 0; i < info->AddressLength; i++)
	{
		hash += (info->Address[i] << ((i & 1) * 8));
	}
	return hash;
}

void getMacHash(u16& mac1, u16& mac2)
{
	IP_ADAPTER_INFO AdapterInfo[32];
	DWORD dwBufLen = sizeof(AdapterInfo);

	DWORD dwStatus = GetAdaptersInfo(AdapterInfo, &dwBufLen);
	if (dwStatus != ERROR_SUCCESS)
		return; // no adapters.

	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
	mac1 = hashMacAddress(pAdapterInfo);
	if (pAdapterInfo->Next)
		mac2 = hashMacAddress(pAdapterInfo->Next);

	// sort the mac addresses. We don't want to invalidate
	// both macs if they just change order.
	if (mac1 > mac2)
	{
		u16 tmp = mac2;
		mac2 = mac1;
		mac1 = tmp;
	}
}

u16 getVolumeHash()
{
	DWORD serialNum = 0;

	// Determine if this volume uses an NTFS file system.
	GetVolumeInformation(/*L"c:\\"*/ NULL, NULL, 0, &serialNum, NULL, NULL, NULL, 0);
	u16 hash = (u16)((serialNum + (serialNum >> 16)) & 0xFFFF);

	return hash;
}

u16 getCpuHash()
{
	int cpuinfo[4] = { 0, 0, 0, 0 };
	__cpuid(cpuinfo, 0);
	u16 hash = 0;
	u16* ptr = (u16*)(&cpuinfo[0]);
	for (u32 i = 0; i < 8; i++)
		hash += ptr[i];

	return hash;
}

const char* getMachineName()
{
	static char computerName[1024];
	DWORD size = 1024;
	GetComputerNameA(computerName, &size);
	return &(computerName[0]);
}
#else
    void getMacHash( u16& mac1, u16& mac2 );
    u16 getVolumeHash();
    u16 getCpuHash();
    const char* getMachineName();

#endif

u16 mask[5] = { 0x4e25, 0xf4a1, 0x5437, 0xab41, 0x0000 };

static void smear(u16* id)
{
    for (u32 i = 0; i < 5; i++)
        for (u32 j = i; j < 5; j++)
            if (i != j)
                id[i] ^= id[j];
    
    for (u32 i = 0; i < 5; i++)
        id[i] ^= mask[i];
}

static void unsmear(u16* id)
{
    for (u32 i = 0; i < 5; i++)
        id[i] ^= mask[i];
    
    for (u32 i = 0; i < 5; i++)
        for (u32 j = 0; j < i; j++)
            if (i != j)
                id[4 - i] ^= id[4 - j];
}

static u16* computeSystemUniqueId()
{
    static u16 id[5];
    static bool computed = false;
    
    if (computed) return id;
    
    // produce a number that uniquely identifies this system.
    id[0] = getCpuHash();
    id[1] = getVolumeHash();
    getMacHash(id[2], id[3]);
    
    // fifth block is some checkdigits
    id[4] = 0;
    for (u32 i = 0; i < 4; i++)
        id[4] += id[i];
    
    smear(id);
    
    computed = true;
    return id;
}

std::string getSystemUniqueId()
{
    // get the name of the computer
    std::string buf;
    //buf  = getMachineName();
    
    u16* id = computeSystemUniqueId();
    for (u32 i = 0; i < 5; i++)
    {
        char num[16];
        snprintf(num, 16, "%x", id[i]);
        if (i > 0) {
            buf = buf + "-";
        }
        switch (strlen(num))
        {
            case 1: buf = buf + "000"; break;
            case 2: buf = buf + "00";  break;
            case 3: buf = buf + "0";   break;
        }
        buf = buf + num;
    }
    
    return buf;
    
    //const char* p = buf.c_str();
    
    //while (*p) { *p = toupper(*p); p++; }
    
    //return KxSymbol(buf.getBuffer()).string();
}


extern std::vector<CefString> gDroppedFiles;

namespace appshell_extensions {

class ProcessMessageDelegate : public ClientHandler::ProcessMessageDelegate {
public:
    ProcessMessageDelegate()
    {
    }

    // From ClientHandler::ProcessMessageDelegate.
    virtual bool OnProcessMessageReceived(
                      CefRefPtr<ClientHandler> handler,
                      CefRefPtr<CefBrowser> browser,
                      CefProcessId source_process,
                      CefRefPtr<CefProcessMessage> message) OVERRIDE {
        std::string message_name = message->GetName();
        CefRefPtr<CefListValue> argList = message->GetArgumentList();
        int32 callbackId = -1;
        int32 error = NO_ERROR;
        CefRefPtr<CefProcessMessage> response = 
            CefProcessMessage::Create("invokeCallback");
        CefRefPtr<CefListValue> responseArgs = response->GetArgumentList();
        
        // V8 extension messages are handled here. These messages come from the 
        // render process thread (in client_app.cpp), and have the following format:
        //   name - the name of the native function to call
        //   argument0 - the id of this message. This id is passed back to the
        //               render process in order to execute callbacks
        //   argument1 - argumentN - the arguments for the function
        //
        // Note: Functions without callback can be specified, but they *cannot*
        // take any arguments.
        
        // If we have any arguments, the first is the callbackId
        if (argList->GetSize() > 0) {
            callbackId = argList->GetInt(0);
            
            if (callbackId != -1)
                responseArgs->SetInt(0, callbackId);
        }
        
        if (message_name == "OpenLiveBrowser") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - argURL
            //  2: bool - enableRemoteDebugging
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_BOOL) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString argURL = argList->GetString(1);
                bool enableRemoteDebugging = argList->GetBool(2);
                error = OpenLiveBrowser(argURL, enableRemoteDebugging);
            }
            
        } else if (message_name == "CloseLiveBrowser") {
            // Parameters
            //  0: int32 - callback id
            if (argList->GetSize() != 1) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                CloseLiveBrowser(browser, response);
            
                // Skip standard callback handling. CloseLiveBrowser fires the
                // callback asynchronously.
                return true;
            }
        } else if (message_name == "ShowOpenDialog") {
            // Parameters:
            //  0: int32 - callback id
            //  1: bool - allowMultipleSelection
            //  2: bool - chooseDirectory
            //  3: string - title
            //  4: string - initialPath
            //  5: string - fileTypes (space-delimited string)
            if (argList->GetSize() != 6 ||
                argList->GetType(1) != VTYPE_BOOL ||
                argList->GetType(2) != VTYPE_BOOL ||
                argList->GetType(3) != VTYPE_STRING ||
                argList->GetType(4) != VTYPE_STRING ||
                argList->GetType(5) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
           
            if (error == NO_ERROR) {
                bool allowMultipleSelection = argList->GetBool(1);
                bool chooseDirectory = argList->GetBool(2);
                ExtensionString title = argList->GetString(3);
                ExtensionString initialPath = argList->GetString(4);
                ExtensionString fileTypes = argList->GetString(5);
                
#ifdef OS_MACOSX
                ShowOpenDialog(allowMultipleSelection,
                               chooseDirectory,
                               title,
                               initialPath,
                               fileTypes,
                               browser,
                               response);
                
                // Skip standard callback handling. ShowOpenDialog fires the
                // callback asynchronously.
                
                return true;
#else
                CefRefPtr<CefListValue> selectedFiles = CefListValue::Create();
                error = ShowOpenDialog(allowMultipleSelection,
                                       chooseDirectory,
                                       title,
                                       initialPath,
                                       fileTypes,
                                       selectedFiles);
                // Set response args for this function
                responseArgs->SetList(2, selectedFiles);
#endif
                
            }
            
        } else if (message_name == "ShowSaveDialog") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - title
            //  2: string - initialPath
            //  3: string - poposedNewFilename
            if (argList->GetSize() != 4 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING ||
                argList->GetType(3) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }

            if (error == NO_ERROR) {
                ExtensionString title = argList->GetString(1);
                ExtensionString initialPath = argList->GetString(2);
                ExtensionString proposedNewFilename = argList->GetString(3);
                
#ifdef OS_MACOSX
                // Skip standard callback handling. ShowSaveDialog fires the
                // callback asynchronously.
                ShowSaveDialog(title,
                               initialPath,
                               proposedNewFilename,
                               browser,
                               response);
                return true;
#else
                ExtensionString newFilePath;
                error = ShowSaveDialog(title,
                                       initialPath,
                                       proposedNewFilename,
                                       newFilePath);
                
                // Set response args for this function
                responseArgs->SetString(2, newFilePath);
#endif
            }

        } else if (message_name == "IsNetworkDrive") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - directory path
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            bool isRemote = false;
            if (error == NO_ERROR) {
                ExtensionString path = argList->GetString(1);                
                error = IsNetworkDrive(path, isRemote);
            }
            
            // Set response args for this function
            responseArgs->SetBool(2, isRemote);
        } else if (message_name == "ReadDir") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - directory path
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            CefRefPtr<CefListValue> directoryContents = CefListValue::Create();
            
            if (error == NO_ERROR) {
                ExtensionString path = argList->GetString(1);
                
                error = ReadDir(path, directoryContents);
            }
            
            // Set response args for this function
            responseArgs->SetList(2, directoryContents);
        } else if (message_name == "MakeDir") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - directory path
            //  2: number - mode
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_INT) {
                error = ERR_INVALID_PARAMS;
            }
          
            if (error == NO_ERROR) {
                ExtensionString pathname = argList->GetString(1);
                int32 mode = argList->GetInt(2);
              
                error = MakeDir(pathname, mode);
            }
            // No additional response args for this function
        } else if (message_name == "Rename") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - old path
            //  2: string - new path
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
          
            if (error == NO_ERROR) {
                ExtensionString oldName = argList->GetString(1);
                ExtensionString newName = argList->GetString(2);
            
                error = Rename(oldName, newName);
            }
          // No additional response args for this function
        } else if (message_name == "GetFileInfo") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString filename = argList->GetString(1);
                ExtensionString realPath;
                uint32 modtime;
                double size;
                bool isDir;
                
                error = GetFileInfo(filename, modtime, isDir, size, realPath);
                
                // Set response args for this function
                responseArgs->SetInt(2, modtime);
                responseArgs->SetBool(3, isDir);
                responseArgs->SetInt(4, size);
                responseArgs->SetString(5, realPath);
            }
        } else if (message_name == "ReadFile") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            //  2: string - encoding
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString filename = argList->GetString(1);
                ExtensionString encoding = argList->GetString(2);
                std::string contents = "";
                
                error = ReadFile(filename, encoding, contents);
                
                // Set response args for this function
                responseArgs->SetString(2, contents);
            }
        } else if (message_name == "WriteFile") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            //  2: string - data
            //  3: string - encoding
            if (argList->GetSize() != 4 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING ||
                argList->GetType(3) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString filename = argList->GetString(1);
                std::string contents = argList->GetString(2);
                ExtensionString encoding = argList->GetString(3);
                
                error = WriteFile(filename, contents, encoding);
                // No additional response args for this function
            }
        } else if (message_name == "SetPosixPermissions") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            //  2: int - mode
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_INT) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString filename = argList->GetString(1);
                int32 mode = argList->GetInt(2);
                
                error = SetPosixPermissions(filename, mode);
                
                // No additional response args for this function
            }
        } else if (message_name == "DeleteFileOrDirectory") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString filename = argList->GetString(1);
                
                error = DeleteFileOrDirectory(filename);
                
                // No additional response args for this function
            }
        } else if (message_name == "MoveFileOrDirectoryToTrash") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - path
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString path = argList->GetString(1);
                
                MoveFileOrDirectoryToTrash(path, browser, response);
                
                // Skip standard callback handling. MoveFileOrDirectoryToTrash fires the
                // callback asynchronously.
                return true;
            }
        } else if (message_name == "ShowDeveloperTools") {
            // Parameters - none
            CefWindowInfo wi;
            CefBrowserSettings settings;

#if defined(OS_WIN)
            wi.SetAsPopup(NULL, "DevTools");
#endif
            browser->GetHost()->ShowDevTools(wi, browser->GetHost()->GetClient(), settings, CefPoint());

        } else if (message_name == "GetNodeState") {
            // Parameters:
            //  0: int32 - callback id
            if (argList->GetSize() != 1) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                int32 port = 0;
                int32 portOrErr = getNodeState();
                if (portOrErr < 0) { // there is an error
                    error = portOrErr;
                } else {
                    port = portOrErr;
                    error = NO_ERROR;
                }
                responseArgs->SetInt(2, port);
            }
            
        } else if (message_name == "QuitApplication") {
            // Parameters - none

            // The DispatchCloseToNextBrowser() call initiates a quit sequence. The app will
            // quit if all browser windows are closed.
            handler->DispatchCloseToNextBrowser();

        } else if (message_name == "AbortQuit") {
            // Parameters - none
          
            handler->AbortQuit();
        } else if (message_name == "OpenURLInDefaultBrowser") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - url
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString url = argList->GetString(1);
                
                error = OpenURLInDefaultBrowser(url);
                
                // No additional response args for this function
            }
        } else if (message_name == "ShowOSFolder") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - path
            
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
          
            if (error == NO_ERROR) {
                ExtensionString path = argList->GetString(1);
                error = ShowFolderInOSWindow(path);
            }
        } else if (message_name == "GetPendingFilesToOpen") {
            // Parameters:
            //  0: int32 - callback id
            if (argList->GetSize() != 1) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString files;
                error = GetPendingFilesToOpen(files);
                responseArgs->SetString(2, files.c_str());
            }
        } else if (message_name == "CopyFile") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - filename
            //  2: string - dest filename
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString src = argList->GetString(1);
                ExtensionString dest = argList->GetString(2);
                
                error = CopyFile(src, dest);
                // No additional response args for this function
            }
        } else if (message_name == "GetDroppedFiles") {
            // Parameters:
            //  0: int32 - callback id
            if (argList->GetSize() != 1) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                std::wstring files;
                
                files = L"[";
                for (unsigned int i = 0; i < gDroppedFiles.size(); i++) {
                    std::wstring file(gDroppedFiles[i]);
                    // Convert windows paths to unix paths
                    replace(file.begin(), file.end(), '\\', '/');
                    files += L"\"";
                    files += file;
                    files += L"\"";
                    if (i < gDroppedFiles.size() - 1) {
                        files += L", ";
                    }
                }
                files += L"]";
                gDroppedFiles.clear();
                
                responseArgs->SetString(2, files.c_str());
            }
            
        } else if (message_name == "AddMenu") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - menuTitle to display
            //  2: string - menu/command ID
            //  3: string - position - first, last, before, after
            //  4: string - relativeID - ID of other element relative to which this should be positioned (for position before and after)
            if (argList->GetSize() != 5 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING ||
                argList->GetType(3) != VTYPE_STRING ||
                argList->GetType(4) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString menuTitle = argList->GetString(1);
                ExtensionString command = CefString(argList->GetString(2));
                ExtensionString position = CefString(argList->GetString(3));
                ExtensionString relativeId = CefString(argList->GetString(4));
                
                error = AddMenu(browser, menuTitle, command, position, relativeId);
                // No additional response args for this function
            }
        } else if (message_name == "AddMenuItem") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - parent menu this is part of
            //  2: string - menuTitle to display
            //  3: string - command ID
            //  4: string - keyboard shortcut
            //  5: string - display string
            //  6: string - position - first, last, before, after
            //  7: string - relativeID - ID of other element relative to which this should be positioned (for position before and after)
            if (argList->GetSize() != 8 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING ||
                argList->GetType(3) != VTYPE_STRING ||
                argList->GetType(4) != VTYPE_STRING ||
                argList->GetType(5) != VTYPE_STRING ||
                argList->GetType(6) != VTYPE_STRING ||
                argList->GetType(7) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString parentCommand = argList->GetString(1);
                ExtensionString menuTitle = argList->GetString(2);
                ExtensionString command = argList->GetString(3);
                ExtensionString key = argList->GetString(4);
                ExtensionString displayStr = argList->GetString(5);
                ExtensionString position = argList->GetString(6);
                ExtensionString relativeId = argList->GetString(7);

                error = AddMenuItem(browser, parentCommand, menuTitle, command, key, displayStr, position, relativeId);
                // No additional response args for this function
            }
        } else if (message_name == "RemoveMenu") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - command ID
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString commandId;
                commandId = argList->GetString(1);
                error = RemoveMenu(browser, commandId);
            }
            // No response args for this function
        } else if (message_name == "RemoveMenuItem") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - command ID
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString commandId;
                commandId = argList->GetString(1);
                error = RemoveMenuItem(browser, commandId);
            }
            // No response args for this function
        } else if (message_name == "GetMenuItemState") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - menu/command ID
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString commandId = CefString(argList->GetString(1));
                ExtensionString menuTitle;
                bool checked, enabled;
                int index;
                error = GetMenuItemState(browser, commandId, enabled, checked, index);
                responseArgs->SetBool(2, enabled);
                responseArgs->SetBool(3, checked);
                responseArgs->SetInt(4, index);
            }
        }  else if(message_name == "SetMenuItemState") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - commandName
            //  2: bool - enabled
            //  3: bool - checked
            if (argList->GetSize() != 4 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_BOOL ||
                argList->GetType(3) != VTYPE_BOOL) {
                error = ERR_INVALID_PARAMS;
            }

            if (error == NO_ERROR) {
                ExtensionString command = argList->GetString(1);
                bool enabled = argList->GetBool(2);
                bool checked = argList->GetBool(3);
                error = NativeMenuModel::getInstance(getMenuParent(browser)).setMenuItemState(command, enabled, checked);
            }
        } else if (message_name == "SetMenuTitle") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - menu/command ID
            //  2: string - menuTitle to display
            if (argList->GetSize() != 3 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING) {
                    error = ERR_INVALID_PARAMS;
            }

            if (error == NO_ERROR) {
                ExtensionString command = CefString(argList->GetString(1));
                ExtensionString menuTitle = argList->GetString(2);

                error = SetMenuTitle(browser, command, menuTitle);
                // No additional response args for this function
            }
        }  else if (message_name == "GetMenuTitle") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - menu/command ID
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString commandId = CefString(argList->GetString(1));
                ExtensionString menuTitle;
                error = GetMenuTitle(browser, commandId, menuTitle);
                responseArgs->SetString(2, menuTitle);
            }
        } else if (message_name == "SetMenuItemShortcut") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - command ID
            //  2: string - shortcut
            //  3: string - display string
            if (argList->GetSize() != 4 ||
                argList->GetType(1) != VTYPE_STRING ||
                argList->GetType(2) != VTYPE_STRING ||
                argList->GetType(3) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString commandId = argList->GetString(1);
                ExtensionString shortcut = argList->GetString(2);
                ExtensionString displayStr = argList->GetString(3);
                
                error = SetMenuItemShortcut(browser, commandId, shortcut, displayStr);
                // No additional response args for this function
            }
        } else if (message_name == "GetMenuPosition") {
            // Parameters:
            //  0: int32 - callback id
            //  1: string - menu/command ID
            if (argList->GetSize() != 2 ||
                argList->GetType(1) != VTYPE_STRING) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                ExtensionString commandId;
                ExtensionString parentId;
                int index;
                commandId = argList->GetString(1);
                error = GetMenuPosition(browser, commandId, parentId, index);
                responseArgs->SetString(2, parentId);
                responseArgs->SetInt(3, index);
            }
        } else if (message_name == "DragWindow") {     
            // Parameters: none       
            DragWindow(browser);
        } else if (message_name == "GetZoomLevel") {
            // Parameters:
            //  0: int32 - callback id
            double zoomLevel = browser->GetHost()->GetZoomLevel();

            responseArgs->SetDouble(2, zoomLevel);
		} else if (message_name == "GetMachineHash") {
			// Parameters:
			//  0: int32 - callback id
			//double zoomLevel = browser->GetHost()->GetZoomLevel();
			
			responseArgs->SetString(2, getSystemUniqueId());
		} else if (message_name == "SetZoomLevel") {
            // Parameters:
            //  0: int32 - callback id
            //  1: int32 - zoom level

            if (argList->GetSize() != 2 ||
                !(argList->GetType(1) == VTYPE_INT || argList->GetType(1) == VTYPE_DOUBLE)) {
                error = ERR_INVALID_PARAMS;
            }

            if (error == NO_ERROR) {
                // cast to double
                double zoomLevel = 1;
                
                if (argList->GetType(1) == VTYPE_DOUBLE) {
                    zoomLevel = argList->GetDouble(1);
                } else {
                    zoomLevel = argList->GetInt(1);
                }

                browser->GetHost()->SetZoomLevel(zoomLevel);
            }
        }
        else if (message_name == "InstallCommandLineTools") {
            // Parameters:
            //  0: int32 - callback id
            if (argList->GetSize() != 1) {
                error = ERR_INVALID_PARAMS;
            }
            
            if (error == NO_ERROR) {
                error = InstallCommandLineTools();
            }

        } else {
            fprintf(stderr, "Native function not implemented yet: %s\n", message_name.c_str());
            return false;
        }
      
        if (callbackId != -1) {
            responseArgs->SetInt(1, error);
          
            // Send response
            browser->SendProcessMessage(PID_RENDERER, response);
        }
      
        return true;
    }
  
    IMPLEMENT_REFCOUNTING(ProcessMessageDelegate);
};
    
void CreateProcessMessageDelegates(ClientHandler::ProcessMessageDelegateSet& delegates) {
    delegates.insert(new ProcessMessageDelegate);
}

} // namespace appshell_extensions
