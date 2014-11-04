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

encapsulated package StateMachineFeatures
" file:        StateMachineFeatures.mo
  package:     StateMachineFeatures
  description: Provides support for Modelica State Machines.

  RCS: $Id$"

public import Absyn;
public import BackendDAE;
public import DAE;

protected import BackendDAEOptimize;
protected import BackendDAEUtil;
protected import BackendDump;
protected import BackendEquation;
protected import BackendVariable;
protected import ComponentReference;
protected import DAEUtil;
protected import Debug;
protected import Error;
protected import Flags;
protected import List;
protected import Util;
protected import HashSet;
protected import BaseHashSet;
protected import HashTableSM;
protected import Array;

/*
public
uniontype Transition "
Tbd"
  record TRANSITION
    DAE.ComponentRef to;
  end TRANSITION;
end Transition;
*/

/*
public
uniontype Automaton
  record AUTOMATON
    DAE.ComponentRef initialState;
    list<Mode> modes;
    //Transition t[:] "Array of transition data sorted in priority";
    //Modes q;
  end AUTOMATON;
end Automaton;
*/

public
uniontype Mode "
Tbd"
  record MODE
    String name;
    Boolean isInitial;
    HashSet.HashSet edges; // needed?
    BackendDAE.EquationArray eqs;
//    list<Automaton> refinement;
  end MODE;
end Mode;

// Table having crefs as keys and corresponding MODE as value
type ModeTable = HashTableSM.HashTable;

public
uniontype IncidenceTable
  record INCIDENCE_TABLE
    HashTable.HashTable cref2index "Map cref to corresponding index in incidence matrix";
    Boolean incidence[:,:] "Incidence matrix showing which modes are connected by transitions";
  end INCIDENCE_TABLE;
end IncidenceTable;

public
uniontype FlatAutomaton
  record FLAT_AUTOMATON
    DAE.ComponentRef initialState;
    HashSet.HashSet states;
  end FLAT_AUTOMATON;
end FlatAutomaton;

public
uniontype Composition
  record COMPOSITION
    DAE.ComponentRef refined;
    list<Composition> refining;
  end COMPOSITION;
end Composition;


public function stateMachineElab
  "Elaborate state machines and transform them in data-flow equations."
  input BackendDAE.BackendDAE inDAE;
  output BackendDAE.BackendDAE outDAE;
protected
  ModeTable modes;
  Integer nModes;
  list<String> names;
  IncidenceTable iTable;
  IncidenceTable transClosure;
  list<DAE.ComponentRef> initialStates;
  list<FlatAutomaton> flatAutomata;
  list<Composition> compositions;
  list<String> ss;
  BackendDAE.EqSystem syst, systNew;
  BackendDAE.Shared shared, sharedNew;
algorithm
  (syst, shared) := match inDAE
    local
      BackendDAE.EqSystem syst1;
      BackendDAE.Shared shared1;
    case (BackendDAE.DAE({syst1}, shared1)) then (syst1, shared1);
    else equation
      BackendDAE.DAE({syst1}, shared1) = BackendDAEOptimize.collapseIndependentBlocks(inDAE);
    then (syst1, shared1);
  end match;

  // Identify modes in the system
  modes := identifyModes(shared);
  names := List.map(BaseHashTable.hashTableKeyList(modes), ComponentReference.crefLastIdent);
  print("SMF-stateMachineElab States: " + stringDelimitList(names, ",")  + "\n");
  print("SMF-stateMachineElab ModeTable:\n");
  BaseHashTable.dumpHashTable(modes);
  nModes := BaseHashTable.hashTableCurrentSize(modes);

  print("SMF-stateMachineElab: Incidence matrix:\n");
  iTable := createIncidenceTable(modes, nModes);
  printIncidenceTable(iTable, nModes);

  print("SMF-stateMachineElab: Transitive closure:\n");
  transClosure := transitiveClosure(iTable, nModes);
  printIncidenceTable(transClosure, nModes);

  print("SMF-stateMachineElab: Initial States:\n");
  initialStates := extractInitialStates(modes);
  print( stringDelimitList(List.map(initialStates, ComponentReference.printComponentRefStr), ", ") + "\n");

  print("SMF-stateMachineElab: Flat Automata:\n");
  flatAutomata := extractFlatAutomata(initialStates, transClosure, nModes);
  printFlatAutomata(flatAutomata);

  print("SMF-stateMachineElab: Composition:\n");
  //compositions := getComposition(flatAutomata, {});
  compositions := getComposition(flatAutomata);
  ss := List.map(compositions, dumpCompositionStr);
  print(stringDelimitList(ss, ",\n") + "\n");

  print("SMF-stateMachineElab: annotate modes with additional information\n");
  (modes, systNew, sharedNew) := annotateModes(modes, syst, shared);
  BaseHashTable.dumpHashTable(modes);

  outDAE := BackendDAE.DAE({syst}, shared); // dummy assignment as long as elabortion is not implemented

