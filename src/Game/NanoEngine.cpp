#include "NanoEngine.h"

//==============================================================================
// GPU Config
//==============================================================================
#pragma region Gpu Config
// Use discrete GPU by default.
extern "C"
{
	// http://developer.download.nvidia.com/devzone/devcenter/gamegraphics/files/OptimusRenderingPolicies.pdf
	__declspec(dllexport) unsigned long NvOptimusEnablement = 0x00000001;

	// https://gpuopen.com/learn/amdpowerxpressrequesthighperformance/
	__declspec(dllexport) unsigned long AmdPowerXpressRequestHighPerformance = 0x00000001;
}
#pragma endregion
//==============================================================================
// GPU Config
//==============================================================================

//==============================================================================
// GLOBAL VARS
//==============================================================================
#pragma region Global Vars
namespace 
{
	struct
	{
		bool IsEnd = false;
	} Engine;
}

void ResetGlobalVars()
{
	Engine.IsEnd = false;
}
#pragma endregion
//==============================================================================
// END GLOBAL VARS
//==============================================================================

//==============================================================================
// LOG
//==============================================================================
#pragma region Log
void Print(const std::string& text)
{
	puts(text.c_str());
}
void Warning(const std::string& text)
{
	Print("WARNING: " + text);
}
void Error(const std::string& text)
{
	Print("ERROR: " + text);
}
void Fatal(const std::string& text)
{
	Print("FATAL: " + text);
	Engine.IsEnd = true;
}
#pragma endregion
//==============================================================================
// END LOG
//==============================================================================