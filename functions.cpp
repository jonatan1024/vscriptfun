#include "functions.h"
#include <utlhash.h>

void CFunctions::RegisterVM(IScriptVM * svm)
{
	FOR_EACH_VEC(m_svm, it)
		if (m_svm[it] == svm)
			return;
	m_svm.AddToTail(svm);
}

void CFunctions::OnRegisterFunction(ScriptFunctionBinding_t* pScriptFunction) {
	RegisterVM(META_IFACEPTR(IScriptVM));
	CClass& globalClass = m_classes[""];
	globalClass.m_functions[pScriptFunction->m_desc.m_pszScriptName] = pScriptFunction;
	RETURN_META(MRES_IGNORED);
}

bool CFunctions::OnRegisterClass(ScriptClassDesc_t* pClassDesc) {
	RegisterVM(META_IFACEPTR(IScriptVM));
	CClass& newClass = m_classes[pClassDesc->m_pszScriptName];
	newClass.m_descriptor = pClassDesc;
	FOR_EACH_VEC(pClassDesc->m_FunctionBindings, it) {
		const ScriptFunctionBinding_t * function = &pClassDesc->m_FunctionBindings[it];
		newClass.m_functions[function->m_desc.m_pszScriptName] = function;
	}
	RETURN_META_VALUE(MRES_IGNORED, false);
}

HSCRIPT CFunctions::OnRegisterInstance(ScriptClassDesc_t *pDesc, void *pInstance) {
	m_lastClass = pDesc;
	m_lastInstance = pInstance;
	RETURN_META_VALUE(MRES_IGNORED, NULL);
}

bool CFunctions::OnSetValue(HSCRIPT hScope, const char *pszKey, const ScriptVariant_t &value) {
	if (m_lastClass && m_lastInstance && value.m_type == FIELD_HSCRIPT) {
		void* instance = META_IFACEPTR(IScriptVM)->GetInstanceValue(value, m_lastClass);
		if (instance == m_lastInstance)
			m_classes[m_lastClass->m_pszScriptName].m_instance = m_lastInstance;
	}
	m_lastClass = NULL;
	m_lastInstance = NULL;
	RETURN_META_VALUE(MRES_IGNORED, false);
}

const ScriptFunctionBinding_t * CFunctions::LookupFunction(const char * classname, const char * funcname) {
	return m_classes[classname].m_functions[funcname];
}

ScriptVariant_t CFunctions::CallFunction(const char * classname, const char * funcname, void * context, const ScriptVariant_t * args) {
	const ScriptFunctionBinding_t * func = LookupFunction(classname, funcname);
	return CallFunction(func, context, args);
}

ScriptVariant_t CFunctions::CallFunction(const ScriptFunctionBinding_t * func, void * context, const ScriptVariant_t * args) {
	ScriptVariant_t ret;
	//passing a non-NULL return value pointer for void-returning functions caused crashes
	ScriptVariant_t * retPtr = ((func->m_desc.m_ReturnType) ? &ret : 0);
	bool ok = func->m_pfnBinding(func->m_pFunction, context, (ScriptVariant_t*)args, func->m_desc.m_Parameters.Count(), retPtr);
	Assert(ok);
	return ret;
}

void CFunctions::DumpNatives(CUtlBuffer& nativesBuffer) {
	for (UtlSymId_t cit = 0; cit < m_classes.GetNumStrings(); cit++) {
		const char * classname = m_classes.String(cit);
		const CUtlStringMap<const ScriptFunctionBinding_t*>& functions = m_classes[cit].m_functions;
		for (UtlSymId_t fit = 0; fit < functions.GetNumStrings(); fit++) {
			const char * funcname = functions.String(fit);
			nativesBuffer.Printf("%s%s.%s\n", classname[0] ? "VSF_" : "VSF", classname, funcname);
		}
	}
}

inline const char * SourcePawnTypeName(int16 eType)
{
	switch (eType)
	{
	case FIELD_VOID:	return "void";
	case FIELD_FLOAT:	return "float";
	case FIELD_CSTRING:	return "char[]";
	case FIELD_VECTOR:	return "float[]";
	case FIELD_INTEGER:	return "int";
	case FIELD_BOOLEAN:	return "bool";
	case FIELD_CHARACTER:	return "char";
	case FIELD_HSCRIPT:	return "HScript";
	default:	return "int";
	}
}

inline bool IsArrayType(int16 eType) {
	switch (eType)
	{
	case FIELD_CSTRING:	return true;
	case FIELD_VECTOR:	return true;
	default:	return false;
	}
}