end stateMachineElab;

protected function annotateModes "
Author: BTH
Collect mode relevant information, e.g., equations declared in that mode.
Tbd."
  input ModeTable modesIn;
  input BackendDAE.EqSystem inSyst;
  input BackendDAE.Shared inShared;
  output ModeTable modesOut;
  output BackendDAE.EqSystem outSyst;
  output BackendDAE.Shared outShared;
protected
  BackendDAE.EquationArray orderedEqsNew, removedEqsNew;
  // Fields EQSYSTEM:
  BackendDAE.Variables orderedVars;
  BackendDAE.EquationArray orderedEqs;
  Option<BackendDAE.IncidenceMatrix> m;
  Option<BackendDAE.IncidenceMatrixT> mT;
  BackendDAE.Matching matching;
  BackendDAE.StateSets stateSets;
  BackendDAE.BaseClockPartitionKind partitionKind;
  // Fields SHARED:
  BackendDAE.Variables knownVars;
  BackendDAE.Variables externalObjects;
  BackendDAE.Variables aliasVars;
  BackendDAE.EquationArray initialEqs;
  BackendDAE.EquationArray removedEqs;
  list<DAE.Constraint> constraints;
  list<DAE.ClassAttributes> classAttrs;
  FCore.Cache cache;
  FCore.Graph graph;
  DAE.FunctionTree functionTree;
  BackendDAE.EventInfo eventInfo;
  BackendDAE.ExternalObjectClasses extObjClasses;
  BackendDAE.BackendDAEType backendDAEType;
  BackendDAE.SymbolicJacobians symjacs;
  BackendDAE.ExtraInfo info;
  // Fields of EQUATION_ARRAY
  Integer orderedSize, removedSize;
  Integer orderedNumberOfElement, removedNumberOfElement;
  Integer orderedArrSize, removedArrSize;
  array<Option<BackendDAE.Equation>> orderedEquOptArr, removedEquOptArr;
