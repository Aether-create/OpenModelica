/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-CurrentYear, Linköping University,
 * Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3 
 * AND THIS OSMC PUBLIC LICENSE (OSMC-PL). 
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES RECIPIENT'S  
 * ACCEPTANCE OF THE OSMC PUBLIC LICENSE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from Linköping University, either from the above address,
 * from the URLs: http://www.ida.liu.se/projects/OpenModelica or  
 * http://www.openmodelica.org, and in the OpenModelica distribution. 
 * GNU version 3 is obtained from: http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS
 * OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 */

encapsulated package SimCodeMain
" file:        SimCodeMain.mo
  package:     SimCodeMain
  description: Code generation using Susan templates

  The entry points to this module are the translateModel function and the
  translateFunctions fuction.
  
  RCS: $Id$"

// public imports
public import Absyn;
public import BackendDAE;
public import BackendDAEUtil;
public import Ceval;
public import DAE;
public import Env;
public import HashTableExpToIndex;
public import HashTableStringToPath;
public import Interactive;
public import Tpl;
public import Values;
public import SimCode;

// protected imports
protected import BackendDAECreate;
protected import BackendQSS;
protected import BaseHashTable;
protected import CevalScript;
protected import CodegenC;
protected import CodegenFMU;
protected import CodegenFMUCpp;
protected import CodegenQSS;
protected import CodegenAdevs;
protected import CodegenCSharp;
protected import CodegenCpp;
protected import CodegenXML;
protected import CodegenJava;
protected import Config;
protected import DAEUtil;
protected import Debug;
protected import Error;
protected import Flags;
protected import SimCodeDump;
protected import SimCodeUtil;
protected import System;
protected import Util;


public function createSimulationSettings
  input Real startTime;
  input Real stopTime;
  input Integer inumberOfIntervals;
  input Real tolerance;
  input String method;
  input String options;
  input String outputFormat;
  input String variableFilter;
  input Boolean measureTime;
  input String cflags;
  output SimCode.SimulationSettings simSettings;
protected
  Real stepSize;
  Integer numberOfIntervals;
algorithm
  numberOfIntervals := Util.if_(inumberOfIntervals <= 0, 1, inumberOfIntervals);
  stepSize := (stopTime -. startTime) /. intReal(numberOfIntervals);
  simSettings := SimCode.SIMULATION_SETTINGS(
    startTime, stopTime, numberOfIntervals, stepSize, tolerance,
    method, options, outputFormat, variableFilter, measureTime, cflags);
end createSimulationSettings;


public function generateModelCodeFMU
  "Generates code for a model by creating a SimCode structure and calling the
   template-based code generator on it."
  input BackendDAE.BackendDAE inBackendDAE;
  input Absyn.Program p;
  input DAE.DAElist dae;
  input Absyn.Path className;
  input String filenamePrefix;
  input Option<SimCode.SimulationSettings> simSettingsOpt;
  output BackendDAE.BackendDAE outIndexedBackendDAE;
  output list<String> libs;
  output String fileDir;
  output Real timeBackend;
  output Real timeSimCode;
  output Real timeTemplates;
protected 
  list<String> includes,includeDirs;
  list<SimCode.Function> functions;
  // DAE.DAElist dae2;
  String filename, funcfilename;
  SimCode.SimCode simCode;
  list<SimCode.RecordDeclaration> recordDecls;
  BackendDAE.BackendDAE indexed_dlow,indexed_dlow_1;
  Absyn.ComponentRef a_cref;
  tuple<Integer,HashTableExpToIndex.HashTable,list<DAE.Exp>> literals;
algorithm
  timeBackend := System.realtimeTock(CevalScript.RT_CLOCK_BACKEND);
  a_cref := Absyn.pathToCref(className);
  fileDir := CevalScript.getFileDir(a_cref, p);
  System.realtimeTick(CevalScript.RT_CLOCK_SIMCODE);
  (libs, includes, includeDirs, recordDecls, functions, outIndexedBackendDAE, _, literals) :=
  SimCodeUtil.createFunctions(dae, inBackendDAE, className);
  simCode := SimCodeUtil.createSimCode(outIndexedBackendDAE,
    className, filenamePrefix, fileDir, functions, includes, includeDirs, libs, simSettingsOpt, recordDecls, literals,Absyn.FUNCTIONARGS({},{}));
  timeSimCode := System.realtimeTock(CevalScript.RT_CLOCK_SIMCODE);
  Debug.execStat("SimCode",CevalScript.RT_CLOCK_SIMCODE);
  
  System.realtimeTick(CevalScript.RT_CLOCK_TEMPLATES);
  callTargetTemplatesFMU(simCode, Config.simCodeTarget());
  timeTemplates := System.realtimeTock(CevalScript.RT_CLOCK_TEMPLATES);
