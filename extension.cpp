/**
 * vim: set ts=4 :
 * =============================================================================
 * SourceMod VScript Functions Extension
 * Copyright (C) 2004-2008 AlliedModders LLC.  All rights reserved.
 * =============================================================================
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * As a special exception, AlliedModders LLC gives you permission to link the
 * code of this program (as well as its derivative works) to "Half-Life 2," the
 * "Source Engine," the "SourcePawn JIT," and any Game MODs that run on software
 * by the Valve Corporation.  You must obey the GNU General Public License in
 * all respects for all other code used.  Additionally, AlliedModders LLC grants
 * this exception to all derivative works.  AlliedModders LLC defines further
 * exceptions, found in LICENSE.txt (as of this writing, version JULY-31-2007),
 * or <http://www.sourcemod.net/license.php>.
 *
 * Version: $Id$
 */

#include "extension.h"
#include "functions.h"
#include <vscript/ivscript.h>
#include <filesystem.h>
#include <utlbuffer.h>

/**
 * @file extension.cpp
 * @brief Implement extension code here.
 */

CExtension g_pExtension;
SMEXT_LINK(&g_pExtension);

IScriptManager * g_pScriptManager;

CFunctions g_Functions;

sp_nativeinfo_t * g_pNatives;
CUtlStringList g_NativeNames;

#define ONLOAD_ASSERT(expr, message) if(!(expr)){\
	snprintf(error, maxlen, message);\
	return false;\
}

SH_DECL_HOOK1_void(IScriptVM, RegisterFunction, SH_NOATTRIB, 0, ScriptFunctionBinding_t*);
SH_DECL_HOOK1(IScriptVM, RegisterClass, SH_NOATTRIB, 0, bool, ScriptClassDesc_t*);
SH_DECL_HOOK2(IScriptVM, RegisterInstance, SH_NOATTRIB, 0, HSCRIPT, ScriptClassDesc_t*, void*);
SH_DECL_HOOK3(IScriptVM, SetValue, SH_NOATTRIB, 0, bool, HSCRIPT, const char *, const ScriptVariant_t&);
int RegisterFunctionHookId;
int RegisterClassHookId;
int RegisterInstanceHookId;
int SetValueHookId;