algorithm
  BackendDAE.EQSYSTEM(orderedVars, orderedEqs, m, mT, matching, stateSets, partitionKind) := inSyst;
  BackendDAE.SHARED(knownVars, externalObjects, aliasVars, initialEqs, removedEqs, constraints,
    classAttrs, cache, graph, functionTree, eventInfo, extObjClasses,backendDAEType, symjacs, info) := inShared;

  BackendDAE.EQUATION_ARRAY(orderedSize, orderedNumberOfElement, orderedArrSize, orderedEquOptArr) := orderedEqs;
  BackendDAE.EQUATION_ARRAY(removedSize, removedNumberOfElement, removedArrSize, removedEquOptArr) := removedEqs;
  (orderedEquOptArr, modesOut) := Array.mapNoCopy_1(orderedEquOptArr, annotateMode, modesIn);
  (removedEquOptArr, modesOut) := Array.mapNoCopy_1(removedEquOptArr, annotateMode, modesOut);

  orderedEqsNew := BackendDAE.EQUATION_ARRAY(orderedSize, orderedNumberOfElement, orderedArrSize, orderedEquOptArr);
  removedEqsNew := BackendDAE.EQUATION_ARRAY(removedSize, removedNumberOfElement, removedArrSize, removedEquOptArr);

  //(orderedEqsNew, modesOut) := BackendEquation.traverseBackendDAEEqnsWithUpdate(orderedEqs, annotateMode, modesIn);
  //(removedEqsNew, modesOut) := BackendEquation.traverseBackendDAEEqnsWithUpdate(removedEqs, annotateMode, modesOut);

  // A lot of code bloat just for updating fields "orderedEqs" and "removedEqs" in "EQSYSTEM" and "SHARED" ...
  outSyst := BackendDAE.EQSYSTEM(orderedVars, orderedEqsNew, m, mT, matching, stateSets, partitionKind);
  outShared := BackendDAE.SHARED(knownVars, externalObjects, aliasVars, initialEqs, removedEqsNew, constraints,
    classAttrs, cache, graph, functionTree, eventInfo, extObjClasses,backendDAEType, symjacs, info);

  // TODO Could use new features https://trac.openmodelica.org/OpenModelica/changeset/23138 for updating...
  // What syntax would be even nicer to fight the code bloat?
  //outShared := inShared(removedEqs=removedEqsNew);
  //outShared := BackendDAE.SHARED(removedEqs=removedEqsNew) <- inShared
  //outShared := inShared -> BackendDAE.SHARED(removedEqs=removedEqsNew)
  //outShared := inShared /. BackendDAE.SHARED(removedEqs=removedEqsNew)
end annotateModes;

function annotateMode "
Author: BTH
Helper function to annotateModes
Tbd"
  input tuple<Option<BackendDAE.Equation>, ModeTable> inEqModeTable;
  output tuple<Option<BackendDAE.Equation>, ModeTable> outEqModeTable;
protected
  Option<BackendDAE.Equation> eqOpt;
  ModeTable modeTable;
  Option<DAE.ElementSource> sourceOpt "origin of equation";
  Option<DAE.ComponentRef> instanceOpt "the instance(s) this element is part of";
  Option<Mode> optMode;
algorithm
  (eqOpt, modeTable) := inEqModeTable;
  // Just match any possible Equation type and extract the source field.
  // TODO More specific handling with error handling (e.g., WHEN_EQUATION not allowed in a state)
  sourceOpt := match (eqOpt)
    local
      DAE.ElementSource source1;
    case SOME((BackendDAE.EQUATION(source=source1))) then SOME(source1);
    case SOME((BackendDAE.ARRAY_EQUATION(source=source1))) then SOME(source1);
    case SOME((BackendDAE.SOLVED_EQUATION(source=source1))) then SOME(source1);
    case SOME((BackendDAE.RESIDUAL_EQUATION(source=source1))) then SOME(source1);
    case SOME((BackendDAE.ALGORITHM(source=source1))) then SOME(source1);
    case SOME((BackendDAE.WHEN_EQUATION(source=source1))) then SOME(source1);
    case SOME((BackendDAE.COMPLEX_EQUATION(source=source1))) then SOME(source1);
    case SOME((BackendDAE.IF_EQUATION(source=source1))) then SOME(source1);
    else NONE();
  end match;

  instanceOpt := match (sourceOpt)
    local
      Option<DAE.ComponentRef> crefOpt;
    case SOME(DAE.SOURCE(instanceOpt=crefOpt)) then crefOpt;
    else NONE();
  end match;

  // Is there a mode/state that corresponds to the instance?
  optMode := match (instanceOpt)
    local
      DAE.ComponentRef cref;
    case SOME(cref) guard BaseHashTable.hasKey(cref, modeTable)
      then SOME(BaseHashTable.get(cref, modeTable));
    else NONE();
  end match;

  outEqModeTable := match (optMode, instanceOpt, eqOpt)
    local
      DAE.ComponentRef cref;
      BackendDAE.Equation eq;
      ModeTable modeTableNew;
      String name;
      Boolean isInitial;
      HashSet.HashSet edges;
      BackendDAE.EquationArray eqs;
    case (SOME(MODE(name,isInitial,edges,eqs)), SOME(cref), SOME(eq))
      equation
        eqs = BackendEquation.addEquation(eq, eqs);
        modeTableNew = BaseHashTable.update((cref, MODE(name,isInitial,edges,eqs)), modeTable);
      then (NONE(), modeTableNew);
    else (eqOpt, modeTable);
  end match;