void CreateFunctionHeader(CUtlBuffer& includeBuffer, const ScriptFunctionBinding_t * function, bool global) {
	const ScriptFuncDescriptor_t& descriptor = function->m_desc;

	//prepend description
	if (descriptor.m_pszDescription && descriptor.m_pszDescription[0])
		includeBuffer.Printf("\t//%s\n", descriptor.m_pszDescription);

	//decorate with original data types
	includeBuffer.Printf("\t//%s %s(", ScriptFieldTypeName(descriptor.m_ReturnType), descriptor.m_pszScriptName);
	FOR_EACH_VEC(descriptor.m_Parameters, it) {
		includeBuffer.PutString(ScriptFieldTypeName(descriptor.m_Parameters[it]));
		if (it < descriptor.m_Parameters.Count() - 1)
			includeBuffer.PutString(", ");
	}
	includeBuffer.PutString(");\n");

	//create sourcepawn native header
	bool returnsArray = IsArrayType(descriptor.m_ReturnType);
	includeBuffer.Printf("\t%s %s %s(", global ? "public static native" : "public native", SourcePawnTypeName(returnsArray ? FIELD_VOID : descriptor.m_ReturnType), descriptor.m_pszScriptName);
	FOR_EACH_VEC(descriptor.m_Parameters, it) {
		includeBuffer.Printf("%s %c", SourcePawnTypeName(descriptor.m_Parameters[it]), 'a' + it);
		if (returnsArray || it < descriptor.m_Parameters.Count() - 1)
			includeBuffer.PutString(", ");
	}
	if (returnsArray) {
		includeBuffer.Printf("%s %s", SourcePawnTypeName(descriptor.m_ReturnType), "retVal");
		if (descriptor.m_ReturnType == FIELD_CSTRING)
			includeBuffer.PutString(", int maxLength");
	}
	includeBuffer.PutString(");\n\n");
}

bool IsEntityClass(ScriptClassDesc_t * pClassDesc) {
	if (!pClassDesc)
		return false;
	if (V_strcmp(pClassDesc->m_pszScriptName, "CBaseEntity") == 0)
		return true;
	return IsEntityClass(pClassDesc->m_pBaseDesc);
}

void CreateClassHeader(CUtlBuffer& includeBuffer, const CFunctions::CClass& cls) {
	//show class description
	if (cls.m_descriptor)
		includeBuffer.Printf("//%s\n", cls.m_descriptor->m_pszDescription);

	const char * origClassname = cls.m_descriptor ? cls.m_descriptor->m_pszScriptName : "";
	bool global = !origClassname[0] || cls.m_instance;
	//prepend prefix
	char classname[256];
	V_strcpy(classname, "VSF_");
	if (origClassname[0])
		V_strcpy(classname + 4, origClassname);
	else
		classname[3] = 0;

	char inheritance[256];
	if (cls.m_descriptor && cls.m_descriptor->m_pBaseDesc) {
		V_strcpy(inheritance, " < VSF_");
		V_strcpy(inheritance + 7, cls.m_descriptor->m_pBaseDesc->m_pszScriptName);
	}
	else
		inheritance[0] = 0;

	const CUtlStringMap<const ScriptFunctionBinding_t*>& functions = cls.m_functions;
	includeBuffer.Printf("methodmap %s%s {\n", classname, inheritance);
	if (!global)
		includeBuffer.Printf("\tpublic %s(HScript handle) { return view_as<%s>(handle); }\n\n", classname, classname);

	if (IsEntityClass(cls.m_descriptor))
		includeBuffer.Printf("\tpublic static %s FromEntIndex(int entindex) { return view_as<%s>(GetEntityAddress(entindex)); }\n\n", classname, classname);

	for (UtlSymId_t fit = 0; fit < functions.GetNumStrings(); fit++) {
		CreateFunctionHeader(includeBuffer, functions[fit], global);
	}

	includeBuffer.PutString("}\n\n");
}

