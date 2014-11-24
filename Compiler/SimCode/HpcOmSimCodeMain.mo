/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-2014, Open Source Modelica Consortium (OSMC),
 * c/o Linköpings universitet, Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3 LICENSE OR
 * THIS OSMC PUBLIC LICENSE (OSMC-PL) VERSION 1.2.
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES
 * RECIPIENT'S ACCEPTANCE OF THE OSMC PUBLIC LICENSE OR THE GPL VERSION 3,
 * ACCORDING TO RECIPIENTS CHOICE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from OSMC, either from the above address,
 * from the URLs: http://www.ida.liu.se/projects/OpenModelica or
 * http://www.openmodelica.org, and in the OpenModelica distribution.
 * GNU version 3 is obtained from: http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 */

encapsulated package HpcOmSimCodeMain
" file:        HpcOmSimCodeMain.mo
  package:     HpcOmSimCodeMain
  description: HpcOmSimCodeMain contains the logic to create a parallelized simcode.

  RCS: $Id: HpcOmSimCodeMain.mo 15486 2013-05-24 11:12:35Z marcusw $
"
// public imports
public import Absyn;
public import BackendDAE;
public import DAE;
public import HashTableExpToIndex;
public import HpcOmSimCode;
public import HpcOmTaskGraph;
public import HpcOmEqSystems;
public import SimCode;

// protected imports
protected import Array;
protected import BackendDAEUtil;
protected import ClockIndexes;
protected import Debug;
protected import Error;
protected import Flags;
protected import HpcOmMemory;
protected import HpcOmScheduler;
protected import Initialization;
protected import List;
protected import SimCodeUtil;
protected import System;
protected import Util;

public function createSimCode "function createSimCode
  entry point to create SimCode from BackendDAE."
  input BackendDAE.BackendDAE inBackendDAE;
  input Absyn.Path inClassName;
  input String filenamePrefix;
  input String inString11;
  input list<SimCode.Function> functions;
  input list<String> externalFunctionIncludes;
  input list<String> includeDirs;
  input list<String> libs;
  input list<String> libPaths;
  input Option<SimCode.SimulationSettings> simSettingsOpt;
  input list<SimCode.RecordDeclaration> recordDecls;
  input tuple<Integer, HashTableExpToIndex.HashTable, list<DAE.Exp>> literals;
  input Absyn.FunctionArgs args;
  output SimCode.SimCode simCode;
