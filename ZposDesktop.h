// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the ZPOSDESKTOP_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// ZPOSDESKTOP_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef ZPOSDESKTOP_EXPORTS
#define ZPOSDESKTOP_API __declspec(dllexport)
#else
#define ZPOSDESKTOP_API __declspec(dllimport)
#endif

// This class is exported from the dll
class ZPOSDESKTOP_API CZposDesktop {
public:
	CZposDesktop(void);
	// TODO: add your methods here.
};

extern ZPOSDESKTOP_API int nZposDesktop;

ZPOSDESKTOP_API int fnZposDesktop(void);