end annotateMode;



protected function dumpCompositionStr "
Author: BTH
"
  input Composition composition;
  output String str;
algorithm
  str := match (composition)
    local
      DAE.ComponentRef cref;
      list<Composition> cs;
      Composition c;
      String s;
      list<String> ss;
    case COMPOSITION(refined=cref, refining={})
      then ComponentReference.printComponentRefStr(cref);
    case COMPOSITION(refined=cref, refining=cs)
      equation
        s = ComponentReference.printComponentRefStr(cref) + "(";
        ss = List.map(cs, dumpCompositionStr);
      then s + stringDelimitList(ss, ", ") + ")";
    end match;
end dumpCompositionStr;

protected function getComposition "
Author: BTH
Infer the hierachical structure from the input list of flat automata."
  input list<FlatAutomaton> flatAutomata;
  output list<Composition> compositions;
protected
  list<Composition> cs;
  Composition c;
  FlatAutomaton fa;
  DAE.ComponentRef cref;
  list<Boolean> refineds;
  Boolean refined;
algorithm
  cs := {};
  for fa in flatAutomata loop
   FLAT_AUTOMATON(initialState=cref) := fa;
   c := COMPOSITION(cref, {});
   (cs, c) := collectSubmodes(cs, c);
   refined := false;
   (cs, refineds) := List.map1_2(cs, refineSupermode, c);
   refined := List.exist(refineds, function boolEq(b2=true));
   // If no refinement of existing mode was possible, add the mode as (tentatative) root mode
   cs := if (refined) then cs else c::cs;
  end for;
compositions := cs;
end getComposition;

protected function refineSupermode "
Author: BTH
Recursively check whether cSub is a refinement of cSuper or of one of the sub-modes of cSuper.
If so, add cSub as a refinement to the known refinements of cSuper and return the refined cSuper
(cSuper -> cOut). The second output 'wasRefined' indicates if a refinement was found."
  input Composition cSuper;
  input Composition cSub;
  output Composition cOut;
  output Boolean wasRefined;

algorithm
  (cOut, wasRefined) := match (cSuper, cSub)
    local
      Composition c, crefined;
      DAE.ComponentRef refined;
      list<Composition> refining, refiningNew;
      list<Boolean> refineds;
      Boolean isRefined;
    case (COMPOSITION(refined=refined, refining=refining), c) guard isSubMode(cSub, cSuper)
      equation
        crefined = COMPOSITION(refined, c::refining);
      then (crefined, true);
    case (COMPOSITION(refining={}), _)
      then (cSuper, false);
    case (COMPOSITION(refined=refined, refining=refining), c)
      equation
        (refiningNew, refineds) = List.map1_2(refining, refineSupermode, c);
        isRefined = List.exist(refineds, function boolEq(b2=true));
        crefined = COMPOSITION(refined, refiningNew);
      then (crefined, isRefined);
    end match;
end refineSupermode;


protected function collectSubmodes "
Author: BTH
Find sub-modes of cIn in list csIn. If a sub-mode is found, it is added to the
known sub-modes of cIn (cIn->cOut) and deleted from the list csIn (csIn->csOut)."
  input list<Composition> csIn;
  input Composition cIn;
  output list<Composition> csOut;
  output Composition cOut;
protected
  DAE.ComponentRef crefIn,cref2;
  Composition c, cInUpdated;
  list<Composition> refiningIn;
algorithm
  csOut := {};
  cInUpdated := cIn;
  for c in csIn loop
    COMPOSITION(refined=crefIn, refining=refiningIn) := cInUpdated;
    if isSubMode(c, cIn) then
      cInUpdated := COMPOSITION(crefIn, c::refiningIn);
    else
      csOut := c::csOut;
    end if;
  end for;
  cOut := cInUpdated;
end collectSubmodes;



protected function isSubMode "
Author: BTH
Check whether first argument is a sub-mode of the second argument.
"
  input Composition sub;
  input Composition super;
  output Boolean isSub;