bool CExtension::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late) {
	GET_V_IFACE_ANY(GetEngineFactory, g_pScriptManager, IScriptManager, VSCRIPT_INTERFACE_VERSION);
	ONLOAD_ASSERT(g_pScriptManager, "Couldn't get IScriptManager!");

	GET_V_IFACE_ANY(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	ONLOAD_ASSERT(g_pFullFileSystem, "Couldn't get IFileSystem!");

	IScriptVM * svm = g_pScriptManager->CreateVM();
	ONLOAD_ASSERT(g_pScriptManager, "Couldn't create IScriptVM!");
	RegisterFunctionHookId = SH_ADD_VPHOOK(IScriptVM, RegisterFunction, svm, SH_MEMBER(&g_Functions, &CFunctions::OnRegisterFunction), true);
	RegisterClassHookId = SH_ADD_VPHOOK(IScriptVM, RegisterClass, svm, SH_MEMBER(&g_Functions, &CFunctions::OnRegisterClass), true);
	RegisterInstanceHookId = SH_ADD_VPHOOK(IScriptVM, RegisterInstance, svm, SH_MEMBER(&g_Functions, &CFunctions::OnRegisterInstance), true);
	SetValueHookId = SH_ADD_VPHOOK(IScriptVM, SetValue, svm, SH_MEMBER(&g_Functions, &CFunctions::OnSetValue), true);
	g_pScriptManager->DestroyVM(svm);
	return true;
}

void CExtension::SDK_OnUnload() {
	SH_REMOVE_HOOK_ID(RegisterFunctionHookId);
	SH_REMOVE_HOOK_ID(RegisterClassHookId);
	SH_REMOVE_HOOK_ID(RegisterInstanceHookId);
	SH_REMOVE_HOOK_ID(SetValueHookId);
	if(g_pNatives)
		delete[] g_pNatives;
}

class FunctionNameListener : public IDebugListener {
public:
	virtual inline void OnDebugSpew(const char *msg, ...) {	}
	virtual inline void ReportError(const IErrorReport &report, IFrameIterator &iter) {	m_funcname = iter.FunctionName();	}
	inline const char * GetFunctionName() { return m_funcname; }
private:
	const char * m_funcname = NULL;
};

cell_t NativeHandler(IPluginContext *pContext, const cell_t *params) {
	//this is very ugly black magic
	static FunctionNameListener listener;
	pContext->APIv2()->SetDebugListener(&listener);
	pContext->ReportErrorNumber(SP_ERROR_NONE);
	//we did all this just to get the name of this native
	const char * fullname = listener.GetFunctionName();
	if (!fullname)
		return pContext->ThrowNativeError("Couldn't grab native function name!");

	char localname[256];
	const char * funcname;
	V_strncpy(localname, fullname, sizeof(localname));
	funcname = (const char*)V_strnchr(localname, '.', 256);
	if (!funcname)
		return pContext->ThrowNativeError("Couldn't split \"%s\" into classname and funcname!", fullname);

	int dotpos = funcname - localname;
	funcname++;
	localname[dotpos] = 0;
	if (dotpos < 3)
		return pContext->ThrowNativeError("Invalid classname \"%s\" (%s)!", localname, fullname);

	const char * classname = localname + (dotpos > 3 ? 4 : 3);

	return g_Functions.PawnCallFunction(classname, funcname, pContext, params);
}

void CExtension::SDK_OnAllLoaded() {
	char nativesPath[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_SM, nativesPath, sizeof(nativesPath), "data/vscriptfun/natives");
	if (g_pFullFileSystem->FileExists(nativesPath)) {
		CUtlBuffer nativesBuffer(0, 4096, CUtlBuffer::TEXT_BUFFER);
		g_pFullFileSystem->ReadFile(nativesPath, NULL, nativesBuffer);

		char native[256];
		while (true) {
			nativesBuffer.GetLine(native, sizeof(native));
			if (!native[0])
				break;
			for (int i = V_strlen(native)-1; i >= 0 && (native[i] == '\r' || native[i] == '\n'); i--)
				native[i] = 0;
			g_NativeNames.CopyAndAddToTail(native);
		}
		g_pNatives = new sp_nativeinfo_t[g_NativeNames.Count() + 1];
		FOR_EACH_VEC(g_NativeNames, it) {
			g_pNatives[it].func = NativeHandler;
			g_pNatives[it].name = g_NativeNames[it];
		}
		g_pNatives[g_NativeNames.Count()] = sp_nativeinfo_t();
		sharesys->AddNatives(myself, g_pNatives);
	}
}

void CExtension::OnCoreMapStart(edict_t *pEdictList, int edictCount, int clientMax) {
	char nativesPath[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_SM, nativesPath, sizeof(nativesPath), "data/vscriptfun/natives");
	if (g_pFullFileSystem->FileExists(nativesPath))
		return;

	smutils->BuildPath(Path_SM, nativesPath, sizeof(nativesPath), "data/vscriptfun");
	g_pFullFileSystem->CreateDirHierarchy(nativesPath);

	smutils->BuildPath(Path_SM, nativesPath, sizeof(nativesPath), "data/vscriptfun/natives");
	CUtlBuffer nativesBuffer(0, 4096, CUtlBuffer::TEXT_BUFFER);
	g_Functions.DumpNatives(nativesBuffer);
	g_pFullFileSystem->WriteFile(nativesPath, NULL, nativesBuffer);

	char includePath[PLATFORM_MAX_PATH];
	smutils->BuildPath(Path_SM, includePath, sizeof(includePath), "scripting/include");
	g_pFullFileSystem->CreateDirHierarchy(includePath);
	smutils->BuildPath(Path_SM, includePath, sizeof(includePath), "scripting/include/vscriptfun.inc");
	CUtlBuffer includeBuffer(0, 4096, CUtlBuffer::TEXT_BUFFER);
	g_Functions.CreateInclude(includeBuffer);
	g_pFullFileSystem->WriteFile(includePath, NULL, includeBuffer);
}

void CExtension::OnCoreMapEnd() {
	g_Functions.Clear();
}