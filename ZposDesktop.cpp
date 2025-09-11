// ZposDesktop.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "ZposDesktop.h"


// This is an example of an exported variable
ZPOSDESKTOP_API int nZposDesktop=0;

// This is an example of an exported function.
ZPOSDESKTOP_API int fnZposDesktop(void)
{
    return 0;
}

// This is the constructor of a class that has been exported.
CZposDesktop::CZposDesktop()
{
    return;
}