protected
  DAE.ComponentRef cref, subCref, subCrefStripped;
algorithm
  COMPOSITION(refined=subCref) := sub;
  subCrefStripped := ComponentReference.crefStripLastIdent(subCref);
  // print("SMF-isSuperModeOf: Unstripped: " + ComponentReference.printComponentRefStr(subCref) + "\n");
  // print("SMF-isSuperModeOf: Stripped: " + ComponentReference.printComponentRefStr(subCrefStripped) + "\n");
  COMPOSITION(refined=cref) := super;
  isSub := ComponentReference.crefEqual(cref, subCrefStripped);
end isSubMode;


protected function printFlatAutomata "
Author: BTH
Print flat automata (for debugging purposes)"
  input list<FlatAutomaton> flatAutomata;
protected
  DAE.ComponentRef initialState;
  HashSet.HashSet states;
  FlatAutomaton flatA;
  String s;
  list<DAE.ComponentRef> crefs;
  list<String> ss;
algorithm
  for flatA in flatAutomata loop
    FLAT_AUTOMATON(initialState=initialState, states=states) := flatA;
    s := ComponentReference.printComponentRefStr(initialState);
    print("Initial State \""+s+"\" cluster consists of: ");
    crefs := BaseHashSet.hashSetList(states);
    ss := List.map(crefs, ComponentReference.printComponentRefStr);
    print(stringDelimitList(ss, ", ") + "\n");
  end for;
end printFlatAutomata;

protected function extractFlatAutomata "
Author: BTH
For each initial state extract the (flat) automaton that is defined by the
transitive closure associated with that initial state."
  input list<DAE.ComponentRef> initialStates;
  input IncidenceTable iTable;
  input Integer nModes "Number of modes";
  output list<FlatAutomaton> flatAutomata;
protected
  HashTable.HashTable cref2index;
  Boolean incidence[nModes,nModes];
  list<tuple<DAE.ComponentRef, Integer>> entries;
  array<DAE.ComponentRef> i2cref;
  DAE.ComponentRef cref;
  list<DAE.ComponentRef> members;
  HashSet.HashSet memberSet;
  Integer n,i,j;
algorithm
  INCIDENCE_TABLE(cref2index, incidence) := iTable;
  n := BaseHashTable.hashTableCurrentSize(cref2index);
  // sanity check:
  assert(n == nModes, "Value of nModes needs to be equal to number of modes within mode table argument.");

  entries := BaseHashTable.hashTableList(cref2index);
  entries := List.sort(entries, crefIndexCmp);
  //i2cref := arrayCreate(n, ComponentReference.makeDummyCref());
  i2cref := listArray(List.map(entries, Util.tuple21));

  flatAutomata := {};
  for cref in initialStates loop
    i := BaseHashTable.get(cref, cref2index);
    members := {};
    for j in 1:n loop
      if incidence[i,j] then
        members := i2cref[j]::members;
      end if;
    end for;
    memberSet := HashSet.emptyHashSetSized(listLength(members));
    memberSet := List.fold(members, BaseHashSet.add, memberSet);
    flatAutomata := FLAT_AUTOMATON(cref, memberSet)::flatAutomata;
  end for;

end extractFlatAutomata;


protected function extractInitialStates "
Author: BTH
Return crefs of states declared as 'initialState' in modes. "
  input ModeTable modes;
  output list<DAE.ComponentRef> initialStates;
protected
  list<tuple<DAE.ComponentRef, Mode>> entries;
  tuple<DAE.ComponentRef, Mode> e;
  DAE.ComponentRef cref;
  Mode mode;
  Boolean isInitial;
algorithm
  entries := BaseHashTable.hashTableList(modes);
  initialStates := {};
  for e in entries loop
    (cref, mode) := e;
    MODE(isInitial=isInitial) := mode;
    if isInitial then
      initialStates := cref::initialStates;
    end if;
  end for;
end extractInitialStates;