algorithm
  simCode := matchcontinue (inBackendDAE, inClassName, filenamePrefix, inString11, functions, externalFunctionIncludes, includeDirs, libs, libPaths,simSettingsOpt, recordDecls, literals, args)
    local
      String fileDir, cname;
      Integer lastEqMappingIdx, maxDelayedExpIndex, uniqueEqIndex, numberofEqns, numberOfInitialEquations, numberOfInitialAlgorithms, numStateSets;
      Integer numberofLinearSys, numberofNonLinearSys, numberofMixedSys;
      BackendDAE.BackendDAE dlow, dlow2;
      Option<BackendDAE.BackendDAE> initDAE;
      BackendDAE.IncidenceMatrix incidenceMatrix;
      DAE.FunctionTree functionTree;
      BackendDAE.SymbolicJacobians symJacs;
      Absyn.Path class_;

      // new variables
      SimCode.ModelInfo modelInfo;
      list<SimCode.SimEqSystem> allEquations;
      list<list<SimCode.SimEqSystem>> odeEquations;         // --> functionODE
      list<list<SimCode.SimEqSystem>> algebraicEquations;   // --> functionAlgebraics
      list<SimCode.SimEqSystem> residuals;                  // --> initial_residual
      Boolean useSymbolicInitialization;                    // true if a system to solve the initial problem symbolically is generated, otherwise false
      Boolean useHomotopy;                                  // true if homotopy(...) is used during initialization
      list<SimCode.SimEqSystem> initialEquations;           // --> initial_equations
      list<SimCode.SimEqSystem> removedInitialEquations;    // --> functionRemovedInitialEquations
      list<SimCode.SimEqSystem> startValueEquations;        // --> updateBoundStartValues
      list<SimCode.SimEqSystem> nominalValueEquations;      // --> updateBoundNominalValues
      list<SimCode.SimEqSystem> minValueEquations;          // --> updateBoundMinValues
      list<SimCode.SimEqSystem> maxValueEquations;          // --> updateBoundMaxValues
      list<SimCode.SimEqSystem> parameterEquations;         // --> updateBoundParameters
      list<SimCode.SimEqSystem> removedEquations;
      list<SimCode.SimEqSystem> algorithmAndEquationAsserts;
      list<SimCode.SimEqSystem> jacobianEquations;
      list<SimCode.SimEqSystem> zeroCrossingsEquations;
      //list<DAE.Statement> algorithmAndEquationAsserts;
      list<DAE.Constraint> constraints;
      list<DAE.ClassAttributes> classAttributes;
      list<BackendDAE.ZeroCrossing> zeroCrossings, sampleZC, relations;
      list<SimCode.SimWhenClause> whenClauses;
      list<DAE.ComponentRef> discreteModelVars;
      SimCode.ExtObjInfo extObjInfo;
      SimCode.MakefileParams makefileParams;
      SimCode.DelayedExpression delayedExps;
      BackendDAE.Variables knownVars;
      list<BackendDAE.Var> varlst;

      list<SimCode.JacobianMatrix> LinearMatrices, SymbolicJacs, SymbolicJacsTemp, SymbolicJacsStateSelect;
      SimCode.HashTableCrefToSimVar crefToSimVarHT;
      Boolean ifcpp;
      BackendDAE.EqSystems eqs;
      BackendDAE.Shared shared;
      BackendDAE.EquationArray removedEqs;
      list<DAE.Constraint> constrsarr;
      list<DAE.ClassAttributes> clsattrsarra;

      list<DAE.Exp> lits;
      list<SimCodeVar.SimVar> tempvars;

      SimCode.JacobianMatrix jacG;
      Integer highestSccIdx, compCountPlusDummy;
      Option<BackendDAE.BackendDAE> inlineDAE;
      list<SimCode.StateSet> stateSets;
      array<Integer> systemIndexMap;
      list<tuple<Integer,Integer>> equationSccMapping, equationSccMapping1; //Maps each simEq to the scc
      array<list<Integer>> sccSimEqMapping, daeSccMapping; //Maps each scc to a list of simEqs
      array<Integer> simeqCompMapping; //Maps each simEq to the scc
      list<BackendDAE.TimeEvent> timeEvents;
      BackendDAE.StrongComponents allComps, initComps;

      HpcOmTaskGraph.TaskGraph taskGraph, taskGraphDAE, taskGraphOde, taskGraphEvent, taskGraphSimplified, taskGraphScheduled, taskGraphInit;
      HpcOmTaskGraph.TaskGraphMeta taskGraphData, taskGraphDataDAE, taskGraphDataOde, taskGraphDataEvent, taskGraphDataSimplified,taskGraphDataScheduled, taskGraphDataInit;
      String fileName, fileNamePrefix;
      Integer numProc;
      list<list<Integer>> parallelSets;
      list<list<Integer>> criticalPaths, criticalPathsWoC;
      Real cpCosts, cpCostsWoC, serTime, parTime, speedUp, speedUpMax;
      list<HpcOmSimCode.Task> scheduledTasks;
      list<Integer> scheduledDAENodes;

      //Additional informations to append SimCode
      list<DAE.Exp> simCodeLiterals;
      list<SimCode.JacobianMatrix> jacobianMatrixes;
      list<SimCode.SimEqSystem> residualEquations;
      Option<SimCode.SimulationSettings> simulationSettingsOpt;
      list<SimCode.RecordDeclaration> simCodeRecordDecls;
      list<String> simCodeExternalFunctionIncludes;

      Boolean taskGraphMetaValid, numFixed;
      String taskGraphMetaMessage, criticalPathInfo;
      array<tuple<Integer,Integer,Real>> schedulerInfo; //maps each Task to <threadId, orderId, startCalcTime>
      HpcOmSimCode.Schedule schedule;
      array<tuple<Integer,Integer,Integer>> eqCompMapping, varCompMapping;
      Real graphCosts;
      Integer graphOps;
      Option<SimCode.BackendMapping> backendMapping;
      Option<HpcOmSimCode.MemoryMap> optTmpMemoryMap;
      list<HpcOmSimCode.Task> eventSystemTaskList;
      list<tuple<HpcOmSimCode.Task, list<Integer>>> eventSystemTasks;
      list<SimCode.SimEqSystem> equationsForConditions;
      array<Option<SimCode.SimEqSystem>> simEqIdxSimEqMapping;

      array<list<SimCodeVar.SimVar>> simVarMapping; //maps each backend variable to a list of simVars
    case (BackendDAE.DAE(eqs=eqs), _, _, _, _,_, _, _, _, _, _, _, _) equation

      if Flags.isSet(Flags.HPCOM_ANALYZATION_MODE) then
        print("Using analyzation mode\n");
      end if;

      //Initial System
      //--------------
      (initDAE, _, _) = Initialization.solveInitialSystem(inBackendDAE);
      removedInitialEquations = {};
      createAndExportInitialSystemTaskGraph(initDAE, filenamePrefix);

      //Setup
      //-----
      System.realtimeTick(ClockIndexes.RT_CLOCK_EXECSTAT_HPCOM_MODULES);
      (simCode,(lastEqMappingIdx,equationSccMapping)) = SimCodeUtil.createSimCode(inBackendDAE, inClassName, filenamePrefix, inString11, functions, externalFunctionIncludes, includeDirs, libs,libPaths, simSettingsOpt, recordDecls, literals, args);

      SimCode.SIMCODE(modelInfo, simCodeLiterals, simCodeRecordDecls, simCodeExternalFunctionIncludes, allEquations, odeEquations, algebraicEquations, residualEquations, useSymbolicInitialization, useHomotopy, initialEquations, removedInitialEquations, startValueEquations, nominalValueEquations, minValueEquations, maxValueEquations,
                 parameterEquations, removedEquations, algorithmAndEquationAsserts, zeroCrossingsEquations, jacobianEquations, stateSets, constraints, classAttributes, zeroCrossings, relations, timeEvents, whenClauses,
                 discreteModelVars, extObjInfo, makefileParams, delayedExps, jacobianMatrixes, simulationSettingsOpt, fileNamePrefix, _, _, _, crefToSimVarHT, backendMapping) = simCode;

      SOME(SimCode.BACKENDMAPPING(simVarMapping=simVarMapping)) = backendMapping;

      //get SCC to simEqSys-mappping
      //----------------------------
      (allComps,_) = HpcOmTaskGraph.getSystemComponents(inBackendDAE);
      //print("All components size: " + intString(listLength(allComps)) + "\n");
      highestSccIdx = findHighestSccIdxInMapping(equationSccMapping,-1);
      compCountPlusDummy = listLength(allComps)+1;
      equationSccMapping1 = removeDummyStateFromMapping(equationSccMapping);
      //the mapping can contain a dummy state as first scc
      equationSccMapping = if intEq(highestSccIdx, compCountPlusDummy) then equationSccMapping1 else equationSccMapping;
      sccSimEqMapping = convertToSccSimEqMapping(equationSccMapping, listLength(allComps));
      simeqCompMapping = convertToSimeqCompMapping(equationSccMapping, lastEqMappingIdx);
      _ = getSimEqIdxSimEqMapping(allEquations, arrayLength(simeqCompMapping));

      //dumpSimEqSCCMapping(simeqCompMapping);
      //dumpSccSimEqMapping(sccSimEqMapping);
      SimCodeUtil.execStat("hpcom setup");

      //Get complete DAE System
      //-----------------------
      (taskGraph,taskGraphData) = HpcOmTaskGraph.createTaskGraph(inBackendDAE);

      fileName = ("taskGraph"+filenamePrefix+".graphml");
      schedulerInfo = arrayCreate(arrayLength(taskGraph), (-1,-1,-1.0));
      HpcOmTaskGraph.dumpAsGraphMLSccLevel(taskGraph, taskGraphData,inBackendDAE, fileName, "", {}, {}, sccSimEqMapping, schedulerInfo, HpcOmTaskGraph.GRAPHDUMPOPTIONS(false,false,true,true));

      //taskGraphDAE = arrayCopy(taskGraph);
      //taskGraphDataDAE = HpcOmTaskGraph.copyTaskGraphMeta(taskGraphData);
      //(taskGraphDAE,taskGraphDataDAE) = HpcOmTaskGraph.appendRemovedEquations(inBackendDAE,taskGraphDAE,taskGraphDataDAE);

      //fileName = ("taskGraph"+filenamePrefix+"DAE.graphml");
      //daeSccMapping = listArray(List.map(SimCodeUtil.getRemovedEquationSimEqSysIdxes(simCode),List.create));
      //daeSccMapping = arrayAppend(sccSimEqMapping,daeSccMapping);
      //schedulerInfo = arrayCreate(arrayLength(taskGraphDAE), (-1,-1,-1.0));
      //HpcOmTaskGraph.dumpAsGraphMLSccLevel(taskGraphDAE, taskGraphDataDAE,inBackendDAE, fileName, "", {}, {}, daeSccMapping, schedulerInfo, HpcOmTaskGraph.GRAPHDUMPOPTIONS(false,false,true,true));
      SimCodeUtil.execStat("hpcom create and dump DAE TaskGraph");

      //print("DAE\n");
      //HpcOmTaskGraph.printTaskGraph(taskGraphDAE);
      //HpcOmTaskGraph.printTaskGraphMeta(taskGraphDataDAE);


      //Get Event System
      //----------------
      taskGraphEvent = arrayCopy(taskGraph);
      taskGraphDataEvent = HpcOmTaskGraph.copyTaskGraphMeta(taskGraphData);
      (taskGraphEvent,taskGraphDataEvent) = HpcOmTaskGraph.getEventSystem(taskGraphEvent,taskGraphDataEvent,inBackendDAE, zeroCrossings, simeqCompMapping);

      fileName = ("taskGraph"+filenamePrefix+"_event.graphml");
      schedulerInfo = arrayCreate(arrayLength(taskGraphEvent), (-1,-1,-1.0));
      HpcOmTaskGraph.dumpAsGraphMLSccLevel(taskGraphEvent, taskGraphDataEvent,inBackendDAE, fileName, "", {}, {}, sccSimEqMapping, schedulerInfo, HpcOmTaskGraph.GRAPHDUMPOPTIONS(false,false,true,true));
      SimCodeUtil.execStat("hpcom create and dump event TaskGraph");

      HpcOmSimCode.TASKDEPSCHEDULE(tasks=eventSystemTasks) = HpcOmScheduler.createTaskDepSchedule(taskGraphEvent, taskGraphDataEvent, sccSimEqMapping);
      _ = List.map(eventSystemTasks, Util.tuple21);

      //Create Costs
      //------------
      taskGraphData = HpcOmTaskGraph.createCosts(inBackendDAE, filenamePrefix + "_eqs_prof" , simeqCompMapping, taskGraphData);
      SimCodeUtil.execStat("hpcom create costs");

      //Get ODE System
      //--------------
      taskGraphOde = arrayCopy(taskGraph);
      taskGraphDataOde = HpcOmTaskGraph.copyTaskGraphMeta(taskGraphData);
      (taskGraphOde,taskGraphDataOde) = HpcOmTaskGraph.getOdeSystem(taskGraphOde,taskGraphDataOde,inBackendDAE);
      SimCodeUtil.execStat("hpcom create ODE TaskGraph");

      taskGraphMetaValid = HpcOmTaskGraph.validateTaskGraphMeta(taskGraphDataOde, inBackendDAE);
      taskGraphMetaMessage = if taskGraphMetaValid then "TaskgraphMeta valid\n" else "TaskgraphMeta invalid\n";
      print(taskGraphMetaMessage);

      //print("ODE\n");
      //HpcOmTaskGraph.printTaskGraph(taskGraphOde);
      //HpcOmTaskGraph.printTaskGraphMeta(taskGraphDataOde);


      //Get critical path
      //----------------------------------
      ((criticalPaths,cpCosts),(criticalPathsWoC,cpCostsWoC)) = HpcOmTaskGraph.getCriticalPaths(taskGraphOde,taskGraphDataOde);
      criticalPathInfo = HpcOmTaskGraph.dumpCriticalPathInfo((criticalPaths,cpCosts),(criticalPathsWoC,cpCostsWoC));
      ((graphOps,graphCosts)) = HpcOmTaskGraph.sumUpExeCosts(taskGraphOde,taskGraphDataOde);
      graphCosts = HpcOmTaskGraph.roundReal(graphCosts,2);
      criticalPathInfo = criticalPathInfo + " sum: (" + realString(graphCosts) + " ; " + intString(graphOps) + ")";
      fileName = ("taskGraph"+filenamePrefix+"ODE.graphml");
      schedulerInfo = arrayCreate(arrayLength(taskGraphOde), (-1,-1,-1.0));
      SimCodeUtil.execStat("hpcom assign levels / get crit. path");
      HpcOmTaskGraph.dumpAsGraphMLSccLevel(taskGraphOde, taskGraphDataOde,inBackendDAE, fileName, criticalPathInfo, HpcOmTaskGraph.convertNodeListToEdgeTuples(List.first(criticalPaths)), HpcOmTaskGraph.convertNodeListToEdgeTuples(List.first(criticalPathsWoC)), sccSimEqMapping, schedulerInfo, HpcOmTaskGraph.GRAPHDUMPOPTIONS(true,false,true,true));
      SimCodeUtil.execStat("hpcom dump ODE TaskGraph");

      if Flags.isSet(Flags.HPCOM_DUMP) then
        print("Critical Path successfully calculated\n");
      end if;

      // Analyse Systems of Equations
      //-----------------------------
      (scheduledTasks,scheduledDAENodes) = HpcOmEqSystems.parallelizeTornSystems(taskGraphOde,taskGraphDataOde,sccSimEqMapping,simVarMapping,inBackendDAE);


      //Apply filters
      //-------------
      taskGraphDataSimplified = taskGraphDataOde;
      taskGraphSimplified = taskGraphOde;
      (taskGraphSimplified,taskGraphDataSimplified) = applyGRS(taskGraphOde,taskGraphDataOde);
      SimCodeUtil.execStat("hpcom GRS");

      fileName = ("taskGraph"+filenamePrefix+"ODE_merged.graphml");
      HpcOmTaskGraph.dumpAsGraphMLSccLevel(taskGraphSimplified, taskGraphDataSimplified, inBackendDAE, fileName, criticalPathInfo, HpcOmTaskGraph.convertNodeListToEdgeTuples(List.first(criticalPaths)), HpcOmTaskGraph.convertNodeListToEdgeTuples(List.first(criticalPathsWoC)), sccSimEqMapping, schedulerInfo, HpcOmTaskGraph.GRAPHDUMPOPTIONS(true,false,true,true));
      SimCodeUtil.execStat("hpcom dump simplified TaskGraph");

      if Flags.isSet(Flags.HPCOM_DUMP) then
        print("Filter successfully applied. Merged "+intString(intSub(arrayLength(taskGraphOde),arrayLength(taskGraphSimplified)))+" tasks.\n");
      end if;

      //Create schedule
      //---------------
      numProc = Flags.getConfigInt(Flags.NUM_PROC);
      (numProc,_) = setNumProc(numProc,cpCostsWoC,taskGraphDataOde);//in case n-flag is not set
      (schedule,simCode,taskGraphScheduled,taskGraphDataScheduled,sccSimEqMapping) = createSchedule(taskGraphSimplified,taskGraphDataSimplified,sccSimEqMapping,simVarMapping,filenamePrefix,numProc,simCode,scheduledTasks);

      SimCode.SIMCODE(modelInfo, simCodeLiterals, simCodeRecordDecls, simCodeExternalFunctionIncludes, allEquations, odeEquations, algebraicEquations, residualEquations, useSymbolicInitialization, useHomotopy, initialEquations, removedInitialEquations, startValueEquations, nominalValueEquations, minValueEquations, maxValueEquations,
          parameterEquations, removedEquations, algorithmAndEquationAsserts, zeroCrossingsEquations, jacobianEquations, stateSets, constraints, classAttributes, zeroCrossings, relations, timeEvents, whenClauses,
          discreteModelVars, extObjInfo, makefileParams, delayedExps, jacobianMatrixes, simulationSettingsOpt, fileNamePrefix, _, _, _, crefToSimVarHT, backendMapping) = simCode;

      //(schedule,numProc) = repeatScheduleWithOtherNumProc(taskGraphSimplified,taskGraphDataSimplified,sccSimEqMapping,filenamePrefix,cpCostsWoC,schedule,numProc,numFixed);
      numProc = Flags.getConfigInt(Flags.NUM_PROC);
      criticalPathInfo = HpcOmScheduler.analyseScheduledTaskGraph(schedule,numProc,taskGraphScheduled,taskGraphDataScheduled);
      schedulerInfo = HpcOmScheduler.convertScheduleStrucToInfo(schedule,arrayLength(taskGraphScheduled));
      SimCodeUtil.execStat("hpcom create schedule");

      fileName = ("taskGraph"+filenamePrefix+"ODE_schedule.graphml");
      HpcOmTaskGraph.dumpAsGraphMLSccLevel(taskGraphScheduled, taskGraphDataScheduled, inBackendDAE, fileName, criticalPathInfo, HpcOmTaskGraph.convertNodeListToEdgeTuples(List.first(criticalPaths)), HpcOmTaskGraph.convertNodeListToEdgeTuples(List.first(criticalPathsWoC)), sccSimEqMapping, schedulerInfo, HpcOmTaskGraph.GRAPHDUMPOPTIONS(true,false,true,true));
      //HpcOmScheduler.printSchedule(schedule);

      SimCodeUtil.execStat("hpcom dump schedule TaskGraph");

      if Flags.isSet(Flags.HPCOM_DUMP) then
        print("Schedule created\n");
      end if;

      //Check ODE-System size
      //---------------------
      System.realtimeTick(ClockIndexes.RT_CLOCK_EXECSTAT_HPCOM_MODULES);
      checkOdeSystemSize(taskGraphOde,odeEquations);
      SimCodeUtil.execStat("hpcom check ODE system size");

      //Create Memory-Map and Sim-Code
      //------------------------------
      optTmpMemoryMap = HpcOmMemory.createMemoryMap(modelInfo, taskGraphSimplified, taskGraphDataSimplified, eqs, filenamePrefix, schedulerInfo, schedule, sccSimEqMapping, criticalPaths, criticalPathsWoC, criticalPathInfo, numProc, allComps);
      SimCodeUtil.execStat("hpcom create memory map");

      equationsForConditions = allEquations; //getSimCodeEqsByTaskList(eventSystemTaskList, simEqIdxSimEqMapping);

      simCode = SimCode.SIMCODE(modelInfo, simCodeLiterals, simCodeRecordDecls, simCodeExternalFunctionIncludes, allEquations, odeEquations, algebraicEquations, residualEquations, useSymbolicInitialization, useHomotopy, initialEquations, removedInitialEquations, startValueEquations, nominalValueEquations, minValueEquations, maxValueEquations,
                 parameterEquations, removedEquations, algorithmAndEquationAsserts, zeroCrossingsEquations, jacobianEquations, stateSets, constraints, classAttributes, zeroCrossings, relations, timeEvents, whenClauses,
                 discreteModelVars, extObjInfo, makefileParams, delayedExps, jacobianMatrixes, simulationSettingsOpt, fileNamePrefix, SOME(schedule), optTmpMemoryMap, equationsForConditions, crefToSimVarHT, backendMapping);

      //evaluateCacheBehaviour(schedulerInfo,taskGraphDataSimplified,clTaskMapping, transposeCacheLineTaskMapping(clTaskMapping, arrayLength(taskGraphSimplified)));
      SimCodeUtil.execStat("hpcom other");
      print("HpcOm is still under construction.\n");
      then simCode;
    else equation
      Error.addMessage(Error.INTERNAL_ERROR, {"function createSimCode failed."});
    then fail();
  end matchcontinue;
