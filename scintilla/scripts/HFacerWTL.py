#!/usr/bin/env python
# HFacer.py - regenerate the Scintilla.h and SciLexer.h files from the Scintilla.iface interface
# definition file.
# Implemented 2000 by Neil Hodgson neilh@scintilla.org
# Requires Python 2.5 or later

import sys
import os
import Face

from FileGenerator import UpdateFile, Generate, Regenerate, UpdateLineInFile, lineEnd

def printWTLFile(f):
	typeMap = {}
	typeMap[""] = ""
	typeMap["void"] = "void"
	typeMap["int"] = "int"
	typeMap["bool"] = "bool"
	typeMap["position"] = "int"
	typeMap["colour"] = "COLORREF"
	typeMap["string"] = "LPCSTR"
	typeMap["stringresult"] = "LPSTR"
	typeMap["cells"] = "char *"
	typeMap["textrange"] = "Sci_TextRange *"
	typeMap["findtext"] = "Sci_TextToFind *"
	typeMap["keymod"] = "DWORD"
	typeMap["formatrange"] = "Sci_RangeToFormat *"

	out = []
	previousCategory = ""
	anyProvisional = False
	for name in f.order:
		v = f.features[name]
		if v["Category"] != "Deprecated":
			if v["Category"] == "Provisional" and previousCategory != "Provisional":
				out.append("#ifndef SCI_DISABLE_PROVISIONAL")
				anyProvisional = True
			previousCategory = v["Category"]
			if v["FeatureType"] in ["fun", "get", "set"]:
				featureDefineName = "SCI_" + name.upper()
				methodMod = "throw()"
				if v["FeatureType"] == "get":
					methodMod = "const " + methodMod
				returnType = typeMap[v["ReturnType"]]
				param1Name = v["Param1Name"]
				param1Type = typeMap[v["Param1Type"]]
				param2Name = v["Param2Name"]
				param2Type = typeMap[v["Param2Type"]]
				out.append("\t" + returnType + " " + name + "(" + (param1Type + " " + param1Name if param1Name else "") + (", " if param1Name and param2Name else "") + (param2Type + " " + param2Name if param2Name else "") + ") " + methodMod)
				out.append("\t{")
				out.append("\t\tATLASSERT(::IsWindow(m_hWnd));")
				out.append("\t\t" + ("return " if returnType != "void" else "") + "Call(" + featureDefineName + ", " + ("(WPARAM) " + param1Name if param1Name else "0") + ", " + ("(LPARAM) " + param2Name if param2Name else "0") + ");")
				out.append("\t}")
				out.append("")
				if v["Param2Type"] == "stringresult":
					out.append("\tint " + name + "(" + (param1Type + " " + param1Name + ", " if param1Name else "") + "CSimpleStringA& strText) const")
					out.append("\t{")
					out.append("\t\tint nLength;")
					out.append("\t\tLPSTR pszText;")
					out.append("")
					out.append("\t\tnLength = " + name + "Length(" + (param1Name if param1Name else "") + ");")
					out.append("\t\tpszText = strText.GetBuffer(nLength+1);")
					out.append("\t\tnLength = " + name + "(" + (param1Name + ", " if param1Name else "") + "pszText);")
					out.append("\t\tstrText.ReleaseBuffer();")
					out.append("")
					out.append("return nLength;")
					out.append("\t}")
					out.append("")
					out.append("\tint " + name + "Length(" + (param1Type + " " + param1Name if param1Name else "") + ") const throw()")
					out.append("\t{")
					out.append("\t\tATLASSERT(::IsWindow(m_hWnd));")
					out.append("\t\treturn Call(" + featureDefineName + (", (WPARAM) " + param1Name if param1Name else "") + ", 0)")
					out.append("\t}")
					out.append("")
				#
				#out.append("#define " + featureDefineName + " " + v["Value"])
			#elif v["FeatureType"] in ["evt"]:
			#	featureDefineName = "SCN_" + name.upper()
			#	out.append("#define " + featureDefineName + " " + v["Value"])
			#elif v["FeatureType"] in ["val"]:
			#	if not ("SCE_" in name or "SCLEX_" in name):
			#		out.append("#define " + name + " " + v["Value"])
	if anyProvisional:
		out.append("#endif")
	return out

def RegenerateAll(root, showMaxID):
	f = Face.Face()
	f.ReadFromFile(root + "include/Scintilla.iface")
	Regenerate(root + "include/atlscilexer.h", "/* ", printWTLFile(f))
	if showMaxID:
		valueSet = set(int(x) for x in f.values if int(x) < 3000)
		maximumID = max(valueSet)
		print("Maximum ID is %d" % maximumID)
		#~ valuesUnused = sorted(x for x in range(2001,maximumID) if x not in valueSet)
		#~ print("\nUnused values")
		#~ for v in valuesUnused:
			#~ print(v)

if __name__ == "__main__":
	RegenerateAll("../", True)
