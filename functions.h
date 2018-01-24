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

	cell_t PawnCallFunction(const char * classname, const char * funcname, IPluginContext *pContext, const cell_t *params);

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