end generateModelCodeFMU;


public function translateModelFMU
"Entry point to translate a Modelica model for FMU export.
    
 Called from other places in the compiler."
  input Env.Cache inCache;
  input Env.Env inEnv;
  input Absyn.Path className "path for the model";
  input Interactive.SymbolTable inInteractiveSymbolTable;
  input String inFileNamePrefix;
  input Boolean addDummy "if true, add a dummy state";
  input Option<SimCode.SimulationSettings> inSimSettingsOpt;
  output Env.Cache outCache;
  output Values.Value outValue;
  output Interactive.SymbolTable outInteractiveSymbolTable;
  output BackendDAE.BackendDAE outBackendDAE;
  output list<String> outStringLst;
  output String outFileDir;
  output list<tuple<String,Values.Value>> resultValues;
algorithm
  (outCache,outValue,outInteractiveSymbolTable,outBackendDAE,outStringLst,outFileDir,resultValues):=
  matchcontinue (inCache,inEnv,className,inInteractiveSymbolTable,inFileNamePrefix,addDummy, inSimSettingsOpt)
    local
      String filenameprefix,file_dir,resstr;
      DAE.DAElist dae;
      list<Env.Frame> env;
      BackendDAE.BackendDAE dlow,dlow_1,indexed_dlow_1;
      list<String> libs;
      Interactive.SymbolTable st;
      Absyn.Program p;
      //DAE.Exp fileprefix;
      Env.Cache cache;
      DAE.FunctionTree funcs;
      Real timeSimCode, timeTemplates, timeBackend, timeFrontend;
    case (cache,env,_,st as Interactive.SYMBOLTABLE(ast=p),filenameprefix,_, _)
      equation
        /* calculate stuff that we need to create SimCode data structure */
        System.realtimeTick(CevalScript.RT_CLOCK_FRONTEND);
        //(cache,Values.STRING(filenameprefix),SOME(_)) = Ceval.ceval(cache,env, fileprefix, true, SOME(st),NONE(), msg);
        (cache,env,dae,st) = CevalScript.runFrontEnd(cache,env,className,st,false);
        timeFrontend = System.realtimeTock(CevalScript.RT_CLOCK_FRONTEND);
        System.realtimeTick(CevalScript.RT_CLOCK_BACKEND);
        funcs = Env.getFunctionTree(cache);
        dae = DAEUtil.transformationsBeforeBackend(cache,env,dae);
        dlow = BackendDAECreate.lower(dae,cache,env);
        dlow_1 = BackendDAEUtil.getSolvedSystem(dlow,NONE(), NONE(), NONE(), NONE());
        Debug.fprintln(Flags.DYN_LOAD, "translateModel: Generating simulation code and functions.");
        (indexed_dlow_1,libs,file_dir,timeBackend,timeSimCode,timeTemplates) = 
          generateModelCodeFMU(dlow_1, p, dae,  className, filenameprefix, inSimSettingsOpt);
        resultValues = 
        {("timeTemplates",Values.REAL(timeTemplates)),
          ("timeSimCode",  Values.REAL(timeSimCode)),
          ("timeBackend",  Values.REAL(timeBackend)),
          ("timeFrontend", Values.REAL(timeFrontend))
          };
          resstr = Absyn.pathStringNoQual(className);
        resstr = stringAppendList({"SimCode: The model ",resstr," has been translated to FMU"});
      then
        (cache,Values.STRING(resstr),st,indexed_dlow_1,libs,file_dir, resultValues);
    case (_,_,_,_,_,_, _)
      equation        
        resstr = Absyn.pathStringNoQual(className);
        resstr = stringAppendList({"SimCode: The model ",resstr," could not be translated to FMU"});
        Error.addMessage(Error.INTERNAL_ERROR, {resstr});
      then
        fail();
  end matchcontinue;