protected function transitiveClosure "
Author: BTH
Compute the transitive closure over the transition relation between states/modes.
This allows to cluster modes/states that are part of the same (flat) automaton.
The function uses the Warshall's algorithm for that task, c.f.
http://en.wikipedia.org/wiki/Floyd%E2%80%93Warshall_algorithm
or the more succinct (and potentially more readable) description
http://de.wikipedia.org/wiki/Warshall-Algorithmus
"
  input IncidenceTable iTable;
  input Integer nModes "Number of modes";
  output IncidenceTable  transClosure;
protected
  HashTable.HashTable cref2index;
  Boolean incidence[nModes,nModes];
  Integer n,k,i,j;
  Boolean c;
algorithm
  INCIDENCE_TABLE(cref2index, incidence) := iTable;
  n := BaseHashTable.hashTableCurrentSize(cref2index);
  // sanity check:
  assert(n == nModes, "Value of nModes needs to be equal to number of modes within mode table argument.");

  // Warshall's algorithm for computing the transitive closure
  for k in 1:n loop
    for i in 1:n loop
      if incidence[i,k] then
        for j in 1:n loop
          if incidence[k,j] then
            incidence[i,j] := true;
          end if;
        end for;
      end if;
    end for;
  end for;

  transClosure := INCIDENCE_TABLE(cref2index, incidence);
end transitiveClosure;



protected function printIncidenceTable "
Author: BTH
Print incidence table."
  input IncidenceTable iTable;
  input Integer nModes "Number of modes";
protected
  HashTable.HashTable cref2index;
  Boolean incidence[nModes,nModes];
  list<tuple<DAE.ComponentRef, Integer>> entries;
  tuple<DAE.ComponentRef, Integer> entry;
  DAE.ComponentRef cref;
  Integer n,i,j,padn;
  array<Boolean> row;
  String str,pads;
  Boolean b;
algorithm
  INCIDENCE_TABLE(cref2index, incidence) := iTable;
  entries := BaseHashTable.hashTableList(cref2index);

  // sanity check:
  n := listLength(entries);
  assert(n == nModes, "Value of nModes needs to be equal to number of modes within mode table argument.");

  entries := List.sort(entries, crefIndexCmp);
  for entry in entries loop
    (cref, i) := entry;
    print( ComponentReference.printComponentRefStr(cref) + ": " + intString(i) + "\n" );
  end for;

  pads := " ";
  padn := 8;
  // Print table header
  str := Util.stringPadRight("i", padn, pads);
  for i in 1:n loop
    str := str + Util.stringPadLeft(intString(i)+",", padn, pads);
  end for;
  print(str + "\n");
  // print incidence matrix rows
  for i in 1:n loop
    str := Util.stringPadRight(intString(i), padn, pads);
    for j in 1:n loop
      b := incidence[i,j];
      str := str + Util.stringPadLeft(boolString(b)+",", padn, pads);
    end for;
    print(str + "\n");
  end for;
end printIncidenceTable;

protected function crefIndexCmp "
Author: BTH
Compare the indices assigned to two crefs (helper function for sorting)"
  input tuple<DAE.ComponentRef, Integer> inElement1;
  input tuple<DAE.ComponentRef, Integer> inElement2;
  output Boolean inRes;
protected
  Integer i1, i2;
algorithm
  (_, i1) := inElement1;
  (_, i2) := inElement2;
  inRes := i1 > i2;
end crefIndexCmp;



protected function createIncidenceTable "
Author: BTH
Create incidence table showing which modes are connected by transitions."
  input ModeTable modes;
  input Integer nModes "Number of modes";
  output IncidenceTable iTable;
protected
  HashTable.HashTable cref2index "Map cref to corresponding index in incidence matrix";
  Boolean incidence[nModes,nModes] "Incidence matrix showing which modes are connected by transitions";
  array<Boolean> iRow;
  Integer n,m,i,j,k;
  DAE.ComponentRef cref;
  HashSet.HashSet edges;
  array<DAE.ComponentRef> crefs1,crefs2;