end createSimCode;

protected function createAndExportInitialSystemTaskGraph "author: marcusw
  Create the task graph for the initial system and write it to graphML."
  input Option<BackendDAE.BackendDAE> iInitDae;
  input String iFileNamePrefix;
protected
  BackendDAE.BackendDAE initDAE;
  HpcOmTaskGraph.TaskGraph tmpTaskGraph;
  HpcOmTaskGraph.TaskGraphMeta tmpTaskGraphMeta;
  String fileName;
  array<list<Integer>> sccSimEqMapping;
  array<tuple<Integer,Integer,Real>> schedulerInfo;
algorithm
  _ := match(iInitDae, iFileNamePrefix)
    case(SOME(initDAE), _)
      equation
        (tmpTaskGraph, tmpTaskGraphMeta) = HpcOmTaskGraph.createTaskGraph(initDAE);
        fileName = ("taskGraph"+iFileNamePrefix+"_init.graphml");
        schedulerInfo = arrayCreate(arrayLength(tmpTaskGraph), (-1,-1,-1.0));
        sccSimEqMapping = arrayCreate(arrayLength(tmpTaskGraph), {});
        HpcOmTaskGraph.dumpAsGraphMLSccLevel(tmpTaskGraph, tmpTaskGraphMeta, initDAE, fileName, "", {}, {}, sccSimEqMapping ,schedulerInfo, HpcOmTaskGraph.GRAPHDUMPOPTIONS(false,false,true,true));
      then ();
    else
      then ();
  end match;
end createAndExportInitialSystemTaskGraph;