end translateModelFMU;


public function generateModelCode
  "Generates code for a model by creating a SimCode structure and calling the
   template-based code generator on it."
   
  input BackendDAE.BackendDAE inBackendDAE;
  input Absyn.Program p;
  input DAE.DAElist dae;
  input Absyn.Path className;
  input String filenamePrefix;
  input Option<SimCode.SimulationSettings> simSettingsOpt;
  input Absyn.FunctionArgs args;
  output BackendDAE.BackendDAE outIndexedBackendDAE;
  output list<String> libs;
  output String fileDir;
  output Real timeBackend;
  output Real timeSimCode;
  output Real timeTemplates;
protected 
  
  list<String> includes,includeDirs;
  list<SimCode.Function> functions;
  // DAE.DAElist dae2;
  String filename, funcfilename;
  SimCode.SimCode simCode;
  list<SimCode.RecordDeclaration> recordDecls;
  BackendDAE.BackendDAE indexed_dlow,indexed_dlow_1;
  Absyn.ComponentRef a_cref;
  BackendQSS.QSSinfo qssInfo;
  tuple<Integer,HashTableExpToIndex.HashTable,list<DAE.Exp>> literals;
  list<SimCode.JacobianMatrix> LinearMatrices;
algorithm
  timeBackend := System.realtimeTock(CevalScript.RT_CLOCK_BACKEND);
  a_cref := Absyn.pathToCref(className);
  fileDir := CevalScript.getFileDir(a_cref, p);
  System.realtimeTick(CevalScript.RT_CLOCK_SIMCODE);
  (libs, includes, includeDirs, recordDecls, functions, outIndexedBackendDAE, _, literals) :=
  SimCodeUtil.createFunctions(dae, inBackendDAE, className);
  simCode := SimCodeUtil.createSimCode(outIndexedBackendDAE, 
    className, filenamePrefix, fileDir, functions, includes, includeDirs, libs, simSettingsOpt, recordDecls, literals, args);

  timeSimCode := System.realtimeTock(CevalScript.RT_CLOCK_SIMCODE);
  Debug.execStat("SimCode",CevalScript.RT_CLOCK_SIMCODE);
  
  System.realtimeTick(CevalScript.RT_CLOCK_TEMPLATES);
  callTargetTemplates(simCode,inBackendDAE,Config.simCodeTarget());
  timeTemplates := System.realtimeTock(CevalScript.RT_CLOCK_TEMPLATES);
end generateModelCode;

// TODO: use another switch ... later make it first class option like -target or so
// Update: inQSSrequiredData passed in order to call BackendQSS and generate the extra structures needed for QSS simulation.
protected function callTargetTemplates
"Generate target code by passing the SimCode data structure to templates."
  input SimCode.SimCode simCode;
  input BackendDAE.BackendDAE inQSSrequiredData;
  input String target;
algorithm
  _ := match (simCode,inQSSrequiredData,target)
    local
      BackendDAE.BackendDAE outIndexedBackendDAE;
      array<Integer> equationIndices, variableIndices;
      BackendDAE.IncidenceMatrix incidenceMatrix;
      BackendDAE.IncidenceMatrixT incidenceMatrixT;
      BackendDAE.StrongComponents strongComponents;
      BackendQSS.QSSinfo qssInfo;
      String str;
      SimCode.SimCode sc;
      
    case (_,_,"CSharp")
      equation
        Tpl.tplNoret(CodegenCSharp.translateModel, simCode);
      then ();
   case (_,_,"Cpp")
      equation
        Tpl.tplNoret(CodegenCpp.translateModel, simCode);
      then ();
   case (_,_,"Adevs")
      equation
        Tpl.tplNoret(CodegenAdevs.translateModel, simCode);
      then ();
    case (_,outIndexedBackendDAE as BackendDAE.DAE(eqs={
        BackendDAE.EQSYSTEM(m=SOME(incidenceMatrix), mT=SOME(incidenceMatrixT), matching=BackendDAE.MATCHING(equationIndices, variableIndices,strongComponents))
      }),"QSS")
      equation
        Debug.trace("Generating code for QSS solver\n");
        (qssInfo,sc) = BackendQSS.generateStructureCodeQSS(outIndexedBackendDAE, equationIndices, variableIndices, incidenceMatrix, incidenceMatrixT, strongComponents,simCode);
        Tpl.tplNoret2(CodegenQSS.translateModel, sc,qssInfo);
      then ();
    case (_,_,"C")
      equation
        Tpl.tplNoret(CodegenC.translateModel, simCode);
      then ();        
    case (_,_,"Dump")
      equation
        // Yes, do this better later on...
        print(Tpl.tplString(SimCodeDump.dumpSimCode, simCode));
      then ();
    case (_,_,"XML")
      equation
        Tpl.tplNoret(CodegenXML.translateModel, simCode);
      then ();
    case (_,_,"Java")
      equation
        Tpl.tplNoret(CodegenJava.translateModel, simCode);
      then ();
    case (_,_,_)
      equation
        str = "Unknown template target: " +& target;
        Error.addMessage(Error.INTERNAL_ERROR, {str});
      then fail();
  end match;
