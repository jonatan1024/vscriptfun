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

#pragma once
#include <extension.h>
#include <UtlStringMap.h>
#include <vscript/ivscript.h>
#include <utlbuffer.h>

class CFunctions {
public:
	const ScriptFunctionBinding_t * LookupFunction(const char * classname, const char * funcname);
	ScriptVariant_t CallFunction(const char * classname, const char * funcname, void * context, const ScriptVariant_t * args);
	ScriptVariant_t CallFunction(const ScriptFunctionBinding_t* func, void * context, const ScriptVariant_t * args);

	cell_t PawnCallFunction(const char * classname, const char * funcname, IPluginContext *pContext, cell_t *params);

	void DumpNatives(CUtlBuffer& nativesBuffer);
	void CreateInclude(CUtlBuffer& includeBuffer);

	void Clear();

	struct CClass {
		CUtlStringMap<const ScriptFunctionBinding_t*> m_functions;
		ScriptClassDesc_t * m_descriptor = NULL;
		void * m_instance = NULL;
	};
private:
	void RegisterVM(IScriptVM * svm);
	void OnRegisterFunction(ScriptFunctionBinding_t *pScriptFunction);
	bool OnRegisterClass(ScriptClassDesc_t* pClassDesc);
	HSCRIPT OnRegisterInstance(ScriptClassDesc_t *pDesc, void *pInstance);
	bool OnSetValue(HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value);

	CUtlVector<IScriptVM*> m_svm;
	CUtlStringMap<CClass> m_classes;
	ScriptClassDesc_t * m_lastClass = NULL;
	void * m_lastInstance = NULL;

	friend bool CExtension::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late);
};