algorithm
  crefs1 := listArray(BaseHashTable.hashTableKeyList(modes));
  n := arrayLength(crefs1);
  cref2index := HashTable.emptyHashTableSized(n);
  assert(n == nModes, "Value of nModes needs to be equal to number of modes within mode table argument.");
  incidence := fill(false,n,n);

  for i in 1:n loop
    cref2index := BaseHashTable.addNoUpdCheck((crefs1[i], i), cref2index);
  end for;

  for i in 1:n loop
    MODE(edges=edges) := BaseHashTable.get(crefs1[i], modes);
    crefs2 := listArray(BaseHashSet.hashSetList(edges));
    m := arrayLength(crefs2);
    for j in 1:m loop
      cref := crefs2[j];
      k := BaseHashTable.get(cref, cref2index);
      incidence[i,k] := true;
    end for;
  end for;

  iTable := INCIDENCE_TABLE(cref2index, incidence);
end createIncidenceTable;



protected function identifyModes "
Author: BTH
Traverse the equations, search for 'transition' and 'initialState' operators,
extract the state arguments from them and collect them in the table.
"
  input BackendDAE.Shared inShared;
  output ModeTable modes;
protected
  BackendDAE.EquationArray removedEqs;
algorithm
  BackendDAE.SHARED(removedEqs=removedEqs) := inShared;
  modes := BackendEquation.traverseBackendDAEEqns(removedEqs, extractStates, HashTableSM.emptyHashTable());
end identifyModes;


protected function extractStates "
Author: BTH
Helper function to identifyModes"
  input BackendDAE.Equation inEq;
  input ModeTable inA;
  output BackendDAE.Equation outEq;
  output ModeTable outA;
algorithm
  (outEq, outA) := match (inEq, inA)
    local
      String name;
      list<DAE.Exp> expLst;
      ModeTable modes;
    case (
        BackendDAE.ALGORITHM(alg = DAE.ALGORITHM_STMTS(
          statementLst = {
            DAE.STMT_NORETCALL(
              exp = DAE.CALL(
                path = Absyn.IDENT(name = name),
                expLst = expLst
              )
            )
          }
        )),
      _)
      equation
        //print("SMF-extractStates: "+ BackendDump.dumpEqnsStr({inEq}) +"\n");
        modes = extractState(name, expLst, inA);
      then
        (inEq, modes);
    else
      equation
        //print("SMF-extractStates: NO MATCH\n");
      then
        (inEq, HashTableSM.emptyHashTable());
  end match;
end extractStates;


protected function extractState "
Author: BTH
Helper function to extractStates"
  input String name;
  input list<DAE.Exp> expLst;
  input ModeTable inA;
  output ModeTable outA;
algorithm
  outA := match (name, expLst)
    local
      DAE.ComponentRef cstate1, cstate2;
      Integer tmp;
      Mode mode1, mode2;
      ModeTable modes;
      String name1,name2;
      Boolean isInitial1,isInitial2;
      HashSet.HashSet edges1,edges2;
      BackendDAE.EquationArray eqs1,eqs2;
      //array<Option<BackendDAE.Equation>> equOptArr;
    case ("initialState", {DAE.CREF(componentRef=cstate1)})
      equation
        //print("SMF-printEq2: "+anyString(cstate1)+"\n");
        mode1 = if BaseHashTable.hasKey(cstate1, inA)
          then BaseHashTable.get(cstate1, inA)
            else MODE(ComponentReference.crefLastIdent(cstate1), true, HashSet.emptyHashSet(),
                      BackendDAE.EQUATION_ARRAY(0,0,0,arrayCreate(0,NONE())));
        MODE(name1,isInitial1,edges1,eqs1) = mode1;
        mode1 = MODE(name1,true,edges1,eqs1);
        modes = BaseHashTable.add((cstate1, mode1), inA);
      then modes;
    case ("transition", DAE.CREF(componentRef=cstate1)::DAE.CREF(componentRef=cstate2)::_)
      equation
        //print("SMF-printEq2: "+anyString(cstate1)+"\n");
        tmp = ComponentReference.crefDepth(cstate1);
        //printArgs(expLst);
        mode1 = if BaseHashTable.hasKey(cstate1, inA)
          then BaseHashTable.get(cstate1, inA)
            else MODE(ComponentReference.crefLastIdent(cstate1), false, HashSet.emptyHashSet(),
                      BackendDAE.EQUATION_ARRAY(0,0,0,arrayCreate(0,NONE())));
        MODE(name1, isInitial1, edges1, eqs1) = mode1;
        isInitial1 = isInitial1 or false;
        edges1 = BaseHashSet.add(cstate2, edges1);
        mode1 = MODE(name1, isInitial1, edges1, eqs1);
        modes = BaseHashTable.add((cstate1, mode1), inA);

        mode2 = if BaseHashTable.hasKey(cstate2, modes)
          then BaseHashTable.get(cstate2, modes)
            else MODE(ComponentReference.crefLastIdent(cstate1), false, HashSet.emptyHashSet(),
                      BackendDAE.EQUATION_ARRAY(0,0,0,arrayCreate(0,NONE())));
        MODE(name2, isInitial2, edges2, eqs2) = mode2;
        isInitial2 = isInitial2 or false;
        edges2 = BaseHashSet.add(cstate1, edges2);
        mode2 = MODE(name2, isInitial2, edges2, eqs2);
        modes = BaseHashTable.add((cstate2, mode2), modes);
      then modes;
  end match;