end callTargetTemplates;

protected function callTargetTemplatesFMU
"Generate target code by passing the SimCode data structure to templates."
  input SimCode.SimCode simCode;
  input String target;
algorithm
  _ := match (simCode,target)
    local
      String str;
      
    case (_,"C")
      equation
        Tpl.tplNoret(CodegenFMU.translateModel, simCode);
      then ();        
    case (_,"Cpp")
      equation
        Tpl.tplNoret(CodegenFMUCpp.translateModel, simCode);
      then (); 
    case (_,"Dump")
      equation
        // Yes, do this better later on...
        print(Tpl.tplString(SimCodeDump.dumpSimCode, simCode));
      then ();
    case (_,_)
      equation
        str = "Unknown template target: " +& target;
        Error.addMessage(Error.INTERNAL_ERROR, {str});
      then fail();
  end match;
end callTargetTemplatesFMU;

public function translateModel
"Entry point to translate a Modelica model for simulation.
    
 Called from other places in the compiler."
  input Env.Cache inCache;
  input Env.Env inEnv;
  input Absyn.Path className "path for the model";
  input Interactive.SymbolTable inInteractiveSymbolTable;
  input String inFileNamePrefix;
  input Boolean addDummy "if true, add a dummy state";
  input Option<SimCode.SimulationSettings> inSimSettingsOpt;
  input Absyn.FunctionArgs args "labels for remove terms";
  output Env.Cache outCache;
  output Values.Value outValue;
  output Interactive.SymbolTable outInteractiveSymbolTable;
  output BackendDAE.BackendDAE outBackendDAE;
  output list<String> outStringLst;
  output String outFileDir;
  output list<tuple<String,Values.Value>> resultValues;
algorithm
  (outCache,outValue,outInteractiveSymbolTable,outBackendDAE,outStringLst,outFileDir,resultValues):=
  matchcontinue (inCache,inEnv,className,inInteractiveSymbolTable,inFileNamePrefix,addDummy, inSimSettingsOpt, args)
    local
      String filenameprefix,file_dir,resstr;
      DAE.DAElist dae;
      list<Env.Frame> env;
      BackendDAE.BackendDAE dlow,dlow_1,indexed_dlow_1;
      list<String> libs;
      Interactive.SymbolTable st;
      Absyn.Program p;
      //DAE.Exp fileprefix;
      Env.Cache cache;
      Real timeSimCode, timeTemplates, timeBackend, timeFrontend;

    case (cache,env,_,(st as Interactive.SYMBOLTABLE(ast = p)),filenameprefix,_, _,_)
      equation
        // calculate stuff that we need to create SimCode data structure 
        System.realtimeTick(CevalScript.RT_CLOCK_FRONTEND);
        //(cache,Values.STRING(filenameprefix),SOME(_)) = Ceval.ceval(cache,env, fileprefix, true, SOME(st),NONE(), msg);
        (cache,env,dae,st) = CevalScript.runFrontEnd(cache,env,className,st,false);
        timeFrontend = System.realtimeTock(CevalScript.RT_CLOCK_FRONTEND);
        System.realtimeTick(CevalScript.RT_CLOCK_BACKEND);
        dae = DAEUtil.transformationsBeforeBackend(cache,env,dae);
        dlow = BackendDAECreate.lower(dae,cache,env);
        dlow_1 = BackendDAEUtil.getSolvedSystem(dlow,NONE(), NONE(), NONE(), NONE());
        (indexed_dlow_1,libs,file_dir,timeBackend,timeSimCode,timeTemplates) = 
          generateModelCode(dlow_1, p, dae,  className, filenameprefix, inSimSettingsOpt, args);
        resultValues = 
        {("timeTemplates",Values.REAL(timeTemplates)),
          ("timeSimCode",  Values.REAL(timeSimCode)),
          ("timeBackend",  Values.REAL(timeBackend)),
          ("timeFrontend", Values.REAL(timeFrontend))
          };          
        resstr = Util.if_(Flags.isSet(Flags.FAILTRACE),Absyn.pathStringNoQual(className),"");
        resstr = stringAppendList({"SimCode: The model ",resstr," has been translated"});
        //        resstr = "SimCode: The model has been translated";
      then
        (cache,Values.STRING(resstr),st,indexed_dlow_1,libs,file_dir, resultValues);
    case (_,_,_,_,_,_,_,_)
      equation        
        true = Flags.isSet(Flags.FAILTRACE);
        resstr = Absyn.pathStringNoQual(className);
        resstr = stringAppendList({"SimCode: The model ",resstr," could not be translated"});
        Error.addMessage(Error.INTERNAL_ERROR, {resstr});
      then
        fail();
  end matchcontinue;