protected function setNumProc "sets the number of processors. its upper limit is the number of processsors provided by the system.
if no n-flag is set, a ideal number is suggested but the simulation fails.
author: Waurich TUD 2013-11"
  input Integer numProcFlag;
  input Real cpCosts;
  input HpcOmTaskGraph.TaskGraphMeta taskGraphMetaIn;
  output Integer numProcOut;
  output Boolean numFixed;
algorithm
  (numProcOut,numFixed) := match(numProcFlag,cpCosts,taskGraphMetaIn)
    local
      Boolean isFixed;
      Integer numProcSys, numProc, numProcSched;
      Real serCosts, maxSpeedUp;
      String string1, string2;
    case(0,_,_)
      equation
        serCosts = HpcOmScheduler.getSerialExecutionTime(taskGraphMetaIn);
        maxSpeedUp = realDiv(serCosts,cpCosts);
        numProcSched = realInt(realAdd(maxSpeedUp,1.0));
        numProcSys = System.numProcessors();
        numProc = intMin(numProcSched,numProcSys);
        string1 = "Your system provides only "+intString(numProcSys)+" processors!\n";
        string2 = intString(numProcSched)+" processors might be a reasonable number of processors.\n";
        string1 = if intGt(numProcSched,numProcSys) then string1 else string2;
        print("Please set the number of processors you want to use!\n");
        print(string1);
        Flags.setConfigInt(Flags.NUM_PROC,numProc);
      then
        (numProc,true);
    else
      equation
        numProcSys = System.numProcessors();
        _ = if intGt(numProcFlag,numProcSys) then numProcSys else numProcFlag; // the system does not provide so many cores
        if intGt(numProcFlag,numProcSys) and Flags.isSet(Flags.HPCOM_DUMP) then
          print("Warning: Your system provides only "+intString(numProcSys)+" processors!\n");
        end if;
      then
        (numProcFlag,true);
  end match;
end setNumProc;


public function applyGRS"applies several task graph rewriting rules to merge tasks. builds a new incidence matrix for the task graph after finishing ther merging
author:Waurich 2014-11"
  input HpcOmTaskGraph.TaskGraph iTaskGraph;
  input HpcOmTaskGraph.TaskGraphMeta iTaskGraphMeta;
  output HpcOmTaskGraph.TaskGraph oTaskGraph;
  output HpcOmTaskGraph.TaskGraphMeta oTaskGraphMeta;
protected
    HpcOmTaskGraph.TaskGraph taskGraph1,taskGraphT;
    HpcOmTaskGraph.TaskGraphMeta taskGraphMeta1;
    array<Integer> contractedTasks;
 algorithm
   taskGraph1 := arrayCopy(iTaskGraph);
   taskGraphT := BackendDAEUtil.transposeMatrix(taskGraph1,arrayLength(taskGraph1));
   taskGraphMeta1 := HpcOmTaskGraph.copyTaskGraphMeta(iTaskGraphMeta);
   contractedTasks := arrayCreate(arrayLength(taskGraph1),0);
   // contract nodes in the graph
   (taskGraph1,taskGraphMeta1) := applyGRS1(taskGraph1,taskGraphT,taskGraphMeta1,contractedTasks,true);
   // build new taskGraph
   (oTaskGraph,oTaskGraphMeta) := GRS_newGraph(taskGraph1,taskGraphMeta1,contractedTasks);
end applyGRS;


public function applyGRS1"applies several task graph rewriting rules to merge tasks.
author:Waurich 2014-11"
  input HpcOmTaskGraph.TaskGraph iTaskGraph;
  input HpcOmTaskGraph.TaskGraph iTaskGraphT;
  input HpcOmTaskGraph.TaskGraphMeta iTaskGraphMeta;
  input array<Integer> contractedTasksIn;
  input Boolean again;
  output HpcOmTaskGraph.TaskGraph oTaskGraph;
  output HpcOmTaskGraph.TaskGraphMeta oTaskGraphMeta;
algorithm
  (oTaskGraph,oTaskGraphMeta) := matchcontinue(iTaskGraph,iTaskGraphT,iTaskGraphMeta,contractedTasksIn,again)
    local
      Boolean changed,changed2;
    case(_,_,_,_,true)
      equation
        //Merge nodes
        (_,_,_,_,changed) = HpcOmTaskGraph.mergeSimpleNodes(iTaskGraph, iTaskGraphT, iTaskGraphMeta, contractedTasksIn);
        //changed = false;

        (_,_,_,_,changed2) = HpcOmTaskGraph.mergeParentNodes(iTaskGraph, iTaskGraphT, iTaskGraphMeta, contractedTasksIn);
        changed = changed or changed2;

        //HpcOmTaskGraph.TASKGRAPHMETA(inComps=inComps,nodeMark=nodeMark) = taskGraphMeta1;
        //doNotMerge = List.map3(doNotMergeIn,HpcOmTaskGraph.getCompInComps,1,inComps,nodeMark);
        //(taskGraph1,taskGraphMeta1,changed3) = HpcOmTaskGraph.mergeSingleNodes(taskGraph1, taskGraphMeta1, doNotMerge);

      then applyGRS1(iTaskGraph,iTaskGraphT,iTaskGraphMeta,contractedTasksIn,changed);
    else (iTaskGraph, iTaskGraphMeta);
  end matchcontinue;
end applyGRS1;

protected function GRS_newGraph"build a new task graph and update the inComps for the merged nodes.
author:Waurich TUD 2014-11"
  input HpcOmTaskGraph.TaskGraph graphIn;
  input HpcOmTaskGraph.TaskGraphMeta metaIn;
  input array<Integer> contrTasks;
  output HpcOmTaskGraph.TaskGraph graphOut;
  output HpcOmTaskGraph.TaskGraphMeta metaOut;
protected
  Integer newSize;
  list<Integer> notRemovedNodes,removedNodes;
  array<list<Integer>> inComps,inCompsNew;
algorithm
  HpcOmTaskGraph.TASKGRAPHMETA(inComps = inComps) := metaIn;
  notRemovedNodes := HpcOmTaskGraph.filterContractedNodes(List.intRange(arrayLength(graphIn)),contrTasks);
  removedNodes := HpcOmTaskGraph.filterNonContractedNodes(List.intRange(arrayLength(graphIn)),contrTasks);
  newSize := listLength(notRemovedNodes);
  graphOut := arrayCreate(newSize,{});
  inCompsNew := arrayCreate(newSize,{});
  (graphOut,inCompsNew) := GRS_newGraph2(notRemovedNodes,removedNodes,contrTasks,graphIn,inComps,graphOut,inCompsNew,1);
  metaOut := HpcOmTaskGraph.setInCompsInMeta(inCompsNew,metaIn);
end GRS_newGraph;

protected function GRS_newGraph2"build a new task graph and update the inComps for the merged nodes.
author: Waurich TUD 2014-11"
  input list<Integer> origNodes;
  input list<Integer> removedNodes;
  input array<Integer> contrTasks;
  input HpcOmTaskGraph.TaskGraph origGraph;
  input array<list<Integer>> origInComps;
  input HpcOmTaskGraph.TaskGraph newGraph;
  input array<list<Integer>> newInComps;
  input Integer newNode;
  output HpcOmTaskGraph.TaskGraph graphOut;
  output array<list<Integer>> inCompsOut;
algorithm
  (graphOut,inCompsOut) := matchcontinue(origNodes,removedNodes,contrTasks,origGraph,origInComps,newGraph,newInComps,newNode)
    local
      Integer node;
      list<Integer> rest,row,comps;
    case({},_,_,_,_,_,_,_)
      equation
      then (newGraph,newInComps);
    case(node::rest,_,_,_,_,_,_,_)
      equation
      //print("origNode "+intString(node)+" and newNode "+intString(newNode)+"\n");
      row = arrayGet(origGraph,node);
      row = HpcOmTaskGraph.filterContractedNodes(row,contrTasks);
      row = HpcOmTaskGraph.updateContinuousEntriesInList(row,removedNodes);

      comps = arrayGet(origInComps,node);
      //print("comps1 "+stringDelimitList(List.map(comps,intString),", ")+"\n");
      arrayUpdate(newGraph,newNode,row);
      comps = List.sort(comps,intGt);
      arrayUpdate(newInComps,newNode,comps);
    then GRS_newGraph2(rest,removedNodes,contrTasks,origGraph,origInComps,newGraph,newInComps,newNode+1);
  end matchcontinue;
end GRS_newGraph2;


