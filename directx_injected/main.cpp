#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <stdio.h>
#include "stdint.h"


#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

volatile DWORD dwEndscene_hook;
volatile DWORD dwEndscene_ret;

HINSTANCE hDllModule;

DWORD *vTable;
volatile DWORD dmghook_ret;

LPD3DXFONT pFont;

static volatile LPDIRECT3DDEVICE9 pDevice;

VOID WriteText(LPDIRECT3DDEVICE9 pDevice, INT x, INT y, DWORD color, CHAR *text)
{
  RECT rect;
  SetRect(&rect, x, y, x, y);
  pFont->DrawText(NULL, text, -1, &rect, DT_NOCLIP | DT_LEFT, color);
}

VOID WINAPI BeforeEndScene(LPDIRECT3DDEVICE9 pDevice)
{
  //The custom function which is called before calling EndScene() 
  D3DXCreateFont(pDevice, 30, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &pFont);
  WriteText(pDevice, 0, 0, D3DCOLOR_ARGB(255, 255, 0, 0), "Hello World!");  //Display Hello World
  pFont->Release();
}

DWORD HookVMT(void *address, void *newfunction)
{
  DWORD NewProtection;
  DWORD retval;
  VirtualProtect(address, 4, PAGE_EXECUTE_READWRITE, &NewProtection);
  memcpy(&retval, address, 4);
  memcpy(address, newfunction, 4);
  VirtualProtect(address, 4, NewProtection, &NewProtection);
  return retval;
}

// The function which the VMT hook jumps to
__declspec(naked) void hook()
{
  static volatile LPDIRECT3DDEVICE9 pDevice;
  // get arguments from registers
  __asm {
    // Push registers
    pushad
    pushfd
    push esi
    // Get LPDIRECT3DDEVICE9 for drawing from stack
    mov esi, dword ptr ss : [esp + 0x2c];
    mov pDevice, esi;
  }

  {
    BeforeEndScene(pDevice);
  }
  __asm {
    // Clean up stack/registers. Everything must look like the function was never hooked
    pop esi
    popfd
    popad
    jmp[dwEndscene_ret] // jump to original EndScene
  }
}

bool Mask(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
  for (; *szMask; ++szMask, ++pData, ++bMask)
    if (*szMask == 'x' && *pData != *bMask)
      return false;
  return (*szMask) == NULL;
}

DWORD FindPattern(DWORD dwAddress, DWORD dwLen, BYTE *bMask, char * szMask)
{
  for (DWORD i = 0; i<dwLen; i++)
    if (Mask((BYTE*)(dwAddress + i), bMask, szMask))
      return (DWORD)(dwAddress + i);
  return 0;
}

void MyHook(void)
{
  DWORD hD3D = NULL;
  //Find VMT in memory
  while (!hD3D) hD3D = (DWORD)GetModuleHandle("d3d9.dll");
  DWORD PPPDevice = FindPattern(hD3D, 0x128000, (PBYTE)"\xC7\x06\x00\x00\x00\x00\x89\x86\x00\x00\x00\x00\x89\x86", "xx????xx????xx");
  //Copy VMT address
  memcpy(&vTable, (void *)(PPPDevice + 2), 4);

  DWORD NewProtection;
  // Hook the EndScene function in VMT; straightforward by replacing the function pointer
  VirtualProtect(&vTable[42], 4, PAGE_EXECUTE_READWRITE, &NewProtection);
  dwEndscene_ret = vTable[42];  //Save original function pointer for EndScene
  vTable[42] = (DWORD)&hook;    //Replace VMT function pointer with the one to the function "hook" in this code
  VirtualProtect(&vTable[42], 4, NewProtection, &NewProtection);

  //Infinite loop
  while (1)
  {
    Sleep(1000);
  }
}

//Entry point
BOOL WINAPI DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpvReserved)
{
  if (dwReason == DLL_PROCESS_ATTACH)
  {
    DisableThreadLibraryCalls(hModule);
    CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)MyHook, NULL, NULL, NULL);
  }
  return TRUE;
}