end translateModel;

public function translateFunctions
"Entry point to translate Modelica/MetaModelica functions to C functions.
    
 Called from other places in the compiler."
  input String name;
  input Option<DAE.Function> optMainFunction;
  input list<DAE.Function> idaeElements;
  input list<DAE.Type> metarecordTypes;
  input list<String> inIncludes;
algorithm
  _ := match (name, optMainFunction, idaeElements, metarecordTypes, inIncludes)
    local
      DAE.Function daeMainFunction;
      SimCode.Function mainFunction;
      list<SimCode.Function> fns;
      list<String> includes, libs, includeDirs;
      SimCode.MakefileParams makefileParams;
      SimCode.FunctionCode fnCode;
      list<SimCode.RecordDeclaration> extraRecordDecls;
      list<DAE.Exp> literals;
      list<DAE.Function> daeElements;
      
    case (_, SOME(daeMainFunction), daeElements, _, includes)
      equation
        // Create SimCode.FunctionCode
        (daeElements,literals) = SimCodeUtil.findLiterals(daeMainFunction::daeElements);
        (mainFunction::fns, extraRecordDecls, includes, includeDirs, libs) = SimCodeUtil.elaborateFunctions(daeElements, metarecordTypes, literals, includes);
        SimCodeUtil.checkValidMainFunction(name, mainFunction);
        makefileParams = SimCodeUtil.createMakefileParams(includeDirs, libs);
        fnCode = SimCode.FUNCTIONCODE(name, SOME(mainFunction), fns, literals, includes, makefileParams, extraRecordDecls);
        // Generate code
        _ = Tpl.tplString(CodegenC.translateFunctions, fnCode);
      then
        ();
    case (_, NONE(), daeElements, _, includes)
      equation
        // Create SimCode.FunctionCode
        (daeElements,literals) = SimCodeUtil.findLiterals(daeElements);
        (fns, extraRecordDecls, includes, includeDirs, libs) = SimCodeUtil.elaborateFunctions(daeElements, metarecordTypes, literals, includes);
        makefileParams = SimCodeUtil.createMakefileParams(includeDirs, libs);
        fnCode = SimCode.FUNCTIONCODE(name, NONE(), fns, literals, includes, makefileParams, extraRecordDecls);
        // Generate code
        _ = Tpl.tplString(CodegenC.translateFunctions, fnCode);
      then
        ();
  end match;
end translateFunctions;

public function getCalledFunctionsInFunction
"function: getCalledFunctionsInFunction
  Goes through the given DAE, finds the given function and collects
  the names of the functions called from within those functions"
  input Absyn.Path path;
  input DAE.FunctionTree funcs;
  output list<Absyn.Path> outPaths;
protected
  HashTableStringToPath.HashTable ht;
algorithm
  ht := HashTableStringToPath.emptyHashTable();
  ht := SimCodeUtil.getCalledFunctionsInFunction2(path,Absyn.pathStringNoQual(path),ht,funcs);
  outPaths := BaseHashTable.hashTableValueList(ht);
end getCalledFunctionsInFunction;

end SimCodeMain;
