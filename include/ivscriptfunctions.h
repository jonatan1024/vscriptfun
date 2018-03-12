#pragma once

#include <IShareSys.h>
#include <vscript/ivscript.h>

#define SMINTERFACE_VSCRIPTFUNCTIONS_NAME "IVScriptFunctions"
#define SMINTERFACE_VSCRIPTFUNCTIONS_VERSION 1

class IVScriptFunctions : public SourceMod::SMInterface
{
public:
	virtual const char *GetInterfaceName() { return SMINTERFACE_VSCRIPTFUNCTIONS_NAME; }
	virtual unsigned int GetInterfaceVersion() { return SMINTERFACE_VSCRIPTFUNCTIONS_VERSION; }

	virtual const ScriptFunctionBinding_t * LookupFunction(const char * classname, const char * funcname) = 0;
	virtual ScriptVariant_t CallFunction(const char * classname, const char * funcname, void * context, const ScriptVariant_t * args) = 0;
	virtual ScriptVariant_t CallFunction(const ScriptFunctionBinding_t* func, void * context, const ScriptVariant_t * args) = 0;
};