protected function createSchedule
  input HpcOmTaskGraph.TaskGraph iTaskGraph;
  input HpcOmTaskGraph.TaskGraphMeta iTaskGraphMeta;
  input array<list<Integer>> iSccSimEqMapping;
  input array<list<SimCodeVar.SimVar>> iSimVarMapping;
  input String iFilenamePrefix;
  input Integer numProc;
  input SimCode.SimCode iSimCode;
  input list<HpcOmSimCode.Task> scheduledTasks;
  output HpcOmSimCode.Schedule oSchedule;
  output SimCode.SimCode oSimCode;
  output HpcOmTaskGraph.TaskGraph oTaskGraph;
  output HpcOmTaskGraph.TaskGraphMeta oTaskGraphMeta;
  output array<list<Integer>> oSccSimEqMapping;
protected
  String flagValue;
  list<Integer> lst;
  array<list<Integer>> sccSimEqMap;
  HpcOmSimCode.Schedule schedule;
  HpcOmTaskGraph.TaskGraph taskGraph1;
  HpcOmTaskGraph.TaskGraphMeta taskGraphMeta1;
  SimCode.SimCode simCode;
algorithm
  (oSchedule,oSimCode,oTaskGraph,oTaskGraphMeta,oSccSimEqMapping) := matchcontinue(iTaskGraph,iTaskGraphMeta,iSccSimEqMapping,iSimVarMapping,iFilenamePrefix,numProc,iSimCode,scheduledTasks)
    case(_,_,_,_,_,_,_,_)
      equation
        true = arrayLength(iTaskGraph) == 0;
        print("There is no ODE system that can be parallelized!\n");
        schedule = HpcOmScheduler.createEmptySchedule(iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
      then
        (schedule,iSimCode,iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "none");
        print("Using serial code\n");
        schedule = HpcOmScheduler.createEmptySchedule(iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
      then
        (schedule,iSimCode,iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "level");
        print("Using level Scheduler\n");
        (schedule,taskGraphMeta1) = HpcOmScheduler.createLevelSchedule(iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
      then (schedule,iSimCode,iTaskGraph,taskGraphMeta1,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "levelfix");
        print("Using fixed level Scheduler (experimental)\n");
        (schedule,taskGraphMeta1) = HpcOmScheduler.createFixedLevelSchedule(iTaskGraph,iTaskGraphMeta,numProc,iSccSimEqMapping);
      then (schedule,iSimCode,iTaskGraph,taskGraphMeta1,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "ext");
        print("Using external Scheduler\n");
        schedule = HpcOmScheduler.createExtSchedule(iTaskGraph, iTaskGraphMeta, numProc, iSccSimEqMapping, iSimVarMapping, "taskGraph" + iFilenamePrefix + "_ext.graphml");
      then (schedule,iSimCode,iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "metis");
        print("Using METIS Scheduler\n");
        schedule = HpcOmScheduler.createMetisSchedule(iTaskGraph, iTaskGraphMeta, numProc, iSccSimEqMapping, iSimVarMapping);
      then (schedule,iSimCode,iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "hmet");
        print("Using hMETIS Scheduler\n");
        schedule = HpcOmScheduler.createHMetisSchedule(iTaskGraph, iTaskGraphMeta, numProc, iSccSimEqMapping,iSimVarMapping);
      then (schedule,iSimCode,iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "listr");
        print("Using list reverse Scheduler\n");
        schedule = HpcOmScheduler.createListScheduleReverse(iTaskGraph,iTaskGraphMeta,numProc,iSccSimEqMapping, iSimVarMapping);
      then (schedule,iSimCode,iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "rand");
        print("Using Random Scheduler\n");
        schedule = HpcOmScheduler.createRandomSchedule(iTaskGraph, iTaskGraphMeta, numProc, iSccSimEqMapping, iSimVarMapping);
      then (schedule, iSimCode, iTaskGraph, iTaskGraphMeta, iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "list");
        print("Using list Scheduler\n");
        schedule = HpcOmScheduler.createListSchedule(iTaskGraph,iTaskGraphMeta,numProc,iSccSimEqMapping, iSimVarMapping);
      then (schedule,iSimCode,iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "mcp");
        print("Using Modified Critical Path Scheduler\n");
        schedule = HpcOmScheduler.createMCPschedule(iTaskGraph,iTaskGraphMeta,numProc,iSccSimEqMapping,iSimVarMapping);
      then (schedule,iSimCode,iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "taskdep");
        print("Using dynamic task dependencies\n");
        schedule = HpcOmScheduler.createTaskDepSchedule(iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
      then (schedule,iSimCode,iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "tds");
        print("Using Task Duplication-based Scheduling\n");
        (schedule,simCode,taskGraph1,taskGraphMeta1,sccSimEqMap) = HpcOmScheduler.TDS_schedule(iTaskGraph,iTaskGraphMeta,numProc,iSccSimEqMapping,iSimVarMapping,iSimCode);
      then (schedule,simCode,taskGraph1,taskGraphMeta1,sccSimEqMap);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "bls");
        print("Using Balanced Level Scheduling\n");
        (schedule,taskGraphMeta1) = HpcOmScheduler.createBalancedLevelScheduling(iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
      then (schedule,iSimCode,iTaskGraph,taskGraphMeta1,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        true = stringEq(flagValue, "sbs");
        print("Using Single Block Schedule\n");
        schedule = HpcOmEqSystems.createSingleBlockSchedule(iTaskGraph,iTaskGraphMeta,scheduledTasks,iSccSimEqMapping);
      then (schedule,iSimCode,iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
    case(_,_,_,_,_,_,_,_)
      equation
        flagValue = Flags.getConfigString(Flags.HPCOM_SCHEDULER);
        print("HpcOmScheduler.createSchedule warning: The scheduler '" + flagValue + "' is unknown. The list-scheduling algorithm is used instead.\n");
        schedule = HpcOmScheduler.createListSchedule(iTaskGraph,iTaskGraphMeta,numProc,iSccSimEqMapping,iSimVarMapping);
      then (schedule,iSimCode,iTaskGraph,iTaskGraphMeta,iSccSimEqMapping);
    else
      equation
        print("HpcOmSimCode.createSchedule failed. Maybe the n-Flag was not set correctly\n");
      then fail();
  end matchcontinue;
end createSchedule;

protected function convertToSccSimEqMapping "function convertToSccSimEqMapping
  author: marcusw
  Converts the given mapping (simEqIndex -> sccIndex) to the inverse mapping (sccIndex->simEqIndex)."
  input list<tuple<Integer,Integer>> iMapping; //the mapping (simEqIndex -> sccIndex)
  input Integer numOfSccs; //important for arrayCreate
  output array<list<Integer>> oMapping; //the created mapping (sccIndex->simEqIndex)

protected
  array<list<Integer>> tmpMapping;

algorithm
  tmpMapping := arrayCreate(numOfSccs,{});
  //print("convertToSccSimEqMapping with " + intString(numOfSccs) + " sccs.\n");
  _ := List.fold(iMapping, convertToSccSimEqMapping1, tmpMapping);
  oMapping := tmpMapping;

end convertToSccSimEqMapping;

protected function convertToSccSimEqMapping1 "function convertToSccSimEqMapping1
  author: marcusw
  Helper function for convertToSccSimEqMapping. It will update the arrayIndex of the given mapping value."
  input tuple<Integer,Integer> iMapping; //<simEqIdx,sccIdx>
  input array<list<Integer>> iSccMapping;
  output array<list<Integer>> oSccMapping;

protected
  Integer i1,i2;
  List<Integer> tmpList;

algorithm
  (i1,i2) := iMapping;
  //print("convertToSccSimEqMapping1 accessing index " + intString(i2) + ".\n");
  tmpList := arrayGet(iSccMapping,i2);
  tmpList := i1 :: tmpList;
  oSccMapping := arrayUpdate(iSccMapping,i2,tmpList);

end convertToSccSimEqMapping1;

protected function convertToSimeqCompMapping "function convertToSimeqCompMapping
  author: marcusw
  Converts the given mapping (simEqIndex -> sccIndex) bases on tuples to an array mapping."
  input list<tuple<Integer,Integer>> iMapping; //<simEqIdx,sccIdx>
  input Integer numOfSimEqs;
  output array<Integer> oMapping; //maps each simEq to the scc

protected
  array<Integer> tmpMapping;

algorithm
  tmpMapping := arrayCreate(numOfSimEqs, -1);
  oMapping := List.fold(iMapping, convertToSimeqCompMapping1, tmpMapping);
end convertToSimeqCompMapping;

protected function convertToSimeqCompMapping1 "function convertToSimeqCompMapping1
  author: marcusw
  Helper function for convertToSimeqCompMapping. It will update the array at the given index."
  input tuple<Integer,Integer> iSimEqTuple; //<simEqIdx,sccIdx>
  input array<Integer> iMapping;
  output array<Integer> oMapping;

protected
  Integer simEqIdx,sccIdx;

algorithm
  (simEqIdx,sccIdx) := iSimEqTuple;
  //print("convertToSimeqCompMapping1 " + intString(simEqIdx) + " .. " + intString(sccIdx) + " iMapping_len: " + intString(arrayLength(iMapping)) + "\n");
  oMapping := arrayUpdate(iMapping,simEqIdx,sccIdx);
end convertToSimeqCompMapping1;

protected function getSimEqIdxSimEqMapping "author: marcusw
  Get a mapping from simEqIdx -> option(simEq)."
  input list<SimCode.SimEqSystem> iAllEquations;
  input Integer iSimEqSystemHighestIdx;
  output array<Option<SimCode.SimEqSystem>> oMapping;
protected
  array<Option<SimCode.SimEqSystem>> tmpMapping;
algorithm
  tmpMapping := arrayCreate(iSimEqSystemHighestIdx, NONE());
  oMapping := List.fold(iAllEquations, getSimEqIdxSimEqMapping1, tmpMapping);
end getSimEqIdxSimEqMapping;

protected function getSimEqIdxSimEqMapping1 "author: marcusw
  Helper function that adds the index of the given equation to the mapping."
  input SimCode.SimEqSystem iEquation;
  input array<Option<SimCode.SimEqSystem>> iMapping;
  output array<Option<SimCode.SimEqSystem>> oMapping;
protected
  Integer simEqIdx;
  array<Option<SimCode.SimEqSystem>> tmpMapping;
algorithm
  oMapping := matchcontinue(iEquation, iMapping)
    case(_,_)
      equation
        simEqIdx = getIndexBySimCodeEq(iEquation);
        tmpMapping = arrayUpdate(iMapping, simEqIdx, SOME(iEquation));
      then tmpMapping;
    else
      equation
        simEqIdx = getIndexBySimCodeEq(iEquation);
        //print("getSimEqIdxSimEqMapping1: Can't access idx " + intString(simEqIdx) + "\n");
      then iMapping;
  end matchcontinue;
end getSimEqIdxSimEqMapping1;

public function dumpSimEqSCCMapping "author: marcusw
  Prints the given mapping out to the console."
  input array<Integer> iSccMapping;
protected
  String text;
algorithm
  text := "SimEqToSCCMapping";
  ((_,text)) := Array.fold(iSccMapping, dumpSimEqSCCMapping1, (1,text));
  print(text + "\n");
end dumpSimEqSCCMapping;

protected function dumpSimEqSCCMapping1 "author: marcusw
  Helper function of dumpSimEqSCCMapping to print one mapping entry."
  input Integer iMapping;
  input tuple<Integer,String> iIndexText;
  output tuple<Integer,String> oIndexText;
protected
  Integer iIndex;
  String text, iText;
algorithm
  (iIndex,iText) := iIndexText;
  text := intString(iMapping);
  text := iText + "\nSimEq " + intString(iIndex) + ": {" + text + "}";
  oIndexText := (iIndex+1,text);
end dumpSimEqSCCMapping1;

public function dumpSccSimEqMapping "function dumpSccSimEqMapping
  author: marcusw
  Prints the given mapping out to the console."
  input array<list<Integer>> iSccMapping;
protected
  String text;
algorithm
  text := "SccToSimEqMapping";
  ((_,text)) := Array.fold(iSccMapping, dumpSccSimEqMapping1, (1,text));
  print(text + "\n");
end dumpSccSimEqMapping;

protected function dumpSccSimEqMapping1 "function dumpSccSimEqMapping1
  author: marcusw
  Helper function of dumpSccSimEqMapping to print one mapping list."
  input list<Integer> iMapping;
  input tuple<Integer,String> iIndexText;
  output tuple<Integer,String> oIndexText;
protected
  Integer iIndex;
  String text, iText;
algorithm
  (iIndex,iText) := iIndexText;
  text := List.fold(iMapping, dumpSccSimEqMapping2, " ");
  text := iText + "\nSCC " + intString(iIndex) + ": {" + text + "}";
  oIndexText := (iIndex+1,text);
end dumpSccSimEqMapping1;

protected function dumpSccSimEqMapping2 "function dumpSccSimEqMapping2
  author: marcusw
  Helper function of dumpSccSimEqMapping1 to print one mapping element."
  input Integer iIndex;
  input String iText;
  output String oText;

algorithm
  oText := iText + intString(iIndex) + " ";

end dumpSccSimEqMapping2;

protected function getSimCodeEqsByTaskList "author: marcusw
  Get the simCode.SimEqSystem - objects references by the given tasks."
  input list<HpcOmSimCode.Task> iTaskList;
  input array<Option<SimCode.SimEqSystem>> iSimEqIdxSimEqMapping;
  output list<SimCode.SimEqSystem> oSimEqs;
protected
  list<list<SimCode.SimEqSystem>> tmpSimEqs;
algorithm
  tmpSimEqs := List.map1(iTaskList, getSimCodeEqsByTaskList0, iSimEqIdxSimEqMapping);
  oSimEqs := List.flatten(tmpSimEqs);
end getSimCodeEqsByTaskList;

protected function getSimCodeEqsByTaskList0 "author: marcusw
  Get the simCode.SimEqSystem - objects references by the given task."
  input HpcOmSimCode.Task iTask;
  input array<Option<SimCode.SimEqSystem>> iSimEqIdxSimEqMapping;
  output list<SimCode.SimEqSystem> oSimEqs;
protected
  list<Integer> eqIdc;
  list<SimCode.SimEqSystem> tmpSimEqs;
algorithm
  oSimEqs := match(iTask, iSimEqIdxSimEqMapping)
    case(HpcOmSimCode.CALCTASK(eqIdc=eqIdc),_)
      equation
        tmpSimEqs = List.map1r(eqIdc, getSimCodeEqByIndexAndMapping, iSimEqIdxSimEqMapping);
      then tmpSimEqs;
    case(HpcOmSimCode.CALCTASK_LEVEL(eqIdc=eqIdc),_)
      equation
        tmpSimEqs = List.map1r(eqIdc, getSimCodeEqByIndexAndMapping, iSimEqIdxSimEqMapping);
      then tmpSimEqs;
    else then {};
  end match;
end getSimCodeEqsByTaskList0;

public function getSimCodeEqByIndexAndMapping "author: marcusw
  Returns the SimEqSystem which has the given Index."
  input array<Option<SimCode.SimEqSystem>> iSimEqIdxSimEqMapping; //All SimEqSystems
  input Integer iIdx; //The index of the required system
  output SimCode.SimEqSystem oSimEqSystem;
protected
  Option<SimCode.SimEqSystem> tmpSimEqSystem;
algorithm
  tmpSimEqSystem := arrayGet(iSimEqIdxSimEqMapping, iIdx);
  oSimEqSystem := getSimCodeEqByIndexAndMapping1(tmpSimEqSystem, iIdx);
end getSimCodeEqByIndexAndMapping;

protected function getSimCodeEqByIndexAndMapping1 "author: marcusw
  Returns the SimEqSystem if it's not NONE()."
  input Option<SimCode.SimEqSystem> iSimEqSystem;
  input Integer iIdx;
  output SimCode.SimEqSystem oSimEqSystem;
protected
  SimCode.SimEqSystem tmpSys;
algorithm
  oSimEqSystem := match(iSimEqSystem,iIdx)
    case(SOME(tmpSys),_)
      then tmpSys;
    else
      equation
        print("getSimCodeEqByIndexAndMapping1 failed. Looking for Index " + intString(iIdx) + "\n");
        //print(" -- available indices: " + stringDelimitList(List.map(List.map(iEqs,getIndexBySimCodeEq), intString), ",") + "\n");
      then fail();
  end match;
end getSimCodeEqByIndexAndMapping1;

public function getSimCodeEqByIndex "function getSimCodeEqByIndex
  author: marcusw
  Returns the SimEqSystem which has the given Index. This method is called from susan."
  input list<SimCode.SimEqSystem> iEqs; //All SimEqSystems
  input Integer iIdx; //The index of the wanted system
  output SimCode.SimEqSystem oEq;

protected
  list<SimCode.SimEqSystem> rest;
  SimCode.SimEqSystem head;
  Integer headIdx;

algorithm
  oEq := matchcontinue(iEqs,iIdx)
    case(head::rest,_)
      equation
        headIdx = getIndexBySimCodeEq(head);
        //print("getSimCodeEqByIndex listLength: " + intString(listLength(iEqs)) + " head idx: " + intString(headIdx) + "\n");
        true = intEq(headIdx,iIdx);
      then head;
    case(head::rest,_) then getSimCodeEqByIndex(rest,iIdx);
    else
      equation
        print("getSimCodeEqByIndex failed. Looking for Index " + intString(iIdx) + "\n");
        //print(" -- available indices: " + stringDelimitList(List.map(List.map(iEqs,getIndexBySimCodeEq), intString), ",") + "\n");
      then fail();
  end matchcontinue;
end getSimCodeEqByIndex;

protected function getIndexBySimCodeEq "function getIndexBySimCodeEq
  author: marcusw
  Just a small helper function to get the index of a SimEqSystem."
  input SimCode.SimEqSystem iEq;
  output Integer oIdx;

protected
  Integer index;

algorithm
  oIdx := match(iEq)
    case(SimCode.SES_RESIDUAL(index=index)) then index;
    case(SimCode.SES_SIMPLE_ASSIGN(index=index)) then index;
    case(SimCode.SES_ARRAY_CALL_ASSIGN(index=index)) then index;
    case(SimCode.SES_IFEQUATION(index=index)) then index;
    case(SimCode.SES_ALGORITHM(index=index)) then index;
    case(SimCode.SES_LINEAR(index=index)) then index;
    case(SimCode.SES_NONLINEAR(index=index)) then index;
    case(SimCode.SES_MIXED(index=index)) then index;
    case(SimCode.SES_WHEN(index=index)) then index;
    else fail();
  end match;
end getIndexBySimCodeEq;

protected function findHighestSccIdxInMapping "function findHighestSccIdxInMapping
  author: marcusw
  Find the highest scc-index in the mapping list."
  input list<tuple<Integer,Integer>> iEquationSccMapping; //<simEqIdx,sccIdx>
  input Integer iHighestIndex;
  output Integer oIndex;

protected
  Integer eqIdx, sccIdx;
  list<tuple<Integer,Integer>> rest;
algorithm
  oIndex := matchcontinue(iEquationSccMapping,iHighestIndex)
    case((eqIdx,sccIdx)::rest,_)
      equation
        true = intGt(sccIdx,iHighestIndex);
      then findHighestSccIdxInMapping(rest,sccIdx);
    case((eqIdx,sccIdx)::rest,_)
      then findHighestSccIdxInMapping(rest,iHighestIndex);
    else
      then iHighestIndex;
  end matchcontinue;
end findHighestSccIdxInMapping;

protected function removeDummyStateFromMapping "function removeDummyStateFromMapping
  author: marcusw
  Removes all mappings with sccIdx=1 from the list and decrements all other scc-indices by 1."
  input list<tuple<Integer,Integer>> iEquationSccMapping;
  output list<tuple<Integer,Integer>> oEquationSccMapping;
algorithm
  oEquationSccMapping := List.fold(iEquationSccMapping, removeDummyStateFromMapping1, {});
end removeDummyStateFromMapping;

protected function removeDummyStateFromMapping1 "function removeDummyStateFromMapping1
  author: marcusw
  Helper function of removeDummyStateFromMapping. Handles one list-element."
  input tuple<Integer,Integer> iTuple; //<eqIdx,sccIdx>
  input list<tuple<Integer,Integer>> iNewList;
  output list<tuple<Integer,Integer>> oNewList;
protected
  Integer eqIdx,sccIdx;
  tuple<Integer,Integer> newElem;
algorithm
  oNewList := matchcontinue(iTuple,iNewList)
    case((eqIdx,sccIdx),_)
      equation
        true = intEq(sccIdx,1);
      then iNewList;
    case((eqIdx,sccIdx),_)
      equation
        newElem = (eqIdx,sccIdx-1);
      then newElem::iNewList;
    else
      equation
        print("removeDummyStateFromMapping1 failed\n");
    then iNewList;
  end matchcontinue;
end removeDummyStateFromMapping1;

// test functions
//------------------------------------------
//------------------------------------------

protected function checkOdeSystemSize " compares the size of the ode-taskgraph with the number of ode-equations in the simCode.
Remark: this can occure when asserts are added to the ode-system.
author:Waurich TUD 2013-07"
  input HpcOmTaskGraph.TaskGraph taskGraphOdeIn;
  input list<list<SimCode.SimEqSystem>> odeEqsIn;
algorithm
  _ := matchcontinue(taskGraphOdeIn,odeEqsIn)
    local
      Integer actualSize;
      Integer targetSize;
    case(_,_)
      equation
        targetSize = listLength(List.flatten(odeEqsIn));
        actualSize = arrayLength(taskGraphOdeIn);
        true = intEq(targetSize,actualSize);
        print("the ODE-system size is correct("+intString(actualSize)+")\n");
        then
          ();
    case(_,_)
      equation
        targetSize = listLength(List.flatten(odeEqsIn));
        actualSize = arrayLength(taskGraphOdeIn);
        true = intEq(targetSize,1) and intEq(actualSize,0);
        // there is a dummyDER in the simcode
        print("the ODE-system size is correct(0)\n");
        then
          ();
    else
      equation
        targetSize = listLength(List.flatten(odeEqsIn));
        actualSize = arrayLength(taskGraphOdeIn);
        print("the size should be "+intString(targetSize)+" but it is "+intString(actualSize)+" !\n");
        print("the ODE-system is NOT correct\n");
      then
        ();
  end matchcontinue;
end checkOdeSystemSize;

/*
protected function repeatScheduleWithOtherNumProc"checks if the scheduling with the given numProc is fine.
 if n=auto, more cores are available and more speedup could be achieved repeat schedule with increased num of procs.
 author:Waurich TUD 2013-011"
  input HpcOmTaskGraph.TaskGraph taskGraphIn;
  input HpcOmTaskGraph.TaskGraphMeta taskGraphMetaIn;
  input array<list<Integer>> sccSimEqMappingIn;
  input String fileNamePrefix;
  input Real cpCostsWoC;
  input HpcOmSimCode.Schedule scheduleIn;
  input Integer numProcIn;
  input Boolean numFixed;
  output HpcOmSimCode.Schedule scheduleOut;
  output Integer numProcOut;
protected
  Integer maxNumProc, maxIter;
  Real maxDiff;
algorithm
  maxNumProc := System.numProcessors();
  maxIter := 3;
  maxDiff := 0.5;
  (scheduleOut,numProcOut,_) := repeatScheduleWithOtherNumProc1(taskGraphIn,taskGraphMetaIn,sccSimEqMappingIn,fileNamePrefix,cpCostsWoC,scheduleIn,numProcIn,numFixed,maxNumProc,maxDiff,maxIter);
end repeatScheduleWithOtherNumProc;


protected function repeatScheduleWithOtherNumProc1"checks if the scheduling with the given numProc is fine.
 if n=auto, more cores are available and more speedup could be achieved repeat schedule with increased num of procs.
 author:Waurich TUD 2013-011"
  input HpcOmTaskGraph.TaskGraph taskGraphIn;
  input HpcOmTaskGraph.TaskGraphMeta taskGraphMetaIn;
  input BackendDAE.BackendDAE inDAE;
  input array<list<Integer>> sccSimEqMappingIn;
  input String fileNamePrefix;
  input Real cpCostsWoC;
  input HpcOmSimCode.Schedule scheduleIn;
  input Integer numProcIn;
  input Boolean numFixed;
  input Integer maxNumProc;
  input Real maxDiff;
  input Integer numIterIn;
  output HpcOmSimCode.Schedule scheduleOut;
  output Integer numProcOut;
  output Integer numIterOut;
algorithm
  (scheduleOut,numProcOut,numIterOut) := matchcontinue(taskGraphIn,taskGraphMetaIn,inDAE,sccSimEqMappingIn,fileNamePrefix,cpCostsWoC,scheduleIn,numProcIn,numFixed,maxNumProc,maxDiff,numIterIn)
    local
      Boolean scheduleAgain;
      Integer numProc, numIt;
      Real serTime,parTime,speedup,speedUp,speedUpMax,diff;
      HpcOmSimCode.Schedule schedule;
    case(_,_,_,_,_,_,_,_,true,_,_,_)
      equation // do not schedule again because the number of procs was given
        then
          (scheduleIn,numProcIn,0);
    case(_,_,_,_,_,_,_,_,false,_,_,_)
      equation
        true = numIterIn == 0; // the max number of schedules with increased num of procs
        then
          (scheduleIn,numProcIn,0);
    case(_,_,_,_,_,_,_,_,false,_,_,_)
      equation
        (_,_,speedUp,speedUpMax) = HpcOmScheduler.predictExecutionTime(scheduleIn,SOME(cpCostsWoC),numProcIn,taskGraphIn,taskGraphMetaIn);
        diff = speedUpMax -. speedUp;
        //print("the new speedUp with "+intString(numProcIn)+" processors: "+realString(speedUp)+"\n");
        true = diff <. maxDiff;
        //print("the schedule is fine\n");
      then
        (scheduleIn,numProcIn,numIterIn);
    else
      equation
        numProc = numProcIn+1; // increase the number of procs
        numIt = numIterIn-1; // lower the counter of scheduling runs
        scheduleAgain = intLe(numProc,maxNumProc);
        //print("schedule again\n");
        numProc = if_(scheduleAgain,numProc,numProcIn);
        numIt = if_(scheduleAgain,numIt,0);
        schedule= Debug.bcallret6(scheduleAgain,createSchedule,taskGraphIn,taskGraphMetaIn,sccSimEqMappingIn,fileNamePrefix,numProc,scheduleIn);
        (schedule,numProc,numIt) = repeatScheduleWithOtherNumProc1(taskGraphIn,taskGraphMetaIn,sccSimEqMappingIn,fileNamePrefix,cpCostsWoC,schedule,numProc,numFixed,maxNumProc,maxDiff,numIt);
      then
        (schedule,numProc,numIt);
  end matchcontinue;
end repeatScheduleWithOtherNumProc1;
*/

//----------------------------
// output data about operations in equations and composition of systems of equations
//----------------------------

public function outputTimeBenchmark"outputs infos about all equations and equationsystems of the strongComponents.
author:Waurich TUD "
  input HpcOmTaskGraph.TaskGraphMeta graphData;
  input BackendDAE.BackendDAE dae;
protected
  list<BackendDAE.StrongComponent> allComps;
  array<tuple<BackendDAE.EqSystem,Integer>> systCompEqSysMapping;
  array<tuple<Integer,Real>> exeCosts;
  list<Real> numCycles;
  BackendDAE.Shared shared;
algorithm
  BackendDAE.DAE(shared=shared) := dae;
  (allComps,systCompEqSysMapping) := HpcOmTaskGraph.getSystemComponents(dae);
  HpcOmTaskGraph.TASKGRAPHMETA(exeCosts=exeCosts) := graphData;
  numCycles := List.map(arrayList(exeCosts),Util.tuple22);
  print("compIdx\ttype\tnumAdd\tnumMul\tnumTrig\tsize\tdensity\totherEqs\tmeasuredCycles\n");
  print("start cost benchmark\n");
  outputTimeBenchmark2(List.intRange(listLength(allComps)),allComps,List.map(numCycles,realInt),systCompEqSysMapping,shared);
    print("finish cost benchmark\n");
end outputTimeBenchmark;

protected function outputTimeBenchmark2
  input list<Integer> sccIndex;
  input BackendDAE.StrongComponents allComps;
  input list<Integer> numCycles;
  input array<tuple<BackendDAE.EqSystem,Integer>> systemCompEqSysMapping;
  input BackendDAE.Shared shared;
algorithm
  _ := matchcontinue(sccIndex,allComps,numCycles,systemCompEqSysMapping,shared)
    local
      Integer idx, numAdd, numMul, numTrig, cycles;
      list<Integer> rest;
      BackendDAE.StrongComponent comp;
      BackendDAE.EqSystem eqSystem;
    case({},_,_,_,_)
      equation
      then ();
    case(_,_,_,_,_)
      equation
        true = listLength(allComps) <> listLength(numCycles);
        print("The number of backenddae comps and measured comps is not the same!\n");
      then ();
    case(idx::rest,_,_,_,_)
      equation
        comp = listGet(allComps,idx);
        cycles = listGet(numCycles,idx);
        eqSystem = Util.tuple21(arrayGet(systemCompEqSysMapping,idx));
        print("\n"+intString(idx)+" : ");
        outputTimeBenchmark3(comp,cycles,eqSystem,shared);
        outputTimeBenchmark2(rest,allComps,numCycles,systemCompEqSysMapping,shared);
      then ();
    else
      equation
        print("outputTimeBenchmark2 failed!\n");
      then fail();
  end matchcontinue;
end outputTimeBenchmark2;

protected function outputTimeBenchmark3
  input BackendDAE.StrongComponent comp;
  input Integer cycles;
  input BackendDAE.EqSystem eqSystem;
  input BackendDAE.Shared shared;
algorithm
  _ := matchcontinue(comp,cycles,eqSystem,shared)
    local
      Integer idx, numAdd, numMul, numTrig, density, size, others;
      list<Integer> rest, resEqs, eqs;
      list<tuple<Integer, Integer, BackendDAE.Equation>> jac;
      list<tuple<Integer,list<Integer>>> otherEqnVarTpl;

      BackendDAE.EquationArray eqns;
       BackendDAE.Variables vars;

   case(BackendDAE.SINGLEEQUATION(),_,_,_)
      equation
        //BackendDAE.EQSYSTEM(orderedVars=vars,orderedEqs=eqns) = eqSystem;
        //BackendDump.dumpEqnsSolved2({comp},eqns,vars);

        ((numAdd,numMul,numTrig)) = HpcOmTaskGraph.countOperations(comp,eqSystem,shared);
        density = -1;
        size = -1;
        others = -1;
        print("\tSE\t"+intString(numAdd)+"\t"+intString(numMul)+"\t"+intString(numTrig)+"\t"+intString(size)+"\t"+intString(density)+"\t"+intString(others)+"\t"+intString(cycles)+"\n");
      then ();
   case(BackendDAE.EQUATIONSYSTEM(eqns=eqs,jac=BackendDAE.FULL_JACOBIAN(jacobian = SOME(jac))),_,_,_)
      equation
        //BackendDAE.EQSYSTEM(orderedVars=vars,orderedEqs=eqns) = eqSystem;
        //BackendDump.dumpEqnsSolved2({comp},eqns,vars);

        ((numAdd,numMul,numTrig)) = HpcOmTaskGraph.countOperations(comp,eqSystem,shared);
        size = listLength(eqs);
        density = realInt(realMul(realDiv(intReal(listLength(jac)),intReal(intMul(size,size))),100.0));
        others = -1;
        print("\tEQS\t"+intString(numAdd)+"\t"+intString(numMul)+"\t"+intString(numTrig)+"\t"+intString(size)+"\t"+intString(density)+"\t"+intString(others)+"\t"+intString(cycles)+"\n");
      then ();
    case(BackendDAE.TORNSYSTEM(residualequations=resEqs,otherEqnVarTpl=otherEqnVarTpl,linear=true,jac=BackendDAE.FULL_JACOBIAN(jacobian = SOME(jac))),_,_,_)
      equation
        //BackendDAE.EQSYSTEM(orderedVars=vars,orderedEqs=eqns) = eqSystem;
        //BackendDump.dumpEqnsSolved2({comp},eqns,vars);

        ((numAdd,numMul,numTrig)) = HpcOmTaskGraph.countOperations(comp,eqSystem,shared);
        size = listLength(resEqs);
        density = realInt(realMul(realDiv(intReal(listLength(jac)),intReal(intMul(size,size))),100.0));
        others = listLength(otherEqnVarTpl);
        print("\tTLS1\t"+intString(numAdd)+"\t"+intString(numMul)+"\t"+intString(numTrig)+"\t"+intString(size)+"\t"+intString(density)+"\t"+intString(others)+"\t"+intString(cycles)+"\n");
      then ();
    case(BackendDAE.TORNSYSTEM(residualequations=resEqs,otherEqnVarTpl=otherEqnVarTpl,linear=true,jac=BackendDAE.GENERIC_JACOBIAN()),_,_,_)
      equation
        //BackendDAE.EQSYSTEM(orderedVars=vars,orderedEqs=eqns) = eqSystem;
        //BackendDump.dumpEqnsSolved2({comp},eqns,vars);

        ((numAdd,numMul,numTrig)) = HpcOmTaskGraph.countOperations(comp,eqSystem,shared);
        density = 0;
        size = listLength(resEqs);
        others = listLength(otherEqnVarTpl);
        print("\tTLS2\t"+intString(numAdd)+"\t"+intString(numMul)+"\t"+intString(numTrig)+"\t"+intString(size)+"\t"+intString(density)+"\t"+intString(others)+"\t"+intString(cycles)+"\n");
      then ();
    else
      equation
        print("\tSTUSS\t"+intString(-1)+"\t"+intString(-1)+"\t"+intString(-1)+"\t"+intString(-1)+"\t"+intString(-1)+"\t"+intString(-1)+"\t"+intString(-1)+"\n");
      then ();
  end matchcontinue;
end outputTimeBenchmark3;

annotation(__OpenModelica_Interface="backend");
end HpcOmSimCodeMain;