void CFunctions::CreateInclude(CUtlBuffer& includeBuffer) {
	includeBuffer.PutString("enum HScript {\n\tHSCRIPT_NULL = 0,\n\tHSCRIPT_INVALID = -1\n}\n\n");

	//create the classes definitions in correct order (parents first, children later)
	CUtlHash<ScriptClassDesc_t *> created(
		m_classes.GetNumStrings(), 0, 0,
		[](auto a, auto b) { return a == b; },
		[](auto a) {return (unsigned int)a; });

	while (created.Count() < m_classes.GetNumStrings()) {
		for(UtlSymId_t it = 0; it < m_classes.GetNumStrings(); it++){
			const CClass& cls = m_classes[it];
			if (created.Find(cls.m_descriptor) != created.InvalidHandle())
				continue;
			if (cls.m_descriptor && cls.m_descriptor->m_pBaseDesc && created.Find(cls.m_descriptor->m_pBaseDesc) == created.InvalidHandle())
				continue;
			CreateClassHeader(includeBuffer, cls);
			created.Insert(cls.m_descriptor);
		}
	}
}

inline void PawnToScript(const cell_t& inParam, ScriptVariant_t& outArg, int16 eType, IPluginContext *pContext) {
	outArg.m_type = eType;
	switch (eType)
	{
	case FIELD_FLOAT:
		outArg.m_float = sp_ctof(inParam);
		break;
	case FIELD_CSTRING:
		pContext->LocalToString(inParam, (char**)&outArg.m_pszString);
		break;
	case FIELD_VECTOR:
		cell_t * cVector;
		pContext->LocalToPhysAddr(inParam, &cVector);
		outArg.m_pVector = new Vector(sp_ctof(cVector[0]), sp_ctof(cVector[1]), sp_ctof(cVector[2]));
		break;
	default:
		outArg.m_int = inParam;
		break;
	}
}

inline void ScriptToPawn(const ScriptVariant_t& inArg, cell_t& outParam, IPluginContext *pContext, int maxLength) {
	switch (inArg.m_type)
	{
	case FIELD_FLOAT:
		outParam = sp_ftoc(inArg.m_float);
		break;
	case FIELD_CSTRING:
		pContext->StringToLocalUTF8(outParam, maxLength, inArg.m_pszString, NULL);
		break;
	case FIELD_VECTOR:
		cell_t * cVector;
		pContext->LocalToPhysAddr(outParam, &cVector);
		cVector[0] = sp_ftoc(inArg.m_pVector->x);
		cVector[1] = sp_ftoc(inArg.m_pVector->y);
		cVector[2] = sp_ftoc(inArg.m_pVector->z);
		break;
	default:
		outParam = inArg.m_int;
		break;
	}
}

cell_t CFunctions::PawnCallFunction(const char * classname, const char * funcname, IPluginContext *pContext, const cell_t *params) {
	CClass& cls = m_classes[classname];
	const ScriptFunctionBinding_t * func = cls.m_functions[funcname];
	if (!func)
		return pContext->ThrowNativeError("Couldn't find function %s::%s!", classname, funcname);
	const ScriptFuncDescriptor_t& descriptor = func->m_desc;
	bool global = !classname[0] || cls.m_instance;
	bool returnsArray = IsArrayType(descriptor.m_ReturnType);
	bool returnsString = descriptor.m_ReturnType == FIELD_CSTRING;
	int numParams = params[0];
	int expectedParams = descriptor.m_Parameters.Count() + (global ? 0 : 1) + (returnsArray ? 1 : 0) + (returnsString ? 1 : 0);
	if (numParams != expectedParams)
		return pContext->ThrowNativeError("Expected %d params, got %d!", expectedParams, numParams);

	void * context = NULL;
	if (!global) {
		context = (void*)(params[1]);
		FOR_EACH_VEC(m_svm, it) {
			void * resolvedContext = m_svm[it]->GetInstanceValue((HSCRIPT)context);
			if (resolvedContext) {
				context = resolvedContext;
				break;
			}
		}
	}
	if (cls.m_instance)
		context = cls.m_instance;

	ScriptVariant_t * args = new ScriptVariant_t[descriptor.m_Parameters.Count()]();
	for (int i = 0; i < descriptor.m_Parameters.Count(); i++)
		PawnToScript(params[i + 1 + (global ? 0 : 1)], args[i], descriptor.m_Parameters[i], pContext);

	ScriptVariant_t retVal = CallFunction(func, context, args);
	cell_t retCell = NULL;
	ScriptToPawn(retVal, returnsString ? params[numParams - 1] : returnsArray ? params[numParams] : retCell, pContext, params[numParams]);

	for (int i = 0; i < descriptor.m_Parameters.Count(); i++) {
		if (args[i].m_type == FIELD_VECTOR) {
			delete args[i].m_pVector;
			args[i].m_pVector = NULL;
		}
	}
	delete[] args;
	return retCell;
}

void CFunctions::Clear()
{
	m_classes.Clear();
	m_svm.RemoveAll();
}