end extractState;


protected function printArgs "BTH: DELETE me"
  input list<DAE.Exp> expLst;
  output Boolean dummy;
algorithm
  dummy := match (expLst)
    local
      DAE.Exp x;
      list<DAE.Exp> xs;
    case ({})
      then true;
    case (x::xs)
      equation
        print("SMF-printArgs: "+anyString(x)+"\n");
      then printArgs(xs);
  end match;
end printArgs;


/*
protected function addModeCref "
Autor: BTH
Sort of superfluous. Different from BaseHashTable.add(..) this function will not overwrite an existing entry.
"
  input DAE.ComponentRef stateCref;
  input ModeTable inTable;
  output ModeTable outTable;
protected
  Boolean exists;
  Mode controlMode;
algorithm
  exists := BaseHashTable.hasKey(stateCref, inTable);

  outTable := if (exists) then inTable
              else BaseHashTable.add((stateCref, MODE(ComponentReference.crefFirstIdent(stateCref), {})), inTable);

end addModeCref;
*/

/*
protected function getControllingModeCref "
Autor: BTH
Get the component reference of the mode-automaton that controls/embeds the
respective state.
Can fail if controlling mode doesn't exist in modeTable."
  input DAE.ComponentRef stateCref;
  input ModeTable modeTable;
  output Mode controlMode;
protected
  Integer depth;
  DAE.ComponentRef cref;
algorithm
  depth := ComponentReference.crefDepth(stateCref);
  cref := if (depth > 1) then ComponentReference.crefStripFirstIdent(stateCref)
          else CROOTMA;
  controlMode := BaseHashTable.get(cref, modeTable);
end getControllingModeCref;
*/



/*
protected function createModeTable "
Autor: BTH
Create a ModeTable with one root automaton."
  output ModeTable modeTable;
algorithm
  modeTable := HashTableSM.emptyHashTable();
  modeTable := BaseHashTable.add((CROOTMA, MODE(ROOTMA, {})), modeTable);
end createModeTable;
*/


/*
public function printEq
  input BackendDAE.Equation inEq;
  input list<String> inA;
  output BackendDAE.Equation outEq;
  output list<String> outA;
algorithm
  (outEq, outA) := match (inEq, inA)
    local
      String name;
      list<DAE.Exp> expLst;
    case (
        BackendDAE.ALGORITHM(alg = DAE.ALGORITHM_STMTS(
          statementLst = {
            DAE.STMT_NORETCALL(
              exp = DAE.CALL(
                path = Absyn.IDENT(name = name),
                expLst = expLst
              )
            )
          }
        )),
      _)
      equation
        print("BTH: "+ name +"\n");
        printEq2(name, expLst);
      then
        (inEq, {});
    else
      equation
        print("BTH: NO MATCH\n");
      then
        (inEq, {});
  end match;
end printEq;
*/

annotation(__OpenModelica_Interface="backend");
end StateMachineFeatures;
