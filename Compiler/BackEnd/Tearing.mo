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

encapsulated package Tearing
" file:        Tearing.mo
  package:     Tearing
  description: Tearing contains functions used for tear strong connected components.
               Implemented Methods are:
               - omc tearing developed by TU Dresden: Frenkel,Schubert
               - Cellier tearing
               - Carpanzano 2 tearing
               - Ollero-Amselem tearing
               - Steward tearing
               
  RCS: $Id: Tearing.mo 13560 2012-10-22 23:00:33Z jfrenkel $"

public import Absyn;
public import BackendDAE;
public import DAE;

protected import BackendDAEEXT;
protected import BackendDAEUtil;
protected import BackendDAETransform;
protected import BackendDump;
protected import BackendEquation;
protected import BackendVariable;
protected import BackendVarTransform;
protected import BaseHashSet;
protected import ClassInf;
protected import ComponentReference;
protected import Config;
protected import Debug;
protected import Derive;
protected import Expression;
protected import ExpressionSolve;
protected import ExpressionSimplify;
protected import Flags;
protected import HashSet;
protected import List;
protected import Matching;
protected import Util;

// =============================================================================
// type definitions 
//
//
// =============================================================================

partial function tearingMethodFunction
"interface for all tearing methods"
  input BackendDAE.EqSystem isyst;
  input BackendDAE.Shared ishared;
  input list<Integer> eindex;
  input list<Integer> vindx;
  input Option<list<tuple<Integer, Integer, BackendDAE.Equation>>> ojac;
  input BackendDAE.JacobianType jacType;
  output BackendDAE.StrongComponent ocomp;
  output Boolean outRunMatching;
end tearingMethodFunction;


// =============================================================================
// public 
//
// main function to divide to the selected tearing method 
//
// =============================================================================

public function tearingSystem "function tearingSystem
  author: Frenkel TUD 2012-05"
  input BackendDAE.BackendDAE inDAE;
  output BackendDAE.BackendDAE outDAE;
algorithm
  outDAE := matchcontinue(inDAE)
    local
        String methodString;
        tearingMethodFunction method;
      // if noTearing selected, do nothing.
      case(_)
        equation
          methodString = Config.getTearingMethod();
          true = stringEqual(methodString,"noTearing");
        then
          inDAE;
      // get method function and traveres systems
      case(_)
        equation
          method = getTearingMethod();
          (outDAE,_) = BackendDAEUtil.mapEqSystemAndFold(inDAE,tearingSystemWork,method);
        then
          outDAE;
  end matchcontinue;
end tearingSystem;

// =============================================================================
// protected 
//
//
// =============================================================================

protected function tearingSystemWork "function tearingSystemWork
  author: Frenkel TUD 2012-05"
  input BackendDAE.EqSystem isyst;
  input tuple<BackendDAE.Shared,tearingMethodFunction> sharedChanged;
  output BackendDAE.EqSystem osyst;
  output tuple<BackendDAE.Shared,tearingMethodFunction> osharedChanged;
protected  
  BackendDAE.StrongComponents comps;
  tearingMethodFunction method;
  Boolean b;
  BackendDAE.Shared shared;
  array<Integer> ass1,ass2;
algorithm
  BackendDAE.EQSYSTEM(matching=BackendDAE.MATCHING(ass1=ass1, ass2=ass2, comps=comps)):=isyst;
  (shared, method) := sharedChanged;
  (comps,b) := traverseComponents(comps,isyst,shared,method,{},false);
  osyst := Debug.bcallret2(b, BackendDAEUtil.setEqSystemMatching, isyst, BackendDAE.MATCHING(ass1, ass2, comps), isyst);
  osharedChanged := sharedChanged;
end tearingSystemWork;

protected function traverseComponents "function traverseComponents
  author: Frenkel TUD 2012-05"
  input BackendDAE.StrongComponents inComps;
  input BackendDAE.EqSystem isyst;
  input BackendDAE.Shared ishared;
  input tearingMethodFunction method;
  input BackendDAE.StrongComponents iAcc;
  input Boolean iRunMatching;
  output BackendDAE.StrongComponents oComps;
  output Boolean outRunMatching;
algorithm
  (oComps,outRunMatching):=
  matchcontinue (inComps,isyst,ishared,method,iAcc,iRunMatching)
    local
      list<Integer> eindex,vindx;
      list<list<Integer>> othercomps;
      Boolean b,b1;
      BackendDAE.StrongComponents comps,acc;
      BackendDAE.StrongComponent comp,comp1;   
      Option<list<tuple<Integer, Integer, BackendDAE.Equation>>> ojac;
      BackendDAE.JacobianType jacType;
    case ({},_,_,_,_,_)
      then (listReverse(iAcc),iRunMatching);
    // don't tear linear system as long as we do not handle them 
    // as linear system while the runtime
    case ((comp as BackendDAE.EQUATIONSYSTEM(eqns=eindex,vars=vindx,jac=ojac,jacType=jacType))::comps,_,_,_,_,_)
      equation
        equality(jacType = BackendDAE.JAC_TIME_VARYING());
        true = Flags.isSet(Flags.LINEAR_TEARING);
        (comp1,true) = method(isyst,ishared,eindex,vindx,ojac,jacType);
        (acc,b1) = traverseComponents(comps,isyst,ishared,method,comp1::iAcc,true);
      then
        (acc,b1);
    // tearing of non-linear systems
    case ((comp as BackendDAE.EQUATIONSYSTEM(eqns=eindex,vars=vindx,jac=ojac,jacType=jacType))::comps,_,_,_,_,_)
      equation
        failure(equality(jacType = BackendDAE.JAC_TIME_VARYING()));
        (comp1,true) = method(isyst,ishared,eindex,vindx,ojac,jacType);
        (acc,b1) = traverseComponents(comps,isyst,ishared,method,comp1::iAcc,true);
      then
        (acc,b1);        
    // only continues part of a mixed system
    case ((comp as BackendDAE.MIXEDEQUATIONSYSTEM(condSystem=comp1,disc_eqns=eindex,disc_vars=vindx))::comps,_,_,_,_,_)
      equation
        true = Flags.isSet(Flags.NO_MIXED_TEARING);
        (comp1::{},true) = traverseComponents({comp1},isyst,ishared,method,{},false);
        (acc,b1) = traverseComponents(comps,isyst,ishared,method,BackendDAE.MIXEDEQUATIONSYSTEM(comp1,eindex,vindx)::iAcc,true);
      then
        (acc,b1);
    // mixed and continues part
    case ((comp as BackendDAE.MIXEDEQUATIONSYSTEM(condSystem=comp1,disc_eqns=eindex,disc_vars=vindx))::comps,_,_,_,_,_)
      equation
        false = Flags.isSet(Flags.NO_MIXED_TEARING);
        (eindex,vindx) = BackendDAETransform.getEquationAndSolvedVarIndxes(comp);
        (comp1,true) = method(isyst,ishared,eindex,vindx,NONE(),BackendDAE.JAC_NO_ANALYTIC());
        (acc,b1) = traverseComponents(comps,isyst,ishared,method,comp1::iAcc,true);
      then
        (acc,b1);
    // no component for tearing
    case (comp::comps,_,_,_,_,_)
      equation
        (acc,b) = traverseComponents(comps,isyst,ishared,method,comp::iAcc,iRunMatching);
      then
        (acc,b);
  end matchcontinue;  
end traverseComponents;

protected function getTearingMethod
" function: getTearingMethod"
  output tearingMethodFunction methodFunction;
protected 
  list<tuple<tearingMethodFunction,String>> allMethods;
  String method;
algorithm
  allMethods := {(omcTearing,"omcTearing")
  };
  method := Config.getTearingMethod();
  methodFunction := selectTearingMethods(allMethods,method);  
end getTearingMethod;

protected function selectTearingMethods
  input list<tuple<tearingMethodFunction,String>> allMethods;
  input String method;
  output tearingMethodFunction methodFunction;
algorithm
  methodFunction:=
  matchcontinue (allMethods,method)
    local 
      tearingMethodFunction func;
      String name;
      list<tuple<tearingMethodFunction,String>> rest;
    case ((func,name)::rest,_)
      equation
        true = stringEqual(name,method);
      then   
        func;
    case ((func,name)::rest,_)
      equation
        false = stringEqual(name,method);
      then   
        selectTearingMethods(rest,method);
  end matchcontinue;
end selectTearingMethods;

// =============================================================================
// 
// method: omc tearing 
//
// =============================================================================

protected function omcTearing "function omcTearing
  author: Frenkel TUD 2012-05"
  input BackendDAE.EqSystem isyst;
  input BackendDAE.Shared ishared;
  input list<Integer> eindex;
  input list<Integer> vindx;
  input Option<list<tuple<Integer, Integer, BackendDAE.Equation>>> ojac;
  input BackendDAE.JacobianType jacType;
  output BackendDAE.StrongComponent ocomp;
  output Boolean outRunMatching;
protected
  list<Integer> tvars,residual,unsolvables;
  list<list<Integer>> othercomps;
  BackendDAE.EqSystem syst,subsyst;
  BackendDAE.Shared shared;   
  array<Integer> ass1,ass2,columark,number,lowlink;
  Integer size,tornsize,mark;
  list<BackendDAE.Equation> eqn_lst; 
  list<BackendDAE.Var> var_lst;    
  BackendDAE.Variables vars;
  BackendDAE.EquationArray eqns;
  BackendDAE.IncidenceMatrix m,m1;
  BackendDAE.IncidenceMatrix mt,mt1;      
  BackendDAE.AdjacencyMatrixEnhanced me;
  BackendDAE.AdjacencyMatrixTEnhanced meT;
  array<list<Integer>> mapEqnIncRow;
  array<Integer> mapIncRowEqn;
  DAE.FunctionTree funcs;
algorithm
  // generate Subsystem to get the incidence matrix
  size := listLength(vindx);
  eqn_lst := BackendEquation.getEqns(eindex,BackendEquation.daeEqns(isyst));  
  eqns := BackendEquation.listEquation(eqn_lst);      
  var_lst := List.map1r(vindx, BackendVariable.getVarAt, BackendVariable.daeVars(isyst));
  vars := BackendVariable.listVar1(var_lst);
  subsyst := BackendDAE.EQSYSTEM(vars,eqns,NONE(),NONE(),BackendDAE.NO_MATCHING(),{});
  funcs := BackendDAEUtil.getFunctions(ishared);
  (subsyst,m,mt,_,_) := BackendDAEUtil.getIncidenceMatrixScalar(subsyst, BackendDAE.NORMAL(), SOME(funcs));
  //  IndexReduction.dumpSystemGraphML(subsyst,ishared,NONE(),"System" +& intString(size) +& ".graphml");
  Debug.fcall(Flags.TEARING_DUMP, BackendDump.printEqSystem, subsyst);

  (me,meT,mapEqnIncRow,mapIncRowEqn) := BackendDAEUtil.getAdjacencyMatrixEnhancedScalar(subsyst,ishared);
  Debug.fcall(Flags.TEARING_DUMP, BackendDump.dumpAdjacencyMatrixEnhanced,me);
  Debug.fcall(Flags.TEARING_DUMP, BackendDump.dumpAdjacencyMatrixTEnhanced,meT);
  //  IndexReduction.dumpSystemGraphMLEnhanced(subsyst,shared,me,meT);
      
  /*   m1 := incidenceMatrixfromEnhanced(me);
       Matching.matchingExternalsetIncidenceMatrix(size,size,m1);
       BackendDAEEXT.matching(size,size,5,-1,1.0,1);
       ass1 := arrayCreate(size,-1);
       ass2 := arrayCreate(size,-1);
       BackendDAEEXT.getAssignment(ass1,ass2);         
       Debug.fcall(Flags.TEARING_DUMP, BackendDump.dumpMatching,ass1);
       Debug.fcall(Flags.TEARING_DUMP, BackendDump.dumpMatching,ass2);          
  */  
  // do cheap matching until a maximum matching is there if
  // cheap matching stucks select additional tearing variable and continue
  ass1 := arrayCreate(size,-1);
  ass2 := arrayCreate(size,-1);

  // get all unsolvable variables
  unsolvables := getUnsolvableVars(1,size,meT,{});
  Debug.fcall(Flags.TEARING_DUMP, print,"Unsolvable Vars:\n"); 
  Debug.fcall(Flags.TEARING_DUMP, BackendDump.debuglst,(unsolvables,intString,", ","\n"));
     
  columark := arrayCreate(size,-1);
  (tvars,mark) := omcTearing2(unsolvables,me,meT,mapEqnIncRow,mapIncRowEqn,size,vars,ishared,ass1,ass2,columark,1,{});
  ass1 := List.fold(tvars,unassignTVars,ass1);
  // unmatched equations are residual equations
  residual := Matching.getUnassigned(size,ass2,{});
  //  BackendDump.dumpMatching(ass1);
  Debug.fcall(Flags.TEARING_DUMP, print,"TearingVars:\n"); 
  Debug.fcall(Flags.TEARING_DUMP, BackendDump.debuglst,(tvars,intString,", ","\nResidualEquations:\n"));
  Debug.fcall(Flags.TEARING_DUMP, BackendDump.debuglst,(residual,intString,", ","\n")); 
  //  subsyst := BackendDAEUtil.setEqSystemMatching(subsyst,BackendDAE.MATCHING(ass1,ass2,{}));
  //  IndexReduction.dumpSystemGraphML(subsyst,ishared,NONE(),"TornSystem" +& intString(size) +& ".graphml");
  // check if tearing make sense
  tornsize := listLength(tvars);
  true := intLt(tornsize,size-1);
  // run tarjan to get order of other equations
  m1 := arrayCreate(size,{});
  mt1 := arrayCreate(size,{});
  m1 := getOtherEqSysIncidenceMatrix(m,size,1,ass2,ass1,m1);
  mt1 := getOtherEqSysIncidenceMatrix(mt,size,1,ass1,ass2,mt1);
  //  subsyst := BackendDAE.EQSYSTEM(vars,eqns,SOME(m1),SOME(mt1),BackendDAE.MATCHING(ass1,ass2,{}));
  //  BackendDump.printEqSystem(subsyst);
  number := arrayCreate(size,0);
  lowlink := arrayCreate(size,0);        
  number := setIntArray(residual,number,size);
  (_,_,othercomps) := BackendDAETransform.strongConnectMain(mt1, ass2, number, lowlink, size, 0, 1, {}, {});        
  Debug.fcall(Flags.TEARING_DUMP, print, "OtherEquationsOrder:\n"); 
  Debug.fcall(Flags.TEARING_DUMP, BackendDump.dumpComponentsOLD,othercomps); 
  Debug.fcall(Flags.TEARING_DUMP, print, "\n");
  // calculate influence of tearing vars in residual equations 
  mt1 := arrayCreate(size, {});
  mark := getDependenciesOfVars(othercomps, ass1, ass2, m, mt1, columark, mark);
  (residual, mark) := sortResidualDepentOnTVars(residual, tvars, ass1, m, mt1, columark, mark);
  (ocomp,outRunMatching) := omcTearing4(jacType,isyst,ishared,subsyst,tvars,residual,ass1,ass2,othercomps,eindex,vindx,mapEqnIncRow,mapIncRowEqn,columark,mark);
  Debug.fcall(Flags.TEARING_DUMP, print,Util.if_(outRunMatching,"Ok system torn\n","System not torn\n"));
end omcTearing;

protected function incidenceMatrixfromEnhanced
"function incidenceMatrixfromEnhanced
  author: Frenkel TUD 2012-08"
  input BackendDAE.AdjacencyMatrixEnhanced me;
  output BackendDAE.IncidenceMatrix m;
algorithm
  m := Util.arrayMap(me,incidenceMatrixElementfromEnhanced);
end incidenceMatrixfromEnhanced;

protected function incidenceMatrixElementfromEnhanced
"function incidenceMatrixElementfromEnhanced
  author: Frenkel TUD 2012-08"
  input BackendDAE.AdjacencyMatrixElementEnhanced iRow;
  output BackendDAE.IncidenceMatrixElement oRow;
algorithm
  oRow := List.map(List.sort(iRow,AdjacencyMatrixElementEnhancedCMP), incidenceMatrixElementElementfromEnhanced);
end incidenceMatrixElementfromEnhanced;

protected function AdjacencyMatrixElementEnhancedCMP
"function AdjacencyMatrixElementEnhancedCMP
  author: Frenkel TUD 2012-08"
  input tuple<Integer, BackendDAE.Solvability> inTplA;
  input tuple<Integer, BackendDAE.Solvability> inTplB;
  output Boolean b;
algorithm
  b := BackendDAEUtil.solvabilityCMP(Util.tuple22(inTplA),Util.tuple22(inTplB));
end AdjacencyMatrixElementEnhancedCMP;

protected function incidenceMatrixElementElementfromEnhanced
"function incidenceMatrixElementElementfromEnhanced
  author: Frenkel TUD 2012-08"
  input tuple<Integer, BackendDAE.Solvability> inTpl;
  output Integer oI;
algorithm
  oI := match(inTpl)
    local 
      Integer i;
      Boolean b;
    case ((i,BackendDAE.SOLVABILITY_SOLVED())) then i;
    case ((i,BackendDAE.SOLVABILITY_CONSTONE())) then i;
    case ((i,BackendDAE.SOLVABILITY_CONST())) then i;
    case ((i,BackendDAE.SOLVABILITY_PARAMETER(b=_)))
      equation
        i = Util.if_(intLt(i,0),i,-i);
      then i;
    case ((i,BackendDAE.SOLVABILITY_TIMEVARYING(b=_)))
      equation
        i = Util.if_(intLt(i,0),i,-i);
      then i;
    case ((i,BackendDAE.SOLVABILITY_NONLINEAR()))
      equation
        i = Util.if_(intLt(i,0),i,-i);
       then i;
    case ((i,BackendDAE.SOLVABILITY_UNSOLVABLE()))
      equation
        i = Util.if_(intLt(i,0),i,-i);
      then i;
  end match;
end incidenceMatrixElementElementfromEnhanced;

protected function getUnsolvableVars
"function getUnsolvableVars
  author: Frenkel TUD 2012-08"
  input Integer index;
  input Integer size;
  input BackendDAE.AdjacencyMatrixTEnhanced meT;
  input list<Integer> iAcc;
  output list<Integer> oAcc;
algorithm
  oAcc := matchcontinue(index,size,meT,iAcc)
    local
      BackendDAE.AdjacencyMatrixElementEnhanced elem;
      list<Integer> acc;
      Boolean b;
    case(_,_,_,_)
      equation
        true = intLe(index,size);
        elem = meT[index];
        b = unsolvable(elem);
        acc = List.consOnTrue(b, index, iAcc);
      then
       getUnsolvableVars(index+1,size,meT,acc);
    case(_,_,_,_)
      then
       iAcc;
  end matchcontinue;
end getUnsolvableVars;

protected function unsolvable
"function unsolvable
  author: Frenkel TUD 2012-08"
  input BackendDAE.AdjacencyMatrixElementEnhanced elem;
  output Boolean b;
algorithm
  b := match(elem)
    local
      Integer e;
      BackendDAE.AdjacencyMatrixElementEnhanced rest;
      Boolean b1;
    case ({}) then true;
    case ((e,BackendDAE.SOLVABILITY_SOLVED())::rest)
      equation
        b1 = intLe(e,0);
        b1 = Debug.bcallret1(b1, unsolvable, rest, false);
      then 
        b1;
    case ((e,BackendDAE.SOLVABILITY_CONSTONE())::rest)
      equation
        b1 = intLe(e,0);
        b1 = Debug.bcallret1(b1, unsolvable, rest, false);
      then 
        b1;
    case ((e,BackendDAE.SOLVABILITY_CONST())::rest)
      equation
        b1 = intLe(e,0);
        b1 = Debug.bcallret1(b1, unsolvable, rest, false);
      then 
        b1;
    case ((e,BackendDAE.SOLVABILITY_PARAMETER(b=false))::rest)
      then 
        unsolvable(rest);
    case ((e,BackendDAE.SOLVABILITY_PARAMETER(b=true))::rest)
      equation
        b1 = intLe(e,0);
        b1 = Debug.bcallret1(b1, unsolvable, rest, false);
      then 
        b1;
    case ((e,BackendDAE.SOLVABILITY_TIMEVARYING(b=false))::rest)
      then 
        unsolvable(rest);
    case ((e,BackendDAE.SOLVABILITY_TIMEVARYING(b=true))::rest)
      then 
        unsolvable(rest);
    case ((e,BackendDAE.SOLVABILITY_NONLINEAR())::rest)
      then 
        unsolvable(rest);
    case ((e,BackendDAE.SOLVABILITY_UNSOLVABLE())::rest)
      then 
        unsolvable(rest);
  end match;
end unsolvable;

protected function unassignTVars "function unassignTVars
  author: Frenkel TUD 2012-05"
  input Integer v;
  input array<Integer> inAss;
  output array<Integer> outAss;
algorithm
  outAss := arrayUpdate(inAss,v,-1);
end unassignTVars;

protected function isAssigned "function isAssigned
  author: Frenkel TUD 2012-05"
  input array<Integer> ass;
  input Integer i;
  output Boolean b;
algorithm
  b := intGt(ass[i],0);
end isAssigned;

protected function isUnAssigned "function isUnAssigned
  author: Frenkel TUD 2012-05"
  input array<Integer> ass;
  input Integer i;
  output Boolean b;
algorithm
  b := intLt(ass[i],1);
end isUnAssigned;

protected function isMarked "function isMarked
  author: Frenkel TUD 2012-05"
  input tuple<array<Integer>,Integer> inTpl;
  input Integer v;
  output Boolean b;
protected
  array<Integer> markarray;
  Integer mark;
algorithm
  (markarray,mark) := inTpl;
  b := intEq(markarray[v],mark);
end isMarked;

protected function getOtherEqSysIncidenceMatrix "function getOtherEqSysIncidenceMatrix
  author: Frenkel TUD 2012-05"
  input BackendDAE.IncidenceMatrix m;
  input Integer size;
  input Integer index;
  input array<Integer> skip;
  input array<Integer> rowskip;
  input BackendDAE.IncidenceMatrix mnew;
  output BackendDAE.IncidenceMatrix outMNew;
algorithm
  outMNew := matchcontinue(m,size,index,skip,rowskip,mnew)
    local
      list<Integer> row;
    case (_,_,_,_,_,_)
      equation
        true = intGt(index,size);
      then
        mnew;
    case (_,_,_,_,_,_)
      equation
        true = intGt(skip[index],0);
        row = List.select(m[index], Util.intPositive);
        row = List.select1r(row,isAssigned,rowskip);
        _ = arrayUpdate(mnew,index,row);
      then
        getOtherEqSysIncidenceMatrix(m,size,index+1,skip,rowskip,mnew);
    case (_,_,_,_,_,_)
      equation
        _ = arrayUpdate(mnew,index,{});
      then
        getOtherEqSysIncidenceMatrix(m,size,index+1,skip,rowskip,mnew);
  end matchcontinue;
end getOtherEqSysIncidenceMatrix;

protected function setIntArray
"function setIntArray
  author: Frenkel TUD 2012-08"
  input list<Integer> inLst;
  input array<Integer> arr;
  input Integer value;
  output array<Integer> oarr;
algorithm
  oarr := match(inLst,arr,value)
    local 
      Integer indx;
      list<Integer> rest;
    case(indx::rest,_,_)
      equation
        _= arrayUpdate(arr,indx,value);
      then
        setIntArray(rest,arr,value);
    case({},_,_) then arr;
  end match;
end setIntArray;

protected function getDependenciesOfVars
  input list<list<Integer>> iComps;
  input array<Integer> ass1;
  input array<Integer> ass2;
  input BackendDAE.IncidenceMatrix m;
  input array<list<Integer>> mT;
  input array<Integer> visited;
  input Integer iMark;
  output Integer oMark;
algorithm
  oMark := match(iComps, ass1, ass2, m, mT, visited, iMark)
    local
      Integer c, v;
      list<Integer> comp, tvars, vars;
      list<list<Integer>> comps;
    
    case ({}, _, _, _, _, _, _)
    then iMark;
    
    case ({c}::comps, _, _, _, _, _, _) equation
      // get var of eqn
      v = ass2[c];
      // get TVars of Eqn
      vars = List.select(m[c], Util.intPositive);
      tvars = tVarsofEqn(vars, ass1, mT, visited, iMark, {});
      // update map
      _ = arrayUpdate(mT, v, tvars);
    then getDependenciesOfVars(comps, ass1, ass2, m, mT, visited, iMark+1);
    
    case (comp::comps, _, _, _, _, _, _) equation
      // get var of eqns
      vars = List.map1r(comp,arrayGet,ass2);
      // get TVars of Eqns
      tvars = tVarsofEqns(comp, m, ass1, mT, visited, iMark, {});
      // update map
      _ = List.fold1r(vars, arrayUpdate, tvars, mT);
    then getDependenciesOfVars(comps, ass1, ass2, m, mT, visited, iMark+1);
  end match; 
end getDependenciesOfVars;

protected function tVarsofEqns
  input list<Integer> iEqns;
  input BackendDAE.IncidenceMatrix m;
  input array<Integer> ass1;
  input array<list<Integer>> mT;
  input array<Integer> visited;
  input Integer iMark;
  input list<Integer> iAcc;
  output list<Integer> oAcc;
algorithm
  oAcc := match(iEqns, m, ass1, mT, visited, iMark, iAcc)
    local
      Integer e;
      list<Integer> eqns, vars, tvars;
      
    case ({}, _, _, _, _, _, _)
    then iAcc;
    
    case (e::eqns, _, _, _, _, _, _) equation
      vars = List.select(m[e], Util.intPositive);
      tvars = tVarsofEqn(vars, ass1, mT, visited, iMark, iAcc);
    then tVarsofEqns(eqns, m, ass1, mT, visited, iMark, tvars);
  end match;
end tVarsofEqns;

protected function tVarsofEqn
  input list<Integer> iVars;
  input array<Integer> ass1;
  input array<list<Integer>> mT;
  input array<Integer> visited;
  input Integer iMark;
  input list<Integer> iAcc;
  output list<Integer> oAcc;
algorithm
  oAcc := matchcontinue(iVars,ass1,mT,visited,iMark,iAcc)
    local
      Integer v;
      list<Integer> vars,tvars;
    case ({},_,_,_,_,_) then iAcc;
    case (v::vars,_,_,_,_,_)
      equation
        true = intLt(ass1[v],0);
        tvars = uniqueIntLst(v,iMark,visited,iAcc);
      then
        tVarsofEqn(vars,ass1,mT,visited,iMark,tvars);
    case (v::vars,_,_,_,_,_) equation
      tvars = List.fold2(mT[v],uniqueIntLst,iMark,visited,iAcc);
    then tVarsofEqn(vars, ass1, mT, visited, iMark, tvars);
  end matchcontinue;
end tVarsofEqn;

protected function uniqueIntLst
  input Integer c;
  input Integer mark;
  input array<Integer> markarray;
  input list<Integer> iAcc;
  output list<Integer> oAcc;
algorithm
  oAcc := matchcontinue(c,mark,markarray,iAcc)
    case(_,_,_,_)
      equation
        false = intEq(mark,markarray[c]);
        _ = arrayUpdate(markarray,c,mark);
      then
        c::iAcc;
    else
      then
        iAcc;
  end matchcontinue;
end uniqueIntLst;

protected function sortResidualDepentOnTVars
  input list<Integer> iResiduals;
  input list<Integer> iTVars;
  input array<Integer> ass1;
  input BackendDAE.IncidenceMatrix m;
  input array<list<Integer>> mT;
  input array<Integer> visited;
  input Integer iMark;
  output list<Integer> oResiduals;
  output Integer oMark;
protected
  Integer size;
  list<list<Integer>> maplst;
  array<list<Integer>> map;
  array<Integer> eqnLocalGlobal,varGlobalLocal,v1,v2;
algorithm
  // eqn - local - Global indices
  eqnLocalGlobal := listArray(iResiduals);
  // var - global local indices
  varGlobalLocal := arrayCreate(arrayLength(m),-1);
  varGlobalLocal := getGlobalLocal(iTVars,1,varGlobalLocal);
  // generate list of map[residual]=tvars
  // change indices in map to local
  (oMark,maplst) := tVarsofResidualEqns(iResiduals,m,ass1,mT,varGlobalLocal,visited,iMark,{});
  map := listArray(maplst);
  // get for each residual a tvar
  size := arrayLength(map);
  Matching.matchingExternalsetIncidenceMatrix(size,size,map);
  BackendDAEEXT.matching(size,size,5,-1,1.0,1);
  v1 := arrayCreate(size,-1);
  v2 := arrayCreate(size,-1);
  BackendDAEEXT.getAssignment(v2,v1);
  //  BackendDump.dumpIncidenceMatrix(map);
  //  BackendDump.dumpMatching(v1);
  //  BackendDump.dumpMatching(v2);
  // sort residuals depent on matching to tvars
  oResiduals := getTVarResiduals(size,v1,eqnLocalGlobal,{});
  //  print("iResiduals " +& stringDelimitList(List.map(iResiduals,intString),"\n") +& "\n");
  //  print("oResiduals " +& stringDelimitList(List.map(oResiduals,intString),"\n") +& "\n");
end sortResidualDepentOnTVars;

protected function getGlobalLocal
  input list<Integer> iTVars;
  input Integer index;
  input array<Integer> iVarGlobalLocal;
  output array<Integer> oVarGlobalLocal;
algorithm
oVarGlobalLocal := 
  match(iTVars,index,iVarGlobalLocal)
    local
      Integer i;
      list<Integer> tvars;
    case ({},_,_) then iVarGlobalLocal;
    case (i::tvars,_,_)
      equation
        _= arrayUpdate(iVarGlobalLocal,i,index);
      then
        getGlobalLocal(tvars,index+1,iVarGlobalLocal);
  end match;
end getGlobalLocal;

protected function tVarsofResidualEqns
  input list<Integer> iEqns;
  input BackendDAE.IncidenceMatrix m;
  input array<Integer> ass1;
  input array<list<Integer>> mT;
  input array<Integer> varGlobalLocal;
  input array<Integer> visited;
  input Integer iMark;
  input list<list<Integer>> iAcc;
  output Integer oMark;
  output list<list<Integer>> oAcc;
algorithm
  (oMark,oAcc) := match(iEqns,m,ass1,mT,varGlobalLocal,visited,iMark,iAcc)
    local
      Integer e;
      list<Integer> eqns,vars,tvars;
    case ({},_,_,_,_,_,_,_) then (iMark,listReverse(iAcc));
    case (e::eqns,_,_,_,_,_,_,_)
      equation
        vars = List.select(m[e], Util.intPositive);
        tvars = tVarsofEqn(vars,ass1,mT,visited,iMark,{});
        // change indices to local
        tvars = List.map1r(tvars,arrayGet,varGlobalLocal);
        (oMark,oAcc) = tVarsofResidualEqns(eqns,m,ass1,mT,varGlobalLocal,visited,iMark+1,tvars::iAcc);
      then
        (oMark,oAcc);
  end match;
end tVarsofResidualEqns;

protected function getTVarResiduals
  input Integer index;
  input array<Integer> v1;
  input array<Integer> eqnLocalGlobal;
  input list<Integer> iAcc;
  output list<Integer> oAcc;
algorithm
  oAcc := match(index,v1,eqnLocalGlobal,iAcc)
    local
      Integer e;
    case (0,_,_,_) then iAcc;
    case (_,_,_,_)
      equation
        e = v1[index];
        e = eqnLocalGlobal[e];
      then
        getTVarResiduals(index-1,v1,eqnLocalGlobal,e::iAcc);
  end match;
end getTVarResiduals;

protected function omcTearing2 "function omcTearing2
  author: Frenkel TUD 2012-05"
  input list<Integer> unsolvables;
  input BackendDAE.AdjacencyMatrixEnhanced m;
  input BackendDAE.AdjacencyMatrixTEnhanced mt;
  input array<list<Integer>> mapEqnIncRow;
  input array<Integer> mapIncRowEqn;    
  input Integer size;
  input BackendDAE.Variables vars;
  input BackendDAE.Shared ishared;
  input array<Integer> ass1; 
  input array<Integer> ass2;
  input array<Integer> columark;
  input Integer mark;
  input list<Integer> inTVars;
  output list<Integer> outTVars;
  output Integer oMark;
algorithm
  (outTVars,oMark) := matchcontinue(unsolvables,m,mt,mapEqnIncRow,mapIncRowEqn,size,vars,ishared,ass1,ass2,columark,mark,inTVars)
    local 
      Integer tvar;
      list<Integer> unassigned,rest;
      BackendDAE.AdjacencyMatrixElementEnhanced vareqns;
    case ({},_,_,_,_,_,_,_,_,_,_,_,_)
      equation
        // select tearing var
        tvar = omcTearingSelectTearingVar(vars,ass1,ass2,m,mt);
        //  print("Selected Var " +& intString(tvar) +& " as TearingVar\n");
        // mark tearing var
        _ = arrayUpdate(ass1,tvar,size*2);
        vareqns = List.removeOnTrue(ass2, isAssignedSaveEnhanced, mt[tvar]); 
        //vareqns = List.removeOnTrue((columark,mark), isMarked, vareqns); 
        //markEqns(vareqns,columark,mark);
        // cheap matching
        tearingBFS(vareqns,m,mt,mapEqnIncRow,mapIncRowEqn,size,ass1,ass2,columark,mark,{});

        /*  vlst = getUnnassignedFromArray(1,arrayLength(mt),ass1,vars,BackendVariable.getVarAt,0,{});
          elst = getUnnassignedFromArray(1,arrayLength(m),ass2,eqns,BackendDAEUtil.equationNth,-1,{});
          vars1 = BackendVariable.listVar1(vlst);
          eqns1 = BackendEquation.listEquation(elst);
          subsyst = BackendDAE.EQSYSTEM(vars1,eqns1,NONE(),NONE(),BackendDAE.NO_MATCHING());
          IndexReduction.dumpSystemGraphML(subsyst,ishared,NONE(),"System.graphml");
        */

        // check for unassigned vars, if there some rerun 
        unassigned = Matching.getUnassigned(size,ass1,{});
        (outTVars,oMark) = omcTearing3(unassigned,{},m,mt,mapEqnIncRow,mapIncRowEqn,size,vars,ishared,ass1,ass2,columark,mark+1,tvar::inTVars);
      then
        (outTVars,oMark);
    case (tvar::rest,_,_,_,_,_,_,_,_,_,_,_,_)
      equation
        //  print("Selected Var " +& intString(tvar) +& " as TearingVar\n");
        // mark tearing var
        _ = arrayUpdate(ass1,tvar,size*2);
        vareqns = List.removeOnTrue(ass2, isAssignedSaveEnhanced, mt[tvar]); 
        //vareqns = List.removeOnTrue((columark,mark), isMarked, vareqns); 
        //markEqns(vareqns,columark,mark);
        // cheap matching
        tearingBFS(vareqns,m,mt,mapEqnIncRow,mapIncRowEqn,size,ass1,ass2,columark,mark,{});

        /*  vlst = getUnnassignedFromArray(1,arrayLength(mt),ass1,vars,BackendVariable.getVarAt,0,{});
          elst = getUnnassignedFromArray(1,arrayLength(m),ass2,eqns,BackendDAEUtil.equationNth,-1,{});
          vars1 = BackendVariable.listVar1(vlst);
          eqns1 = BackendEquation.listEquation(elst);
          subsyst = BackendDAE.EQSYSTEM(vars1,eqns1,NONE(),NONE(),BackendDAE.NO_MATCHING());
          IndexReduction.dumpSystemGraphML(subsyst,ishared,NONE(),"System.graphml");
        */
        // check for unassigned vars, if there some rerun 
        unassigned = Matching.getUnassigned(size,ass1,{});
        (outTVars,oMark) = omcTearing3(unassigned,rest,m,mt,mapEqnIncRow,mapIncRowEqn,size,vars,ishared,ass1,ass2,columark,mark+1,tvar::inTVars);
      then
        (outTVars,oMark);
    else
      equation
        print("BackendDAEOptimize.omcTearing2 failed!");
      then
        fail();  
  end matchcontinue; 
end omcTearing2;

protected function omcTearingSelectTearingVar "function omcTearingSelectTearingVar
  author: Frenkel TUD 2012-05"
  input BackendDAE.Variables vars;
  input array<Integer> ass1; 
  input array<Integer> ass2;
  input BackendDAE.AdjacencyMatrixEnhanced m;
  input BackendDAE.AdjacencyMatrixTEnhanced mt;
  output Integer tearingVar;
algorithm
  tearingVar := matchcontinue(vars,ass1,ass2,m,mt)
    local
      list<Integer> states,eqns;
      Integer tvar;
      Integer size,varsize;
      array<Integer> points;
    // if vars there with no liniear occurence in any equation use all of them
/*    case(_,_,_,_)
      equation
      then
          
    // if states there use them as tearing variables
    case(_,_,_,_)
      equation
        (_,states) = BackendVariable.getAllStateVarIndexFromVariables(vars);
        states = List.removeOnTrue(ass1, isAssigned, states);
        true = intGt(listLength(states),0);
        tvar = selectVarWithMostEqns(states,ass2,mt,-1,-1);
      then
        tvar;
*/
    /* if there is a variable unsolvable select it */
    case(_,_,_,_,_)
      equation
        tvar = getUnsolvableVarsConsiderMatching(1,BackendVariable.varsSize(vars),mt,ass1,ass2);
      then
        tvar;
       
    case(_,_,_,_,_)
      equation
        varsize = BackendVariable.varsSize(vars);
        states = Matching.getUnassigned(varsize,ass1,{});
        Debug.fcall(Flags.TEARING_DUMP,  print,"omcTearingSelectTearingVar Candidates:\n"); 
        Debug.fcall(Flags.TEARING_DUMP,  BackendDump.debuglst,(states,intString,", ","\n"));  
        size = listLength(states);
        true = intGt(size,0);
        points = arrayCreate(varsize,0);
        points = List.fold2(states, calcVarWights,mt,ass2,points);
        eqns = Matching.getUnassigned(arrayLength(m),ass2,{});
        points = List.fold2(eqns,addEqnWights,m,ass1,points);
        points = List.fold1(states,discriminateDiscrete,vars,points);
        //points = List.fold2(states,addOneEdgeEqnWights,(m,mt),ass1,points);
         Debug.fcall(Flags.TEARING_DUMP,  BackendDump.dumpMatching,points);
        tvar = selectVarWithMostPoints(states,points,-1,-1);
        
        //states = selectVarsWithMostEqns(states,ass2,mt,{},-1);
        //  print("VarsWithMostEqns:\n"); 
        //  BackendDump.debuglst((states,intString,", ","\n"));        
        //tvar = selectVarWithMostEqnsOneEdge(states,ass1,m,mt,-1,-1);
      then
        tvar;        
      else
    equation
        print("omcTearingSelectTearingVar failed because no unmatched var!\n");
      then
        fail();
  end matchcontinue;  
end omcTearingSelectTearingVar;

protected function getUnsolvableVarsConsiderMatching
"function getUnsolvableVarsConsiderMatching
  author: Frenkel TUD 2012-08"
  input Integer index;
  input Integer size;
  input BackendDAE.AdjacencyMatrixTEnhanced meT;
  input array<Integer> ass1;
  input array<Integer> ass2;
  output Integer Unsolvable;
algorithm
  Unsolvable := matchcontinue(index,size,meT,ass1,ass2)
    local
      BackendDAE.AdjacencyMatrixElementEnhanced elem;
      Boolean b;
    case(_,_,_,_,_)
      equation
        true = intLe(index,size);
        /* unmatched var */
        true = intLt(ass1[index],0);
        elem = meT[index];
        /* consider only unmatched eqns */
        elem = removeMatched(elem,ass2,{});
        true = unsolvable(elem);
      then
       index;
    case(_,_,_,_,_)
      equation
        true = intLe(index,size);
      then
       getUnsolvableVarsConsiderMatching(index+1,size,meT,ass1,ass2);
  end matchcontinue;
end getUnsolvableVarsConsiderMatching;

protected function removeMatched
"function removeMatched
  author: Frenkel TUD 2012-08"
  input BackendDAE.AdjacencyMatrixElementEnhanced elem;
  input array<Integer> ass2;
  input BackendDAE.AdjacencyMatrixElementEnhanced iAcc;
  output BackendDAE.AdjacencyMatrixElementEnhanced oAcc;
algorithm
  oAcc := matchcontinue(elem,ass2,iAcc)
    local
      Integer e;
      BackendDAE.AdjacencyMatrixElementEnhanced rest;
      BackendDAE.Solvability s;
    case ({},_,_) then iAcc;
    case ((e,s)::rest,_,_)
      equation
        true = intLt(ass2[e],0);
      then
        removeMatched(rest,ass2,(e,s)::iAcc);
    case ((e,s)::rest,_,_)
      then
        removeMatched(rest,ass2,iAcc);
  end matchcontinue;
end removeMatched;

protected function calcVarWights
 input Integer v;
 input BackendDAE.AdjacencyMatrixTEnhanced mt;
 input array<Integer> ass2;
 input array<Integer> iPoints;
 output array<Integer> oPoints;
protected
 Integer p;
algorithm
  p := calcSolvabilityWight(mt[v],ass2);
  oPoints := arrayUpdate(iPoints,v,p);
end calcVarWights;

protected function calcSolvabilityWight
  input BackendDAE.AdjacencyMatrixElementEnhanced inRow;
  input array<Integer> ass2;
  output Integer w;
algorithm
  w := List.fold1(inRow,solvabilityWightsnoStates,ass2,0);
end calcSolvabilityWight;

protected function solvabilityWightsnoStates "function: solvabilityWights
  author: Frenkel TUD 2012-05"
  input tuple<Integer,BackendDAE.Solvability> inTpl;
  input array<Integer> ass;
  input Integer iW;
  output Integer oW;
algorithm
  oW := matchcontinue(inTpl,ass,iW)
    local
      BackendDAE.Solvability s;
      Integer v,w;
    case((v,s),_,_)
      equation
        true = intGt(v,0);
        false = intGt(ass[v], 0);
        w = solvabilityWights(s);
      then
        intAdd(w,iW);
    else then iW;
  end matchcontinue;
end solvabilityWightsnoStates;

protected function solvabilityWights "function: solvabilityWights
  author: Frenkel TUD 2012-05,
  return a integer for the solvability, this function is used
  to calculade wights for variables to select the tearing variable."
  input BackendDAE.Solvability solva;
  output Integer i;
algorithm
  i := match(solva)
    case BackendDAE.SOLVABILITY_SOLVED() then 0;
    case BackendDAE.SOLVABILITY_CONSTONE() then 2;
    case BackendDAE.SOLVABILITY_CONST() then 5;
    case BackendDAE.SOLVABILITY_PARAMETER(b=false) then 0;
    case BackendDAE.SOLVABILITY_PARAMETER(b=true) then 50;
    case BackendDAE.SOLVABILITY_TIMEVARYING(b=false) then 0;
    case BackendDAE.SOLVABILITY_TIMEVARYING(b=true) then 100;
    case BackendDAE.SOLVABILITY_NONLINEAR() then 200;
    case BackendDAE.SOLVABILITY_UNSOLVABLE() then 300;
  end match;
end solvabilityWights;

protected function addEqnWights
 input Integer e;
 input BackendDAE.AdjacencyMatrixEnhanced m;
 input array<Integer> ass1;
 input array<Integer> iPoints;
 output array<Integer> oPoints;
algorithm
 oPoints := matchcontinue(e,m,ass1,iPoints)
   local
       Integer v1,v2;
       array<Integer> points;
     case (_,_,_,_)
       equation
         ((v1,_)::(v2,_)::{}) = List.removeOnTrue(ass1, isAssignedSaveEnhanced, m[e]); 
          points = arrayUpdate(iPoints,v1,iPoints[v1]+5);
          points = arrayUpdate(iPoints,v2,points[v2]+5);
       then
         points;
     else
       iPoints;
 end matchcontinue;
end addEqnWights;

protected function isAssignedSaveEnhanced
"function isAssigned
  author: Frenkel TUD 2012-05"
  input array<Integer> ass;
  input tuple<Integer,BackendDAE.Solvability> inTpl;
  output Boolean outB;
algorithm
  outB := matchcontinue(ass,inTpl)
    local
      Integer i;
    case (_,(i,_))
      equation
        true = intGt(i,0);
      then
        intGt(ass[i],0); 
    else
      true;
  end matchcontinue;
end isAssignedSaveEnhanced;

protected function discriminateDiscrete
"function discriminateDiscrete
  author: Frenkel TUD 2012-08"
 input Integer v;
 input BackendDAE.Variables vars;
 input array<Integer> iPoints;
 output array<Integer> oPoints;
protected
 Integer p;
 Boolean b;
 BackendDAE.Var var;
algorithm
  var := BackendVariable.getVarAt(vars, v);
  b := BackendVariable.isVarDiscrete(var);
  p := iPoints[v];
  p := Util.if_(b,intDiv(p,10),p);
  oPoints := arrayUpdate(iPoints,v,p);
end discriminateDiscrete;

protected function selectVarWithMostPoints "function selectVarWithMostPoints
  author: Frenkel TUD 2012-05"
  input list<Integer> vars;
  input array<Integer> points;
  input Integer iVar;
  input Integer defp;
  output Integer oVar;
algorithm
  oVar := matchcontinue(vars,points,iVar,defp)
    local
      list<Integer> rest;
      Integer p,v;
    case ({},_,_,_) then iVar;
    case (v::rest,_,_,_)
      equation
        //  print("Var " +& intString(v));
        p = points[v];
        //  print(" has " +& intString(p) +& " Points\n");
        true = intGt(p,defp);
        //  print("max is  " +& intString(defp) +& "\n");
      then
        selectVarWithMostPoints(rest,points,v,p);
    case (_::rest,_,_,_)
      then
        selectVarWithMostPoints(rest,points,iVar,defp);
  end matchcontinue;
end selectVarWithMostPoints;

protected function tearingBFS "function tearingBFS
  author: Frenkel TUD 2012-05"
  input BackendDAE.AdjacencyMatrixElementEnhanced queue;
  input BackendDAE.AdjacencyMatrixEnhanced m;
  input BackendDAE.AdjacencyMatrixTEnhanced mt;
  input array<list<Integer>> mapEqnIncRow;
  input array<Integer> mapIncRowEqn;   
  input Integer size;
  input array<Integer> ass1; 
  input array<Integer> ass2;
  input array<Integer> columark;
  input Integer mark;
  input BackendDAE.AdjacencyMatrixElementEnhanced nextQeue;
algorithm
  _ := match(queue,m,mt,mapEqnIncRow,mapIncRowEqn,size,ass1,ass2,columark,mark,nextQeue)
    local 
      Integer c,eqnsize,cnonscalar;
      BackendDAE.AdjacencyMatrixElementEnhanced rest,newqueue,rows;
    case ({},_,_,_,_,_,_,_,_,_,{}) then ();
    case ({},_,_,_,_,_,_,_,_,_,_)
      equation
        newqueue = List.removeOnTrue(ass2, isAssignedSaveEnhanced, nextQeue);
        newqueue = sortEqnsSolvabel(newqueue,m);
        //  print("NextQeue:\n" +& stringDelimitList(List.map(List.map(newqueue,Util.tuple21),intString),", ") +& "\n");
        tearingBFS(newqueue,m,mt,mapEqnIncRow,mapIncRowEqn,size,ass1,ass2,columark,mark,{});
      then 
        ();
    case((c,_)::rest,_,_,_,_,_,_,_,_,_,_)
      equation
        //  print("Process Eqn " +& intString(c) +& "\n");
        rows = List.removeOnTrue(ass1, isAssignedSaveEnhanced, m[c]); 
        //_ = arrayUpdate(columark,c,mark);
        cnonscalar = mapIncRowEqn[c];
        eqnsize = listLength(mapEqnIncRow[cnonscalar]);
        //  print("Eqn Size " +& intString(eqnsize) +& "\n");
        //  print("Rows: " +& stringDelimitList(List.map(List.map(rows,Util.tuple21),intString),", ") +& "\n");
        newqueue = tearingBFS1(rows,eqnsize,mapEqnIncRow[cnonscalar],mt,ass1,ass2,columark,mark,nextQeue);
        tearingBFS(rest,m,mt,mapEqnIncRow,mapIncRowEqn,size,ass1,ass2,columark,mark,newqueue);
      then 
        ();
  end match; 
end tearingBFS;

protected function sortEqnsSolvabel
"function sortEqnsSolvabel
  author: Frenkel TUD 2012-10
  moves equations with nonlinear or unsolvable parts on the end"
  input BackendDAE.AdjacencyMatrixElementEnhanced queue;
  input BackendDAE.AdjacencyMatrixEnhanced m;
  output BackendDAE.AdjacencyMatrixElementEnhanced nextQeue;
protected
  BackendDAE.AdjacencyMatrixElementEnhanced qnon,qsolv;
algorithm
  (qnon,qsolv) := List.split1OnTrue(queue,hasnonlinearVars,m);
  nextQeue := listAppend(qsolv,qnon);
end sortEqnsSolvabel;

protected function hasnonlinearVars
  input BackendDAE.AdjacencyMatrixElementEnhancedEntry entry;
  input BackendDAE.AdjacencyMatrixEnhanced m;
  output Boolean hasnonlinear;
protected
  Integer r;
  BackendDAE.AdjacencyMatrixElementEnhanced row;
algorithm
  (r,_) := entry;
  row := m[r];
  hasnonlinear := hasnonlinearVars1(row);
end hasnonlinearVars;

protected function hasnonlinearVars1
  input BackendDAE.AdjacencyMatrixElementEnhanced row;
  output Boolean hasnonlinear;
algorithm
  hasnonlinear := match(row)
    local
      BackendDAE.AdjacencyMatrixElementEnhanced rest;
    case ( {}) then false;
    case ((_,BackendDAE.SOLVABILITY_NONLINEAR())::_)
      then
        true;
    case (_::rest)
      then
        hasnonlinearVars1(rest);
  end match;
end hasnonlinearVars1;

protected function tearingBFS1 "function tearingBFS1
  author: Frenkel TUD 2012-05"
  input BackendDAE.AdjacencyMatrixElementEnhanced rows;
  input Integer size;
  input list<Integer> c;
  input BackendDAE.AdjacencyMatrixTEnhanced mt;
  input array<Integer> ass1; 
  input array<Integer> ass2;
  input array<Integer> columark;
  input Integer mark;
  input BackendDAE.AdjacencyMatrixElementEnhanced inNextQeue;
  output BackendDAE.AdjacencyMatrixElementEnhanced outNextQeue;
algorithm
  outNextQeue := matchcontinue(rows,size,c,mt,ass1,ass2,columark,mark,inNextQeue)
    local 
    case (_,_,_,_,_,_,_,_,_)
      equation
        true = intEq(listLength(rows),size);
        true = solvableLst(rows);
        //  print("Assign Eqns: " +& stringDelimitList(List.map(c,intString),", ") +& "\n");
      then
        tearingBFS2(rows,c,mt,ass1,ass2,columark,mark,inNextQeue);
    case (_,_,_,_,_,_,_,_,_)
      equation
        true = intEq(listLength(rows),size);
        false = solvableLst(rows);
        //  print("cannot Assign Var" +& intString(r) +& " with Eqn " +& intString(c) +& "\n");
      then 
        inNextQeue;
    else then inNextQeue;
  end matchcontinue; 
end tearingBFS1;

protected function solvableLst
  input BackendDAE.AdjacencyMatrixElementEnhanced rows;
  output Boolean solvable;
algorithm
  solvable := match(rows)
    local 
      Integer r;
      BackendDAE.Solvability s;
      BackendDAE.AdjacencyMatrixElementEnhanced rest;
    case ((r,s)::{}) then solvable(s);   
    case ((r,s)::rest)
      equation
        true = solvable(s);   
      then 
        solvableLst(rest);   
  end match;
end solvableLst;

protected function solvable
  input BackendDAE.Solvability s;
  output Boolean b;
algorithm
  b := match(s)
    case BackendDAE.SOLVABILITY_SOLVED() then true;
    case BackendDAE.SOLVABILITY_CONSTONE() then true;
    case BackendDAE.SOLVABILITY_CONST() then true;
    case BackendDAE.SOLVABILITY_PARAMETER(b=b) then b;
    case BackendDAE.SOLVABILITY_TIMEVARYING(b=b) then false;
    case BackendDAE.SOLVABILITY_NONLINEAR() then false;
    case BackendDAE.SOLVABILITY_UNSOLVABLE() then false;
  end match; 
end solvable;

protected function tearingBFS2 "function tearingBFS1
  author: Frenkel TUD 2012-05"
  input BackendDAE.AdjacencyMatrixElementEnhanced rows;
  input list<Integer> clst;
  input BackendDAE.AdjacencyMatrixTEnhanced mt;
  input array<Integer> ass1; 
  input array<Integer> ass2;
  input array<Integer> columark;
  input Integer mark;
  input BackendDAE.AdjacencyMatrixElementEnhanced inNextQeue;
  output BackendDAE.AdjacencyMatrixElementEnhanced outNextQeue;
algorithm
  outNextQeue := match(rows,clst,mt,ass1,ass2,columark,mark,inNextQeue)
    local 
      Integer r,c;
      list<Integer> ilst;
      BackendDAE.Solvability s;
      BackendDAE.AdjacencyMatrixElementEnhanced rest,vareqns,newqueue;
    case ({},_,_,_,_,_,_,_) then inNextQeue;
    case ((r,s)::rest,c::ilst,_,_,_,_,_,_)
      equation
        //  print("Assign Var " +& intString(r) +& " with Eqn " +& intString(c) +& "\n");
        // assigen 
        _ = arrayUpdate(ass1,r,c);
        _ = arrayUpdate(ass2,c,r);
        vareqns = List.removeOnTrue(ass2, isAssignedSaveEnhanced, mt[r]);  
        //vareqns = List.removeOnTrue((columark,mark), isMarked, vareqns);   
        //markEqns(vareqns,columark,mark);     
        newqueue = listAppend(inNextQeue,vareqns);
      then 
        tearingBFS2(rest,ilst,mt,ass1,ass2,columark,mark,newqueue);
  end match; 
end tearingBFS2;

protected function addOneEdgeEqnWights
 input Integer v;
 input tuple<BackendDAE.AdjacencyMatrixEnhanced,BackendDAE.AdjacencyMatrixTEnhanced> mmt;
 input array<Integer> ass1;
 input array<Integer> iPoints;
 output array<Integer> oPoints;
algorithm
 oPoints := matchcontinue(v,mmt,ass1,iPoints)
   local
       BackendDAE.AdjacencyMatrixEnhanced m;
       BackendDAE.AdjacencyMatrixTEnhanced mt;
       list<Integer> elst;
       Integer e;
       array<Integer> points;
     case (_,(m,mt),_,_)
       equation
         elst = List.fold2(mt[v],eqnsWithOneUnassignedVar,m,ass1,{});
          e = listLength(elst);
          points = arrayUpdate(iPoints,v,iPoints[v]+e);
       then
         points;
     else
       iPoints;
 end matchcontinue;
end addOneEdgeEqnWights;

protected function selectVarWithMostEqnsOneEdge
"function selectVarWithMostEqnsOneEdge
  author: Frenkel TUD 2012-08"
  input list<Integer> vars;
  input array<Integer> ass1;
  input BackendDAE.AdjacencyMatrixEnhanced m;
  input BackendDAE.AdjacencyMatrixTEnhanced mt;
  input Integer defaultVar;
  input Integer eqns;
  output Integer var;
algorithm
  var := matchcontinue(vars,ass1,m,mt,defaultVar,eqns)
    local
      list<Integer> rest,elst;
      Integer e,v;
    case ({},_,_,_,_,_) then defaultVar;
    case (v::rest,_,_,_,_,_)
      equation
        elst = List.fold2(mt[v],eqnsWithOneUnassignedVar,m,ass1,{});
        e = listLength(elst);
        //  print("Var " +& intString(v) +& " has " +& intString(e) +& " one eqns\n");
        true = intGt(e,eqns);
      then
        selectVarWithMostEqnsOneEdge(rest,ass1,m,mt,v,e);
    case (_::rest,_,_,_,_,_)
      then
        selectVarWithMostEqnsOneEdge(rest,ass1,m,mt,defaultVar,eqns);
  end matchcontinue;
end selectVarWithMostEqnsOneEdge;

protected function eqnsWithOneUnassignedVar "function: eqnsWithOneUnassignedVar
  author: Frenkel TUD 2012-05"
  input tuple<Integer,BackendDAE.Solvability> inTpl;
  input BackendDAE.AdjacencyMatrixEnhanced m;
  input array<Integer> ass;
  input list<Integer> iLst;
  output list<Integer> oLst;
algorithm
  oLst := matchcontinue(inTpl,m,ass,iLst)
    local
      BackendDAE.Solvability s;
      Integer e;
      BackendDAE.AdjacencyMatrixElementEnhanced vars;
    case((e,s),_,_,_)
      equation
        true = intGt(e,0);
        vars = List.removeOnTrue(ass, isAssignedSaveEnhanced, m[e]);
        //  print("Eqn " +& intString(e) +& " has " +& intString(listLength(vars)) +& " vars\n");
        true = intEq(listLength(vars), 2);
      then
        e::iLst;
    else then iLst;            
  end matchcontinue; 
end eqnsWithOneUnassignedVar;

protected function selectVarsWithMostEqns "function selectVarWithMostEqns
  author: Frenkel TUD 2012-05"
  input list<Integer> vars;
  input array<Integer> ass2;
  input BackendDAE.AdjacencyMatrixTEnhanced mt;
  input list<Integer> iVars;
  input Integer eqns;
  output list<Integer> oVars;
algorithm
  oVars := matchcontinue(vars,ass2,mt,iVars,eqns)
    local
      list<Integer> rest,vlst;
      Integer e,v;
    case ({},_,_,_,_) then iVars;
    case (v::rest,_,_,_,_)
      equation
        e = calcSolvabilityWight(mt[v],ass2);
        //  print("Var " +& intString(v) +& "has w= " +& intString(e) +& "\n");
        true = intGe(e,eqns);
        vlst = List.consOnTrue(intEq(e,eqns),v,iVars);
        ((vlst,e)) = Util.if_(intGt(e,eqns),({v},e),(vlst,eqns));
        //  print("max is  " +& intString(eqns) +& "\n");
        //  BackendDump.debuglst((vlst,intString,", ","\n"));
      then
        selectVarsWithMostEqns(rest,ass2,mt,vlst,e);
    case (_::rest,_,_,_,_)
      then
        selectVarsWithMostEqns(rest,ass2,mt,iVars,eqns);
  end matchcontinue;
end selectVarsWithMostEqns;

protected function markEqns "function markEqns
  author: Frenkel TUD 2012-05"
  input list<Integer> eqns;
  input array<Integer> columark;
  input Integer mark;
algorithm
  _ := match(eqns,columark,mark)
    local
      Integer e;
      list<Integer> rest;
    case({},_,_) then ();
    case(e::rest,_,_)
      equation
        _ = arrayUpdate(columark,e,mark);
        markEqns(rest,columark,mark);
      then
        ();
  end match; 
end markEqns;

protected function getUnnassignedFromArray
  replaceable type Type_a subtypeof Any;
  replaceable type Type_b subtypeof Any;
  input Integer indx;
  input Integer size;
  input array<Integer> ass;
  input Type_b inTypeAArray;
  input FuncTypeType_aFromArray func;
  input Integer off;
  input list<Type_a> iALst;
  output list<Type_a> oALst;
  partial function FuncTypeType_aFromArray
    input Type_b inTypeB;
    input Integer indx;
    output Type_a outTypeA;
  end FuncTypeType_aFromArray;
algorithm
  oALst := matchcontinue(indx,size,ass,inTypeAArray,func,off,iALst)
    local
      Type_a a;
    case (_,_,_,_,_,_,_)
      equation
        true = intLe(indx,size);
        true = intLt(ass[indx],1);
        a = func(inTypeAArray,indx+off);
      then
        getUnnassignedFromArray(indx+1,size,ass,inTypeAArray,func,off,a::iALst);
    case (_,_,_,_,_,_,_)
      equation
        true = intLe(indx,size);
      then
        getUnnassignedFromArray(indx+1,size,ass,inTypeAArray,func,off,iALst);
    else
      listReverse(iALst); 
  end matchcontinue;
end getUnnassignedFromArray;

protected function omcTearing3 "function omcTearing3
  author: Frenkel TUD 2012-05"
  input list<Integer> unassigend;
  input list<Integer> unsolvables;
  input BackendDAE.AdjacencyMatrixEnhanced m;
  input BackendDAE.AdjacencyMatrixTEnhanced mt;
  input array<list<Integer>> mapEqnIncRow;
  input array<Integer> mapIncRowEqn;    
  input Integer size;
  input BackendDAE.Variables vars;
  input BackendDAE.Shared ishared;
  input array<Integer> ass1; 
  input array<Integer> ass2;
  input array<Integer> columark;
  input Integer mark;
  input list<Integer> inTVars;
  output list<Integer> outTVars;
  output Integer oMark;
algorithm
  (outTVars,oMark) := match(unassigend,unsolvables,m,mt,mapEqnIncRow,mapIncRowEqn,size,vars,ishared,ass1,ass2,columark,mark,inTVars)
    local 
    case ({},_,_,_,_,_,_,_,_,_,_,_,_,_) then (inTVars,mark);
    else 
      equation
        (outTVars,oMark) = omcTearing2(unsolvables,m,mt,mapEqnIncRow,mapIncRowEqn,size,vars,ishared,ass1,ass2,columark,mark,inTVars);
      then
        (outTVars,oMark);
  end match; 
end omcTearing3;

protected function omcTearing4 "function omcTearing4
  author: Frenkel TUD 2012-09"
  input BackendDAE.JacobianType jacType;
  input BackendDAE.EqSystem isyst;
  input BackendDAE.Shared ishared;
  input BackendDAE.EqSystem subsyst;
  input list<Integer> tvars;
  input list<Integer> residual;
  input array<Integer> ass1;
  input array<Integer> ass2;
  input list<list<Integer>> othercomps;
  input list<Integer> eindex;
  input list<Integer> vindx;
  input array<list<Integer>> mapEqnIncRow;
  input array<Integer> mapIncRowEqn;
  input array<Integer> columark;
  input Integer mark;
  output BackendDAE.StrongComponent ocomp;
  output Boolean outRunMatching;
algorithm
  (ocomp,outRunMatching):=
    matchcontinue (jacType,isyst,ishared,subsyst,tvars,residual,ass1,ass2,othercomps,eindex,vindx,mapEqnIncRow,mapIncRowEqn,columark,mark)
    local
      list<Integer> ores,residual1,ovars;
      list<tuple<Integer,list<Integer>>> eqnvartpllst;
      array<Integer> eindxarr,varindxarr;
      Boolean linear;
    case (_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)
      equation
        Debug.fcall(Flags.TEARING_DUMP, print,"handle torn System\n");
        // map indexes back
        residual1 = List.map1r(residual,arrayGet,mapIncRowEqn);
        residual1 = List.fold2(residual1,uniqueIntLst,mark,columark,{});
        eindxarr = listArray(eindex);
        ores = List.map1r(residual1,arrayGet,eindxarr);
        varindxarr = listArray(vindx);
        ovars = List.map1r(tvars,arrayGet,varindxarr);
        eqnvartpllst = omcTearing4_1(othercomps,ass2,mapIncRowEqn,eindxarr,varindxarr,columark,mark,{});
        linear = getLinearfromJacType(jacType);
      then
        (BackendDAE.TORNSYSTEM(ovars, ores, eqnvartpllst, linear),true);           
    case (_,_,_,_,_,_,_,_,_,_,_,_,_,_,_)
      then
        (BackendDAE.TORNSYSTEM({}, {}, {}, false),false);        
  end matchcontinue;  
end omcTearing4;

protected function omcTearing4_1 "function omcTearing4_1
  author: Frenkel TUD 2012-09"
  input list<list<Integer>> othercomps;
  input array<Integer> ass2;
  input array<Integer> mapIncRowEqn;
  input array<Integer> eindxarr;
  input array<Integer> varindxarr;
  input array<Integer> columark;
  input Integer mark;  
  input list<tuple<Integer,list<Integer>>> iAcc;
  output list<tuple<Integer,list<Integer>>> oEqnVarTplLst;
algorithm
  oEqnVarTplLst :=
  match (othercomps,ass2,mapIncRowEqn,eindxarr,varindxarr,columark,mark,iAcc)
    local
      list<list<Integer>> rest;
      Integer e,v,c;
      list<Integer> vlst,clst,elst;
    case ({},_,_,_,_,_,_,_) then listReverse(iAcc);
    case ({c}::rest,_,_,_,_,_,_,_)
      equation
        e = mapIncRowEqn[c];
        e = eindxarr[e];
        v = ass2[c];
        v = varindxarr[v];
      then
        omcTearing4_1(rest,ass2,mapIncRowEqn,eindxarr,varindxarr,columark,mark,(e,{v})::iAcc);
    case (clst::rest,_,_,_,_,_,_,_)
      equation
        elst = List.map1r(clst,arrayGet,mapIncRowEqn);
        elst = List.fold2(elst,uniqueIntLst,mark,columark,{});
        {e} = elst;
        e = eindxarr[e];
        vlst = List.map1r(clst,arrayGet,ass2);
        vlst = List.map1r(vlst,arrayGet,varindxarr);
      then
        omcTearing4_1(rest,ass2,mapIncRowEqn,eindxarr,varindxarr,columark,mark,(e,vlst)::iAcc);
  end match;
end omcTearing4_1;

protected function getLinearfromJacType "function getLinearfromJacType
  author: Frenkel TUD 2012-09"
  input BackendDAE.JacobianType jacType;
  output Boolean linear;
algorithm
  linear := match(jacType)
    case (BackendDAE.JAC_CONSTANT()) then true;
    case (BackendDAE.JAC_TIME_VARYING()) then true;
    case (BackendDAE.JAC_NONLINEAR()) then false;
    case (BackendDAE.JAC_NO_ANALYTIC()) then false;
  end match;
end getLinearfromJacType;

/*
 * Tearing from Book of Celier
 *
 */

protected function tearingSystem1_1 "function tearingSystem1_1
  author: Waurich TUD 2012-10"
  input BackendDAE.EqSystem isyst;
  input BackendDAE.Shared ishared;
  input list<Integer> eindex;
  input list<Integer> vindx;
  input Option<list<tuple<Integer, Integer, BackendDAE.Equation>>> ojac;
  input BackendDAE.JacobianType jacType;
  output BackendDAE.StrongComponent ocomp;
  output Boolean outRunMatching;
protected
  list<Integer> tvars,residual,unsolvables,unassigned,potentials;
  list<list<Integer>> othercomps;
  BackendDAE.EqSystem syst,subsyst;
  BackendDAE.Shared shared;   
  array<Integer> columark,number,lowlink;
  list<Integer> ass1,ass2;
  list<Integer> assignables,OutTVars;
  Integer size,tornsize,tvar,mark;
  list<BackendDAE.Equation> eqn_lst; 
  list<BackendDAE.Var> var_lst;    
  BackendDAE.Variables vars;
  BackendDAE.EquationArray eqns;
  BackendDAE.IncidenceMatrix m,m1;
  BackendDAE.IncidenceMatrix mt,mt1;      
  BackendDAE.AdjacencyMatrixEnhanced me;
  BackendDAE.AdjacencyMatrixTEnhanced meT;
  BackendDAE.AdjacencyMatrixElementEnhanced vareqns;
  array<list<Integer>> mapEqnIncRow;
  array<Integer> mapIncRowEqn;  
  list<list<Integer>> orderIn;
  Boolean causal; 
  list<tuple<Integer,BackendDAE.Solvability>> row;  
algorithm
  // generate Subsystem to get the incidence matrix
  size := listLength(vindx);
  eqn_lst := BackendEquation.getEqns(eindex,BackendEquation.daeEqns(isyst));  
  eqns := BackendEquation.listEquation(eqn_lst);      
  var_lst := List.map1r(vindx, BackendVariable.getVarAt, BackendVariable.daeVars(isyst));
  vars := BackendVariable.listVar1(var_lst);
  subsyst := BackendDAE.EQSYSTEM(vars,eqns,NONE(),NONE(),BackendDAE.NO_MATCHING(),{});
  (subsyst,m,mt,_,_) := BackendDAEUtil.getIncidenceMatrixScalar(subsyst, BackendDAE.NORMAL(),NONE());
  BackendDump.printEqSystem(subsyst);
  Debug.fcall(Flags.TEARING_DUMP, BackendDump.printEqSystem,subsyst);
  //get advanced incidence matrix 
  (me,meT,mapEqnIncRow,mapIncRowEqn) := BackendDAEUtil.getAdjacencyMatrixEnhancedScalar(subsyst,ishared);
  unsolvables := getUnsolvableVars(1,size,meT,{});
  ass1 := List.fill(-1,size);
  ass2 := List.fill(-1,size);
  orderIn := {{},{}};
  //(OutTVars,_,_) := TearingSystemOlleroAmselem(m,mt,ass1,ass2);
  //(OutTVars,_,_) := TearingSystemSteward(m,mt,ass1,ass2);
  OutTVars := TearingSystemCellier(false,m,mt,me,meT,ass1,ass2,unsolvables,{},orderIn);
  //OutTVars := TearingSystemCarpanzano(false,m,mt,me,meT,ass1,ass2,unsolvables,{},orderIn);
  print("no of eqs"+&intString(size)+&"\n");
  print("OUTVARS"+&stringDelimitList(List.map(OutTVars,intString),",")+&"\n"); 
  print("no of tvars"+&intString(listLength(OutTVars))+&"\n");
  ocomp := BackendDAE.SINGLEEQUATION(0,0);
  outRunMatching := false;  
end tearingSystem1_1;

protected function TearingSystemSteward" assigns vars to eqs 
with StewardAssignment, selects tearingSet from digraph according 
to Stewards algorithm.
author:Waurich TUD 2012-12"
  input BackendDAE.IncidenceMatrix mIn;
  input BackendDAE.IncidenceMatrixT mtIn;
  input list<Integer> ass1In;
  input list<Integer> ass2In;
  output list<Integer> OutTVars,ass1Out,ass2Out;
  array<list<Integer>> a,at;
  array<list<Integer>> l,lt;
  list<list<Integer>> loops;
  list<Integer> vertParent,vertChild;
algorithm
  (ass1Out,ass2Out) := AssignmentSteward(mIn,mtIn,ass1In,ass2In,1);
  print("ass2"+&stringDelimitList(List.map(ass2Out,intString),",")+&"\n");
  print("ass1"+&stringDelimitList(List.map(ass1Out,intString),",")+&"\n");
  OutTVars := {};
  (a,at) := getAdjacencyMatrix(mIn,mtIn,ass1Out,ass2Out);
  print("a\n");
  BackendDump.dumpIncidenceMatrix(a);
  print("at\n");
  BackendDump.dumpIncidenceMatrix(at);
  loops := findLoops(a,at);
  print(intString(listLength(loops))+&"loops"+&stringDelimitList(List.map(List.flatten(loops),intString),",")+&"\n");
  (l,lt) := getLoopOccurenceMatrix(a,at,loops);
  print("l\n");
  BackendDump.dumpIncidenceMatrix(l);
  print("lt\n");
  BackendDump.dumpIncidenceMatrix(lt);
  (vertParent,vertChild) := tearingSteward(l,lt,{},{});
  OutTVars := StewardConvertToTearingVar(vertParent,vertChild,ass1Out,ass2Out);
  print("Tvars"+&stringDelimitList(List.map(OutTVars,intString),",")+&"\n");
end TearingSystemSteward;     

protected function StewardConvertToTearingVar "converts the chosen edges(vertex pair)
from the digraph to a tearing set.
author: Waurich TUD 2013-01"
  input list<Integer> vertParent,vertChild;
  input list<Integer> ass1,ass2;
  output list<Integer> tVars;
algorithm
  tVars := StewardConvertToTearingVar2(vertParent,vertChild,{},ass1,ass2,1);
  tVars := List.unique(tVars);
end StewardConvertToTearingVar;

protected function StewardConvertToTearingVar2 "implementation for StewardConvertToTearingVar.
author: Waurich TUD 2013-01"
  input list<Integer> vertParent,vertChild;
  input list<Integer> varsIn,ass1,ass2;
  input Integer indx;
  output list<Integer> tVars;
algorithm
  tVars := matchcontinue(vertParent,vertChild,varsIn,ass1,ass2,indx)
  local
    list<Integer> vars;
    Integer var;
  case(_,_,_,_,_,_)
    equation
      true = indx <= listLength(vertChild);
      var = listGet(ass1,listGet(vertChild,indx));
      print("variable"+&intString(var)+&"\n");
      vars = var::varsIn;
      tVars = StewardConvertToTearingVar2(vertParent,vertChild,vars,ass1,ass2,indx+1);
        then
          tVars;
  case(_,_,_,_,_,_)
    equation
      true = indx > listLength(vertChild);
        then
          varsIn;
  end matchcontinue;
end StewardConvertToTearingVar2;

protected function tearingSteward "selects the tearing vertices from the loop-occurence-matrix.
author: Waurich TUD 2013-01"
  input array<list<Integer>> lIn,ltIn;
  input list<Integer> parentIn, childIn;
  output list<Integer> parentOut,childOut;
algorithm
  vertsOut := matchcontinue(lIn,ltIn,parentIn,childIn)
  local
    array<list<Integer>> l,lt;
    list<Integer> counter,values,potentials;
    list<Integer> parentLst,childLst;   
    Integer parent,child;
  case(_,_,_,_)
    equation
      true = arrayLength(lIn) > 0;   
      (counter,values) = countMultiples(ltIn);
      print("counter"+&stringDelimitList(List.map(counter,intString),",")+&"\n");
      print("values"+&stringDelimitList(List.map(values,intString),",")+&"\n");
      potentials = maxListInt(counter);
      parent = listGet(potentials,1);
      child = listGet(values,parent);
      parentLst = parent::parentIn;
      childLst = child::childIn;
      (l,lt) = updateLoopOccurenceMatrix(lIn,ltIn,parent,child);
      //print("l\n");
      //BackendDump.dumpIncidenceMatrix(l);
      //print("lt\n");
      //BackendDump.dumpIncidenceMatrix(lt);
      (parentIn,childIn) = tearingSteward(l,lt,parentLst,childLst);
      then 
        (parentIn,childIn);
  case(_,_,_,_)
    equation
      true = arrayLength(lIn) == 0;
      then 
        (parentIn,childIn);
  end matchcontinue;
end tearingSteward;

protected function updateLoopOccurenceMatrix "removes broken loops from loop-occurence-matrix.
author: Waurich TUD 2013-01"
  input array<list<Integer>> lIn,ltIn;
  input Integer parent,child;
  output array<list<Integer>> lOut,ltOut;
algorithm
  (lOut,ltOut) := matchcontinue(lIn,ltIn,parent,child)
  local
    array<list<Integer>> l,lt;
    list<Integer> row,loops,broken,leftovers; 
    Integer size;
    case(_,_,_,_)
      equation
        row = arrayGet(ltIn,parent);
        true = List.isMemberOnTrue(child,row,intEq);
        broken = listGetPositions(row,child);
        size = arrayLength(lIn);
        loops = listSeries(size);
        (broken,leftovers,_) = List.intersection1OnTrue(loops,broken,intEq);
        l = Util.arraySelect(lIn,leftovers);
        lt = arrayDeleteColumns(ltIn,broken); 
        then 
          (l,lt);
    end matchcontinue;
  end updateLoopOccurenceMatrix;      

protected function arrayDeleteColumns "deletes columns from array<list<Integer>>
given by indexes(starting with 1).
author:Waurich TUD 2013-01"
  input array<list<Integer>> inArr;
  input list<Integer> indxs;
  output array<list<Integer>> outArr;
algorithm
  outArr := arrayDeleteColumns2(inArr,indxs,1);
end arrayDeleteColumns;
  
protected function arrayDeleteColumns2 "implementation of arrayDeleteColumns.
author: Waurich TUD 2013-01"
  input array<list<Integer>> inArr;
  input list<Integer> indxs;
  input Integer indx;
  output array<list<Integer>> outArr;   
algorithm
  outArr := matchcontinue(inArr,indxs,indx)
  local
    array<list<Integer>> Arr;
    list<Integer> row;
  case(_,_,_)
    equation
    true = indx <= arrayLength(inArr);
    row = arrayGet(inArr,indx);
    row = List.deletePositions(row,List.map(indxs,intSubOne));
    Arr = Util.arrayReplaceAtWithFill(indx,row,{},inArr);
    outArr = arrayDeleteColumns2(Arr,indxs,indx+1);
      then
        outArr;
  case(_,_,_)
    equation
    true = indx > arrayLength(inArr);
      then
        inArr;
  end matchcontinue;
end arrayDeleteColumns2;

protected function intSubOne  " mapFunction which subtracts 1 from every entry in a list.
author:Waurich TUD 2013-01"
  input Integer inValue;
  output Integer outValue;
algorithm
  outValue := inValue - 1;
end intSubOne;

protected function listSeries " creates a list<Integer> with n entries and every entry is his previous entry +1 starting with 1.
author: Waurich TUD 2013-01"
  input Integer n;
  output list<Integer> lst;
algorithm
  lst := listSeries2(n,{});
end listSeries;
  
protected function listSeries2 "implementation of listSeries.
author: Waurich TUD 2013-01"
    input Integer n;
    input list<Integer> lstIn;
    output list<Integer> lstOut;
  algorithm   
    lstOut := matchcontinue(n,lstIn)
    local
      list<Integer> lst;
      Integer entry;
      case(_,_)
        equation
          true = listLength(lstIn) <= n-1;
          entry = n-listLength(lstIn);
          lst = entry::lstIn;
          lst = listSeries2(n,lst);
          then
            lst;
      case(_,_)
        equation
          true = listLength(lstIn) > n-1;
          then
            lstIn;
   end matchcontinue;
 end listSeries2;         

  protected function listGetPositions " gets the indexes of the entries equal to value.
author: Waurich TUD 2013-01"
  input list<Integer> lstIn;
  input Integer value;
  output list<Integer> lstOut;
  Integer number;
algorithm
  number := listLength(lstIn)-listLength(List.removeOnTrue(value,intEq,lstIn));
  lstOut := listGetPositions2(lstIn,value,number,1,{});
end listGetPositions;

protected function listGetPositions2 "implementation for listGetPositions.
author: Waurich TUD 2013-01"
  input list<Integer> lstIn;
  input Integer value,num,indx;
  input list<Integer> posIn;
  output list<Integer> posOut;
algorithm
  posOut := matchcontinue(lstIn,value,num,indx,posIn)
  local
    list<Integer> lst,positions;
    Integer pos;
    case(_,_,_,_,_)
      equation
        true = indx <= num;
        pos = List.position(value,lstIn)+1+indx-1;
        lst = List.deleteMember(lstIn,value);
        positions = pos::posIn;
        posOut = listGetPositions2(lst,value,num,indx+1,positions);
          then
            posOut;
    case(_,_,_,_,_)
      equation
        true = indx > num;
          then
            posIn;
  end matchcontinue;
end listGetPositions2;            

protected function countMultiples "counts multiple entries in array<list<Integer row(list)-wise.
counter gives the maximum amount of same entries and value gives the corresponding entry.
if only 0s appear in the row, then (0,0).
author: Waurich TUD 2013-01"
  input array<list<Integer>> inArr;
  output list<Integer> counter,values;
algorithm
  ((counter,values,_)) := Util.arrayFold(inArr,countMultiples2,({},{},1));
end countMultiples;

protected function countMultiples2 " FoldFunc for countMultiples.if entries appear equaly often,
just one is taken.
author: Waurich TUD 2013-01"
  input list<Integer> rowIn;
  input tuple<list<Integer>,list<Integer>,Integer> valIn;
  output tuple<list<Integer>,list<Integer>,Integer> valOut;
protected
  list<Integer> counter,values,row,set,num,val,positions;
  Integer indx,value,number,position;
algorithm
  (counter,values,indx) := valIn;
  row := List.removeOnTrue(0,intEq,rowIn);
  set := List.unique(row);
  (val,num) := countMultiples3(row,set,1,{},{});
  positions := maxListInt(num);
  position := listGet(positions,1);
  number := listGet(num,position);
  value := listGet(val,position);
  counter := List.set(counter,indx,number);
  values := List.set(values,indx,value);
  valOut := (counter,values,indx+1);
end countMultiples2;        
 
protected function countMultiples3 " helper function for countMultiples2.
author:Waurich TUD 2013-01"
  input list<Integer> lstIn,set;
  input Integer indx;
  input list<Integer> valIn,numIn;
  output list<Integer> valOut,numOut;
algorithm
(num,value) := matchcontinue(lstIn,set,indx,valIn,numIn)
  local
    Integer value,number;
    list<Integer> val,num;
  case(_,_,_,_,_)
    equation
      true = listLength(set) == 0;
      (valOut,numOut) = ({0},{0});
    then
      (valOut,numOut);
  case(_,_,_,_,_)
    equation
      true = indx <= listLength(set);
      value = listGet(set,indx);
      number = listLength(lstIn)-listLength(List.removeOnTrue(value,intEq,lstIn));
      num = number::numIn;
      val = value::valIn;
      (valOut,numOut) = countMultiples3(lstIn,set,indx+1,val,num);
    then
      (valOut,numOut);
  case(_,_,_,_,_)
    equation
      true = indx > listLength(set);
    then
      (valIn,numIn);
  end matchcontinue;
end countMultiples3;
 
protected function getLoopOccurenceMatrix "builds the loop-occurence matrix. 
rows correspond to the loops and columns to the included vertices.no reduced matrix representation(contains 0s)
author: Waurich TUD 2013-01"
  input array<list<Integer>> a;
  input array<list<Integer>> at;
  input list<list<Integer>> loops;
  output array<list<Integer>> l,lt;
  list<Integer> row;
algorithm
  row := List.fill(0,arrayLength(a));
  l := arrayCreate(listLength(loops),row);
  row := List.fill(0,listLength(loops));
  lt := arrayCreate(arrayLength(a),row);
  (l,lt) := getLoopOccurenceMatrix2(loops,l,lt,1,1);
end getLoopOccurenceMatrix;

protected function getLoopOccurenceMatrix2 " helper function for getLoopOccurenceMatrix.
  author:Waurich TUD 2013-01"
  input list<list<Integer>> loops;
  input array<list<Integer>> lIn,ltIn;
  input Integer indxLoop,indxVert;
  output array<list<Integer>> lOut,ltOut;
algorithm
  (lOut,ltOut) := matchcontinue(loops,lIn,ltIn,indxLoop,indxVert)
    local
      Integer parent,child;
      list<Integer> row,currLoop;
      array<list<Integer>> l,lt;      
    case(_,_,_,_,_)
      equation
      true = indxLoop <= listLength(loops);
      currLoop = listGet(loops,indxLoop);
      true = indxVert <= listLength(currLoop);
      row = arrayGet(lIn,indxLoop);
      parent = listGet(currLoop,indxVert);
      child = chooseChildInLoop(currLoop,parent);
      row = List.replaceAt(child,parent-1,row);
      l = Util.arrayReplaceAtWithFill(indxLoop,row,{},lIn);
      row = arrayGet(ltIn,parent);
      row = List.replaceAt(child,indxLoop-1,row);
      lt = Util.arrayReplaceAtWithFill(parent,row,{},ltIn);
      (lOut,ltOut) = getLoopOccurenceMatrix2(loops,l,lt,indxLoop,indxVert+1);
      then
        (lOut,ltOut);
    case(_,_,_,_,_)
      equation
          true = indxLoop <= listLength(loops);
          currLoop = listGet(loops,indxLoop);
          true = indxVert > listLength(currLoop);
        (lOut,ltOut) = getLoopOccurenceMatrix2(loops,lIn,ltIn,indxLoop+1,1);
        then
        (lOut,ltOut);
    case(_,_,_,_,_)
      equation
        true = indxLoop > listLength(loops);
        then
          (lIn,ltIn);
  end matchcontinue;
end getLoopOccurenceMatrix2;        
      
protected function chooseChildInLoop "determines the child-vertex in a loop
author: Wauricht TUD 2013-01"
  input list<Integer>loopIn;
  input Integer parent;
  output Integer child;
algorithm
  child := matchcontinue(loopIn,parent)
  local
    Integer parentIndx,childIndx;
    case(_,_)
      equation
      parentIndx = List.position(parent,loopIn)+1;
      true = parentIndx == 1;     
      child = List.last(loopIn);
        then
          child;
    else
    equation
    parentIndx = List.position(parent,loopIn)+1;
    child = listGet(loopIn,parentIndx-1);
      then
        child;
  end matchcontinue;
end chooseChildInLoop;
      
protected function findLoops "searches the AdjacencyMatrix for loops
author:Waurich TUD 2012-12"
  input array<list<Integer>> a,at;
  output list<list<Integer>> loops;
  list<Integer> currLoop;
algorithm
  currLoop := {};
  currLoop := 1::currLoop;
  loops := findLoops2(a,at,{},{},currLoop,1,1);
end findLoops;

protected function findLoops2 " helper function for findLoops
author:Waurich TUD 2012-12"
  input array<list<Integer>> aIn,atIn;
  input list<list<Integer>> finLoops,unfinLoops;
  input list<Integer> currLoop;
  input Integer indxRow,indxCol;
  output list<list<Integer>> loopsOut;
algorithm
  loopsOut := matchcontinue(aIn,atIn,finLoops,unfinLoops,currLoop,indxRow,indxCol)
  local
    Integer parent,child,position;
    list<Integer> row,openLoop;
    Boolean empty;
  case(_,_,_,_,_,0,_)
    equation
      print("loopfinding ready\n");
      print("found Loop "+&stringDelimitList(List.map(List.flatten(finLoops),intString),",")+&"\n");
        then finLoops;
  case(_,_,_,_,_,_,_)
    equation
      parent = indxRow;
      openLoop = List.deleteMember(currLoop,parent);
      true = List.isMemberOnTrue(parent,openLoop,intEq);
      position = List.position(parent,openLoop)+2;
      (currLoop,_) = List.split(currLoop,position);
      print("found Loop "+&stringDelimitList(List.map(currLoop,intString),",")+&"\n");
      currLoop = listDelete(currLoop,0);
      finLoops = currLoop::finLoops;
      currLoop = List.flatten(List.firstOrEmpty(unfinLoops));
      indxRow = firstOrEmpty(currLoop,0);
      indxCol = 1;
      unfinLoops = deleteOrEmpty(unfinLoops,0);
      loopsOut = findLoops2(aIn,atIn,finLoops,unfinLoops,currLoop,indxRow,indxCol);      
        then loopsOut;
  case(_,_,_,_,_,_,_)
    equation
      true = listLength(currLoop) > 0;
      true = indxCol == listLength(arrayGet(aIn,indxRow));
      parent = indxRow;
      child = listGet(arrayGet(aIn,indxRow),indxCol);
      currLoop = child::currLoop;
      print("append loop and jump to next vertex "+&stringDelimitList(List.map(currLoop,intString),",")+&"\n");
      loopsOut = findLoops2(aIn,atIn,finLoops,unfinLoops,currLoop,child,1);
      then loopsOut;        
  case(_,_,_,_,_,_,_)
    equation
      true = indxCol < listLength(arrayGet(aIn,indxRow));
      print("now we have more loops\n");
      parent = indxRow;
      row = arrayGet(aIn,indxRow);
      unfinLoops = traceMooreLoops(unfinLoops,currLoop,row,2);
      child = listGet(arrayGet(aIn,indxRow),indxCol);
      currLoop = child::currLoop;
      print("unfinished"+&stringDelimitList(List.map(List.flatten(unfinLoops),intString),",")+&"\n");
        loopsOut = findLoops2(aIn,atIn,finLoops,unfinLoops,currLoop,child,1);
        then loopsOut; 
     end matchcontinue;
   end findLoops2;   
 
  protected function deleteOrEmpty " deletes the indexed position(index starts at 0), or if list is empty gives empty list back.
author: Waurich TUD 2013-01"
  input list<list<Integer>> lstIn;
  input Integer indx;
  output list<list<Integer>> lstOut;
 algorithm
   lstOut := matchcontinue(lstIn,indx)
     case({},_)
       then {};
     else
       equation
         lstOut = listDelete(lstIn,indx);
         then lstOut;
   end matchcontinue;
 end deleteOrEmpty;             
        
protected function firstOrEmpty " gives the first element of a list.If list is Empty returns given value.
author: Waurich TUD 2013-01"
    input list<Integer> lstIn;
    input Integer valIn;
    output Integer valOut;
  algorithm
    valOut := matchcontinue(lstIn,valIn)
    local
      case({},_)
        equation
          then valIn;
      else
        equation
          valOut = listGet(lstIn,1);
          then valOut;
   end matchcontinue;
 end firstOrEmpty;

  protected function traceMooreLoops "saves more divergent branches during findLoop
author: Waurich TUD 2012-12"
  input list<list<Integer>> unfinLoopsIn;
  input list<Integer> currLoop,row;
  input Integer indx;
  output list<list<Integer>> unfinLoopsOut;
algorithm
  unfinLoopsOut := matchcontinue(unfinLoopsIn,currLoop,row,indx)
  local
    list<Integer> newLoop;
    list<list<Integer>>unfinished;
    Integer child;
  case(_,_,_,_)
    equation
      true = listLength(row)>= indx;
      child = listGet(row,indx);
      //print("child"+&intString(child)+&"\n");
      newLoop = child::currLoop;
      unfinished = newLoop::unfinLoopsIn;
      //print("next unfinished"+&stringDelimitList(List.map(newLoop,intString),",")+&"\n");
        unfinLoopsOut = traceMooreLoops(unfinished,currLoop,row,indx+1);
        then unfinLoopsOut;
    case(_,_,_,_)
      equation
        true = listLength(row) < indx;
        then
          unfinLoopsIn;
   end matchcontinue;
 end traceMooreLoops;

  protected function TearingSystemOlleroAmselem "assigns vars to eqs 
with StewardAssignment, selectes tearingSet from digraph according
to Ollero-Amselem alorithm.
author: Waurich TUD 2012-12"
  input BackendDAE.IncidenceMatrix m;
  input BackendDAE.IncidenceMatrixT mt;
  input list<Integer> ass1In,ass2In;
  output list<Integer> OutTVars,ass1Out,ass2Out; 
  list<Integer> essSet;
  array<list<Integer>> a,at,s,st;
algorithm
  (ass1Out,ass2Out) := AssignmentSteward(m,mt,ass1In,ass2In,1);
  print("ass2"+&stringDelimitList(List.map(ass2Out,intString),",")+&"\n");
  print("ass1"+&stringDelimitList(List.map(ass1Out,intString),",")+&"\n");
  (a,at) := getAdjacencyMatrix(m,mt,ass1Out,ass2Out);
  print("a\n");
  BackendDump.dumpIncidenceMatrix(a);
  print("at\n");
  BackendDump.dumpIncidenceMatrix(at);
  (s,st) := getStreamAdjacencyMatrix(a,at);
  print("s\n");
  BackendDump.dumpIncidenceMatrix(s);
  print("st\n");
  BackendDump.dumpIncidenceMatrix(st);
  essSet := getEssentialSet(s,st);
  print("essential set"+&stringDelimitList(List.map(essSet,intString),",")+&"\n");
  OutTVars := convertToTearingSet(essSet,ass1Out,{},a,1);
end TearingSystemOlleroAmselem;

protected function convertToTearingSet " converts essentialSet to TearingSet
author:Waurich TUD 2012-11"
 input list<Integer> essSet,ass1,vars;
 input array<list<Integer>> a;
 input Integer indx;
 output list<Integer> tVars;
 algorithm
   tVars := matchcontinue(essSet,ass1,vars,a,indx)
   local
     Integer edge,vertex,var;
     list<Integer> vertices,lst;
     case(_,_,_,_,_)
       equation
         true = indx > listLength(essSet);
           vars = List.unique(vars);
         then
           vars;     
     case(_,_,_,_,_)
       equation
         true = indx <= listLength(essSet);
         edge = listGet(essSet,indx);
         (_,lst) = getNumberOfEntries(a);
         vertex = List.getMemberOnTrue(edge,lst,intLe);
         vertex = List.position(vertex,lst)+1;
         var = listGet(ass1,vertex);   
         vars = var::vars;
         vars = convertToTearingSet(essSet,ass1,vars,a,indx+1);       
         then
           vars;
   end matchcontinue;
 end convertToTearingSet;

protected function getEssentialSet "selects from the stream 
adjacency matrix the essential set.
author: Waurich TUD 2012-11"
  input array<list<Integer>> sIn,stIn;
  output list<Integer> essSet;
algorithm
    essSet := getEssentialSet2(sIn,stIn,{},{},1,1);
end getEssentialSet;

protected function getEssentialSet2 
"helper function for getEssentialSet.
author: Waurich TUD 2012-11"
  input array<list<Integer>> sIn,stIn;
  input list<Integer> set,probed;
  input Integer es,indxVert;
  output list<Integer> essSet;
algorithm
  essSet := matchcontinue(sIn,stIn,set,probed,es,indxVert)
  local
    Integer di,do,parent,child,vertex,num;
    list<Integer> set,parentLst,childLst,selfLoops;
    array<list<Integer>> s,st;
    case(_,_,_,_,_,_)
      equation
        true = listLength(probed) == arrayLength(sIn);
        //print("graph is empty \n");
        then
          set;   
    case(_,_,_,_,_,_)
      equation
        true = indxVert > arrayLength(sIn);
        //print("start with vertex 1 and raise es to  "+&intString(es+1)+&"\n");
        essSet = getEssentialSet2(sIn,stIn,set,probed,es+1,1);
        then essSet;                
    case(_,_,_,_,_,_)
      equation
        false = List.isMemberOnTrue(indxVert,probed,intEq);
        do = listLength(arrayGet(sIn,indxVert));
        di = listLength(arrayGet(stIn,indxVert));
        true = do == 0 or di == 0;
        //print("delete vertex "+&intString(indxVert)+&"\n");
        parentLst = arrayGet(stIn,indxVert);
        childLst = arrayGet(sIn,indxVert);       
        (s,st) = deleteInStreamAdjacency(sIn,stIn,parentLst,childLst,indxVert,1);
        probed = indxVert::probed;
        essSet = getEssentialSet2(s,st,set,probed,1,indxVert+1);
        then 
          essSet;
    case(_,_,_,_,_,_)
      equation
        selfLoops = checkSelfLoop(sIn);
        true = listLength(selfLoops)<> 0;
        vertex = listGet(selfLoops,1);
        //print("selfloop vertex "+&intString(vertex)+&"\n"); 
        set = vertex::set;
        parentLst = arrayGet(stIn,vertex);
        childLst = arrayGet(sIn,vertex);
        (s,st) = contractInStreamAdjacency(sIn,stIn,parentLst,childLst,vertex,1,1);          
        probed = vertex::probed;
        //print("s\n");
        //BackendDump.dumpIncidenceMatrix(s);
        essSet = getEssentialSet2(sIn,stIn,set,probed,1,indxVert);
      then
        essSet; 
    case(_,_,_,_,_,_)
      equation
        do = listLength(arrayGet(sIn,indxVert));
        di = listLength(arrayGet(stIn,indxVert));
        true = do == es or di == es;
        //print("contract vertex "+&intString(indxVert)+&"\n"); 
        parentLst = arrayGet(stIn,indxVert);
        childLst = arrayGet(sIn,indxVert);      
        (s,st) = contractInStreamAdjacency(sIn,stIn,parentLst,childLst,indxVert,1,1);
        probed = indxVert::probed;
        //print("s\n");
        //BackendDump.dumpIncidenceMatrix(s);
        essSet = getEssentialSet2(s,st,set,probed,es,indxVert+1);
        then 
          essSet;
      else
        equation
        //print("go to next vertex "+&intString(indxVert+1)+&"\n");
        essSet = getEssentialSet2(sIn,stIn,set,probed,es,indxVert+1); 
        then
          essSet;    
    end matchcontinue;
  end getEssentialSet2;
  
protected function checkSelfLoop" checks if there is a self-loop in the stream adjacency matrix.
author:Waurich TUD 2012-11"
  input array<list<Integer>> sIn;
  output list<Integer> selfLoops;
algorithm
  ((_,selfLoops)) := Util.arrayFold(sIn,checkSelfLoop2,((1,{})));
end checkSelfLoop;

protected function checkSelfLoop2" helper function for checkSelfLoop
author: Waurich TUD 2012-11"
  input list<Integer> row;
  input tuple<Integer,list<Integer>> inValue;
  output tuple<Integer,list<Integer>> outValue;
 algorithm
   outValue := matchcontinue(row,inValue)
     local
       Integer indx,value;
       list<Integer> lst;
     case(_,((indx,lst)))
       equation
         true = List.isMemberOnTrue(indx,row,intEq);
         value = List.getMember(indx,row);
         lst = value::lst;
         then
           ((indx+1,lst));
     case(_,((indx,lst)))
       equation
         false = List.isMemberOnTrue(indx,row,intEq);
         then
           ((indx+1,lst));
     end matchcontinue;
   end checkSelfLoop2;

protected function contractInStreamAdjacency " applies contraction on vertex in stream adjacency matrix.
author: Waurich TUD 2012-11"
    input array<list<Integer>> sIn,stIn;
    input list<Integer> parentLst,childLst;
    input Integer vertex,indxParent,indxChild;
    output array<list<Integer>> sOut,stOut;
  algorithm
    (sOut,stOut) := matchcontinue(sIn,stIn,parentLst,childLst,vertex,indxParent,indxChild)
    local
      Integer parent,child;
      list<Integer> row;
    case(_,_,_,_,_,_,_)
      equation    
        true = indxParent <= listLength(parentLst);
        true = indxChild <= listLength(childLst);
        child = listGet(childLst,indxChild);
        parent = listGet(parentLst,indxParent);
        row = arrayGet(sIn,parent);
        row = child::row;
        row = List.deleteMember(row,vertex);
        row = List.unique(row);
        sOut = Util.arrayReplaceAtWithFill(parent,row,{},sIn);
        row = arrayGet(stIn,child);
        row = parent::row;
        row = List.deleteMember(row,vertex);
        row = List.unique(row);
        stOut = Util.arrayReplaceAtWithFill(child,row,{},stIn);
        sOut = Util.arrayReplaceAtWithFill(vertex,{},{},sOut);
        stOut = Util.arrayReplaceAtWithFill(vertex,{},{},stOut);
        (sOut,stOut) = contractInStreamAdjacency(sOut,stOut,parentLst,childLst,vertex,indxParent,indxChild+1);
        then
          (sOut,stOut);
    case(_,_,_,_,_,_,_)
      equation    
        true = indxParent <= listLength(parentLst);
        true = indxChild > listLength(childLst);
        (sOut,stOut) = contractInStreamAdjacency(sIn,stIn,parentLst,childLst,vertex,indxParent+1,1);
        then
          (sOut,stOut);
    case(_,_,_,_,_,_,_)
      equation    
        true = indxParent > listLength(parentLst);
        then
          (sIn,stIn);
    end matchcontinue;
  end contractInStreamAdjacency;
        
 protected function deleteInStreamAdjacency " updates the stream adjacency matrix 
 after deletion of a vertex.
 author: Waurich TUD 2012-11"
 input array<list<Integer>> sIn,stIn;
 input list<Integer> parentLst,childLst;
 input Integer vertex,indx;
 output array<list<Integer>> sOut,stOut;
 algorithm
   (sOut,stOut) := matchcontinue(sIn,stIn,parentLst,childLst,vertex,indx)
   local
     Integer parent,child;
     list<Integer> row;       
     case(_,_,_,_,_,_)
       equation
         true = listLength(parentLst) == 0;
         true = indx <= listLength(childLst);
         child = listGet(childLst,indx);
         sOut = Util.arrayReplaceAtWithFill(vertex,{},{},sIn);
         row = arrayGet(stIn,child);
         row = List.deleteMember(row,vertex);
         stOut = Util.arrayReplaceAtWithFill(child,row,{},stIn);
         (sOut,stOut) = deleteInStreamAdjacency(stOut,stOut,parentLst,childLst,vertex,indx+1);
         then 
           (sOut,stOut);
     case(_,_,_,_,_,_)
       equation
         true = listLength(parentLst) == 0;
         true = indx > listLength(childLst);
         then 
           (sIn,stIn);
     case(_,_,_,_,_,_)
       equation
         true = listLength(childLst) == 0;
         true = indx <= listLength(parentLst);
         parent = listGet(parentLst,indx);
         stOut = Util.arrayReplaceAtWithFill(vertex,{},{},stIn);
         row = arrayGet(sIn,parent);
         row = List.deleteMember(row,vertex);
         sOut = Util.arrayReplaceAtWithFill(parent,row,{},sIn);
         (sOut,stOut) = deleteInStreamAdjacency(stOut,stOut,parentLst,childLst,vertex,indx+1);
         then 
           (sOut,stOut);
     case(_,_,_,_,_,_)
       equation
         true = listLength(childLst) == 0;
         true = indx > listLength(parentLst);
         then 
           (sIn,stIn);
     end matchcontinue;
   end deleteInStreamAdjacency;

protected function AssignmentSteward " assigns outputvars to equations according to Stewards Algorithm.
 assignments gives the index of the currently assigned equation for the variables.
 author:Waurich TUD 2012-11"
  input BackendDAE.IncidenceMatrix m;
  input BackendDAE.IncidenceMatrixT mt;
  input list<Integer> ass1In,ass2In;
  input Integer indxVar;
  output list<Integer> ass1Out,ass2Out;
algorithm
  (ass1Out,ass2Out) := 
    matchcontinue(m,mt,ass1In,ass2In,indxVar)
      local
        Integer Var,Eq;
        list<Integer> ass1,ass2,possibleEq;
      case(_,_,_,_,_)
        equation
        true = indxVar > listLength(ass1In);
        //print("assignment done\n");
        then
          (ass1In,ass2In);  
      case(_,_,_,_,_)
        equation
          true = indxVar <= listLength(ass1In); 
          Var = indxVar;
          possibleEq = arrayGet(mt,Var); 
          (_,_,possibleEq) = List.intersection1OnTrue(ass2In,possibleEq,intEq);
          true = listLength(possibleEq)>0;
          Eq = listGet(possibleEq,1);
          //print("assign \n");
          //print("Var"+&intString(Var)+&" Eq"+&intString(Eq)+&"\n");
          ass1 = List.set(ass1In,Eq,Var);
          ass2 = List.set(ass2In,Var,Eq);
          indxVar = getNextVarSteward(ass2);
          //print("ass1 "+&stringDelimitList(List.map(ass1,intString),",")+&"\n");
          //print("ass2 "+&stringDelimitList(List.map(ass2,intString),",")+&"\n");
          (ass1,ass2) = AssignmentSteward(m,mt,ass1,ass2,indxVar);
          then 
           (ass1,ass2);
      case(_,_,_,_,_)
        equation
          true = indxVar <= listLength(ass1In); 
          Var = indxVar;
          possibleEq = arrayGet(mt,Var); 
          (_,_,possibleEq) = List.intersection1OnTrue(ass2In,possibleEq,intEq);
          true = listLength(possibleEq)==0;
          //print("reassign \n");
          Eq = listGet(arrayGet(mt,Var),1);
          //print("1\n");
          indxVar = List.position(Eq,ass2In);
                    print("2\n");
          ass1 = List.set(ass1In,Eq,Var);
                    print("3\n");
          ass2 = List.set(ass2In,Var,Eq);  
                    print("4\n");          
          ass2 = List.set(ass2In,indxVar,-1);
          //print("Var"+&intString(Var)+&" Eq"+&intString(Eq)+&" ass1 "+&stringDelimitList(List.map(ass1,intString),",")+&"\n");
          //print("ass2 "+&stringDelimitList(List.map(ass2,intString),",")+&"\n");
          (ass1,ass2) = AssignmentSteward(m,mt,ass1,ass2,indxVar);
          then 
           (ass1,ass2);            
      end matchcontinue;
    end AssignmentSteward;        
  
protected function getNextVarSteward
  input list<Integer> ass2;
  output Integer indxVar;
algorithm
  indxVar := matchcontinue(ass2)
  case(_)
    equation
      true = List.isMemberOnTrue(-1,ass2,intEq);
      indxVar = List.position(-1,ass2)+1;
      then
        indxVar;
  case(_)
    equation
      false = List.isMemberOnTrue(-1,ass2,intEq);
      indxVar = listLength(ass2)+1;
      then
        indxVar;
  end matchcontinue;
end getNextVarSteward;

protected function getAdjacencyMatrix " builds an adjacencymatrix for 
the already matched system.
equation-index gives the vertexindex for the digraph.
author: Waurich TUD 2012-11"
  input BackendDAE.IncidenceMatrix m;
  input BackendDAE.IncidenceMatrixT mt;
  input list<Integer> ass1,ass2;
  output array<list<Integer>> a;
  output array<list<Integer>> at;
algorithm
  a := arrayCreate(listLength(ass1),{});
  at := arrayCreate(listLength(ass1),{});
  (a,at) := getAdjacencyMatrix2(m,mt,ass1,ass2,1,a,at);
end getAdjacencyMatrix;

protected function getAdjacencyMatrix2 "implementation for getAdjacencyMatrix
author:Waurich TUD 11-2012"
  input BackendDAE.IncidenceMatrix m;
  input BackendDAE.IncidenceMatrixT mt;
  input list<Integer> ass1,ass2;
  input Integer indx;
  input array<list<Integer>> a;
  input array<list<Integer>> at;
  output array<list<Integer>> aOut;
  output array<list<Integer>> atOut;
algorithm
  (aOut,atOut) := matchcontinue(m,mt,ass1,ass2,indx,a,at)
    local
      Integer assignedVar;
      list<Integer> incidentVars,parentVerts;      
  case(_,_,_,_,_,_,_)
    equation
    true = listLength(ass1) < indx;
    then
      (a,at);
  case(_,_,_,_,_,_,_)
    equation
    true = listLength(ass1) >= indx;
      incidentVars = arrayGet(m,indx);
      assignedVar = listGet(ass1,indx);
      incidentVars = List.deleteMember(incidentVars,assignedVar);
      parentVerts = selectFromList(ass2,incidentVars);
      a = arrayUpdate(a,indx,parentVerts);
      at = addColumnEntries(at,parentVerts,indx,1);
      (a,at) = getAdjacencyMatrix2(m,mt,ass1,ass2,indx+1,a,at);
    then
      (a,at);
  end matchcontinue;
end getAdjacencyMatrix2;    
    
protected function addColumnEntries " adds Entries to every list in array<list> given by indexes
author:Waurich TUD 2012-11"
  input array<list<Integer>> InArr;
  input list<Integer> indexes;
  input Integer entry,indx;
  output array<list<Integer>> OutArr;
algorithm
  OutArr := matchcontinue(InArr,indexes,entry,indx)
    local
      Integer rowIndx;
      list<Integer> row;
      array<list<Integer>> arr;
  case(_,_,_,_)
    equation
      true = listLength(indexes) >= indx;
        rowIndx = listGet(indexes,indx);
        row = arrayGet(InArr,rowIndx);
        row = entry::row;
        row = List.sort(row,intGt);
        arr = Util.arrayReplaceAtWithFill(rowIndx,row,{},InArr);
        arr = addColumnEntries(arr,indexes,entry,indx+1);
      then
        arr;
  case(_,_,_,_)
    equation
      true = listLength(indexes) < indx;
      then
        InArr;
  end matchcontinue;
end addColumnEntries; 

protected function getStreamAdjacencyMatrix "builds the StreamAdjacencyMatrix from the AdjacencyMatrix.
edges become vertices.
author:Waurich TUD 2012-11"
  input array<list<Integer>> a,at;
  output array<list<Integer>> s,st;
  Integer size;
  list<Integer> sumLst;
algorithm
  (size,sumLst) := getNumberOfEntries(a);
  s := arrayCreate(size,{});
  st := arrayCreate(size,{});  
  (s,st):= getStreamAdjacencyMatrix2(a,at,s,st,sumLst,1,1,1);
end getStreamAdjacencyMatrix;

protected function getStreamAdjacencyMatrix2"helper function for getStreamAdjacencyMatrix.
author:Waurich TUD 2012-11"
  input array<list<Integer>> a,at;
  input array<list<Integer>> sIn,stIn;
  input list<Integer> sumLst;  
  input Integer indxParent,indxChild1,indxChild2;
  output array<list<Integer>> sOut;
  output array<list<Integer>> stOut;
algorithm
  (sOut,stOut) := matchcontinue(a,at,sIn,stIn,sumLst,indxParent,indxChild1,indxChild2)
  local
    Integer numRow,numCol,parent,child1,child2;
    list<Integer> row;   
  case(_,_,_,_,_,_,_,_)
    equation 
      true = indxParent > arrayLength(a);
      then
        (sIn,stIn); 
  case(_,_,_,_,_,_,_,_)
    equation
      //true = indxChild1 <= (listGet(sumLst,indxParent+1)-listGet(sumLst,indxParent));
      true = indxChild1 <= listLength(arrayGet(a,indxParent));
      true = indxChild2 <= listLength(arrayGet(a,listGet(arrayGet(a,indxParent),indxChild1)));
      //print("set entry and raise child2indx\n");
      parent = indxParent;
      child1 = listGet(arrayGet(a,parent),indxChild1);
      child2 = listGet(arrayGet(a,child1),indxChild2);
      //print("parent"+&intString(parent)+&" child1"+&intString(child1)+&" child2"+&intString(child2)+&"\n");
      numRow = listGet(sumLst,parent)+indxChild1-1;
      numCol = listGet(sumLst,child1)+indxChild2-1;
      //print("row"+&intString(numRow)+&" col"+&intString(numCol)+&"\n");
      row = arrayGet(sIn,numRow);
      row = numCol::row;
      row = List.sort(row,intGt);
      sOut = Util.arrayReplaceAtWithFill(numRow,row,{},sIn);
      row = arrayGet(stIn,numCol);
      row = numRow::row;
      row = List.sort(row,intGt);
      stOut = Util.arrayReplaceAtWithFill(numCol,row,{},stIn);
      (sOut,stOut) = getStreamAdjacencyMatrix2(a,at,sIn,stIn,sumLst,indxParent,indxChild1,indxChild2+1);
      then
        (sOut,stOut);
  case(_,_,_,_,_,_,_,_)
    equation
      true = indxChild1 <= listLength(arrayGet(a,indxParent));
      true = indxChild2 > listLength(arrayGet(a,listGet(arrayGet(a,indxParent),indxChild1)));
      //print("raise child1indx\n");
      (sOut,stOut) = getStreamAdjacencyMatrix2(a,at,sIn,stIn,sumLst,indxParent,indxChild1+1,1);
      then
        (sOut,stOut);
  case(_,_,_,_,_,_,_,_)
    equation
      true = indxChild1 > listLength(arrayGet(a,indxParent));
      //print("raise parentindx\n");
      (sOut,stOut) = getStreamAdjacencyMatrix2(a,at,sIn,stIn,sumLst,indxParent+1,1,1);
      then
        (sOut,stOut);
  end matchcontinue;
end getStreamAdjacencyMatrix2;
  
protected function getNumberOfEntries "how many entries in an array<list<Integer>>
auhtor:Waurich TUD 2012-11"
  input array<list<Integer>> inArr;
  output Integer num;
  output list<Integer> lst;
algorithm
  lst := List.fill(-1,arrayLength(inArr));
  ((_,num,lst)) := Util.arrayFold(inArr,countEntries,((1,0,{})));
  //print("num"+&intString(num)+&"lst"+&stringDelimitList(List.map(lst,intString),",")+&"\n");
end getNumberOfEntries;

protected function countEntries " computes listLength and gives the total number and the row-wise summation.
author:Waurich TUD 2012-11"
  input list<Integer> lst;
  input tuple<Integer,Integer,list<Integer>> inValue;
  output tuple<Integer,Integer,list<Integer>> outValue;
algorithm
  ((indx,numOut,lstOut)):= matchcontinue(lst,inValue)
  local
    Integer length,numIn,numOut,indx;
    list<Integer> lstIn,lstOut;
  case(_,(indx,numIn,lstIn))
    equation      
      length = listLength(lst);
      numOut = numIn+length;
      lstOut = List.set(lstIn,indx,numIn+1);
      then
        ((indx+1,numOut,lstOut));
  end matchcontinue;          
end countEntries;

protected function TearingSystemCellier " selects Tearing Set and assigns Vars
  author:Waurich TUD 2012-11"
  input Boolean causal;
  input BackendDAE.IncidenceMatrix mIn;
  input BackendDAE.IncidenceMatrixT mtIn;
  input BackendDAE.AdjacencyMatrixEnhanced meIn;
  input BackendDAE.AdjacencyMatrixTEnhanced meTIn;
  input list<Integer> ass1In,ass2In,unsolvables,tvarsIn;
  input list<list<Integer>> orderIn;  
  output list<Integer> OutTVars;
algorithm
  OutTVars := matchcontinue(causal,mIn,mtIn,meIn,meTIn,ass1In,ass2In,unsolvables,tvarsIn,orderIn)
  local    
    Integer tvar;
    list<Integer> ass1,ass2,tvars,unassigned;
    list<list<Integer>>order;
    BackendDAE.IncidenceMatrix m;  
    BackendDAE.IncidenceMatrixT mt;  
  case(true,_,_,_,_,_,_,_,_,_)
    equation
     then tvarsIn;
  case(false,_,_,_,_,_,_,_,_,_)
    equation
      // select tearing Var
      tvar = selectTearingVar(meIn,meTIn,mIn,mtIn,unsolvables,1);   
      print("tearingVar "+&intString(tvar)+&"\n");       
      tvars = tvar::tvarsIn;
      // remove tearing var from incidence matrix and transpose inc matrix
      m = updateIncidence(mIn,tvar,1);
      BackendDump.dumpIncidenceMatrix(m);
      mt = Util.arrayReplaceAtWithFill(tvar,{},{},mtIn);
      // assign vars to eqs until complete or partially causalisation(and restart algorithm)
      (ass1,ass2,m,mt,order,causal) = Tarjan(m,mt,ass1In,ass2In,tvars,orderIn,true); 
      print("ass1 "+&stringDelimitList(List.map(ass1,intString),",")+&"\n");  
      print("ass2 "+&stringDelimitList(List.map(ass2,intString),",")+&"\n");  
      print("order "+&stringDelimitList(List.map(List.flatten(order),intString),",")+&"\n"); 
        ((_,unassigned)) = List.fold(ass2,getUnassigned,(1,{}));
        ((_,unassigned)) = List.fold(ass1,getUnassigned,(1,{}));
        tvars = TearingSystemCellier(causal,m,mt,meIn,meTIn,ass1,ass2,unsolvables,tvars,order);
        then tvars;
    end matchcontinue;
  end TearingSystemCellier;  
  
protected function TearingSystemCarpanzano " selects Tearing Set and assigns Vars
  author:Waurich TUD 2012-11"
  input Boolean causal;
  input BackendDAE.IncidenceMatrix mIn;
  input BackendDAE.IncidenceMatrixT mtIn;
  input BackendDAE.AdjacencyMatrixEnhanced meIn;
  input BackendDAE.AdjacencyMatrixTEnhanced meTIn;
  input list<Integer> ass1In,ass2In,unsolvables,tvarsIn;
  input list<list<Integer>> orderIn;  
  output list<Integer> OutTVars;
algorithm
  OutTVars := matchcontinue(causal,mIn,mtIn,meIn,meTIn,ass1In,ass2In,unsolvables,tvarsIn,orderIn)
  local    
    Integer tvar;
    list<Integer> ass1,ass2,tvars,unassigned;
    list<list<Integer>>order;
    BackendDAE.IncidenceMatrix m;  
    BackendDAE.IncidenceMatrixT mt;  
    BackendDAE.AdjacencyMatrixEnhanced me;
    BackendDAE.AdjacencyMatrixTEnhanced met;
  case(true,_,_,_,_,_,_,_,_,_)
    equation
     then tvarsIn;
  case(false,_,_,_,_,_,_,_,_,_)
    equation
      // select tearing Var
      tvar = selectTearingVar(meIn,meTIn,mIn,mtIn,unsolvables,3);   
      print("tearingVar "+&intString(tvar)+&"\n");       
      tvars = tvar::tvarsIn;
      // remove tearing var from incidence matrix and transpose inc matrix, as well as from enhanced
      m = updateIncidence(mIn,tvar,1);
      mt = Util.arrayReplaceAtWithFill(tvar,{},{},mtIn);
      me = deleteEntriesFromEnhancedIncidence(meIn,tvar,1);
      met = Util.arrayReplaceAtWithFill(tvar,{},{},meTIn);        
      print("meT after tearingvar \n\n");
      BackendDump.dumpAdjacencyMatrixTEnhanced(met);
      // assign vars to eqs until complete or partially causalisation(and restart algorithm)
      (ass1,ass2,m,mt,order,causal) = Tarjan(m,mt,ass1In,ass2In,tvars,orderIn,true); 
      print("ass1 "+&stringDelimitList(List.map(ass1,intString),",")+&"\n");  
      print("ass2 "+&stringDelimitList(List.map(ass2,intString),",")+&"\n");  
      print("order "+&stringDelimitList(List.map(List.flatten(order),intString),",")+&"\n"); 
      // update enhances incidence matrix with respect to the assigned vars
      (me,met) = updateEnhancedIncidenceMatrixAssigned(me,met,ass1,ass2);
      print("meT after assignment \n\n");
      BackendDump.dumpAdjacencyMatrixTEnhanced(met);
      tvars = TearingSystemCarpanzano(causal,m,mt,me,met,ass1,ass2,unsolvables,tvars,order);
      then tvars;
  end matchcontinue;
end TearingSystemCarpanzano;     

protected function getUnassigned " finds the unassigned vars or eqs.combine with List.fold"
  input Integer assEntry;
  input tuple<Integer,list<Integer>> InValue;
  output tuple<Integer,list<Integer>> OutValue;
algorithm
OutValue := matchcontinue(assEntry,InValue)
  local
    Integer indx;
    list<Integer> lst;
  case(_,(indx,lst))
    equation
    true = intEq(assEntry,-1);
    then
      ((indx+1,indx::lst)); 
  case(_,(indx,lst))
    equation
    false = intEq(assEntry,-1);
    then
      ((indx+1,lst)); 
  end matchcontinue;
end getUnassigned;

protected function potentialsCellier" gets the potentials for the next tearing variable.
author: Waurich TUD 2012-11" 
  input BackendDAE.IncidenceMatrix m,mt;
  output list<Integer> potentials;
  list<Integer> selectedcols1,selectedcols2,selectedrows;
  BackendDAE.IncidenceMatrix mtsel,msel2,msel2t;  
algorithm
// Cellier heuristic
// 1. choose rows(eqs) with most nonzero entries and write the column indexes(vars) for nonzeros in a list
((_,selectedcols1)) := Util.arrayFold(m,findMostEntries,(0,{}));
selectedcols1 := List.unique(List.flatten(selectedcols1));
//print("1st " +& stringDelimitList(List.map(selectedcols1,intString),", ") +& "\n");  
// 2. gather these columns in a new array (reduced mt)
mtsel := Util.arraySelect(mt,selectedcols1);
// 3. choose rows (vars) with most nonzero entries and write the indexes in a list
((_,_,selectedcols2)) := Util.arrayFold(mtsel,findMostEntries2,(0,1,{}));
selectedcols2 := List.unique(selectedcols2); 
//print("2nd"+& stringDelimitList(List.map(selectedcols2,intString),",")+&"\n");
// 4. convert indexes from msel to indexes from mt
selectedcols1 := selectFromList(selectedcols1,selectedcols2);
//print("3rd"+& stringDelimitList(List.map(selectedcols1,intString),",")+&"\n");
// 5. select the rows(eqs) from m with exact two nonzeros 
((_,_,selectedrows)) := Util.arrayFold(m,findNEntries,(2,1,{}));
//print("4th"+& stringDelimitList(List.map(selectedrows,intString),",")+&"\n");
// 6. determine whiche possible Vars causalize how many eqs become causal and take the vars with the biggest number of causalizable eqs
msel2t := Util.arraySelect(mt,selectedcols1);
potentials := selectCausalVars(selectedrows,selectedcols1,msel2t); 
potentials := selectFromList(selectedcols1,potentials);
//print("5th"+& stringDelimitList(List.map(potentials,intString),",")+&"\n");
end potentialsCellier;

protected function selectCausalVars " helper function for Cellier.
  if chosen vars causalize no equation, choose first of list
  author: Waurich TUD 2012-11"
  input list<Integer> selEqs,selVars;
  input BackendDAE.IncidenceMatrixT mt;
  output list<Integer> potentials;
algorithm
  potentials := matchcontinue(selEqs,selVars,mt)
  local
    list<Integer> Vars;
    case({},_,_)
      equation
        then
          {1};
    case(_,_,_)
      equation
        ((_,Vars,_,_)) = Util.arrayFold(mt,selectCausalVars2,(selEqs,{},0,1));          
        then 
          Vars;        
    end matchcontinue;
  end selectCausalVars;  

protected function selectCausalVars2" implementation of selectCausalVars.
matches causalizable equations with selected variables.
author: Waurich TUD 2012-11"
    input list<Integer> row;
    input tuple<list<Integer>,list<Integer>,Integer,Integer> inValue;
    output tuple<list<Integer>,list<Integer>,Integer,Integer> OutValue;
  algorithm
    OutValue := matchcontinue(row,inValue)
  local
    list<Integer> Eqs,selEqs,cVars,interEqs;
    Integer size,num,indx;
    case(row,(selEqs,cVars,num,indx))
      equation
        Eqs = row;
        interEqs = List.intersectionOnTrue(Eqs,selEqs,intEq);
        size = listLength(interEqs);
        true = size < num;
      then ((selEqs,cVars,num,indx+1));
    case(row,(selEqs,cVars,num,indx))
      equation
        Eqs = row;
        interEqs = List.intersectionOnTrue(Eqs,selEqs,intEq);
        size = listLength(interEqs);
        true = size == num;
      then ((selEqs,indx::cVars,num,indx+1));
    case(row,(selEqs,cVars,num,indx))
      equation
        Eqs = row;
        interEqs = List.intersectionOnTrue(Eqs,selEqs,intEq);
        size = listLength(interEqs);
        true = size > num;
      then ((selEqs,{indx},size,indx+1));
    end matchcontinue;
  end selectCausalVars2;
 
  protected function selectTearingVar
  "Selects set of TearingVars referred to one of the following algorithms.
1 = Cellier
2 = 
3 = Carpanzano variant 2
author: Waurich TUD 2012-11"
input BackendDAE.AdjacencyMatrixEnhanced me;
input BackendDAE.AdjacencyMatrixTEnhanced meT;
input BackendDAE.IncidenceMatrix m;
input BackendDAE.IncidenceMatrixT mt;
input list<Integer> unsolvables;
input Integer algo;
output Integer OutTVars;
algorithm
  OutTVars := 
  matchcontinue(me,meT,m,mt,unsolvables,algo)
    local 
      list<Integer> potentials; 
    case(_,_,_,_,_,1)
      equation
      potentials = potentialsCellier(m,mt);
      print("potentials"+& stringDelimitList(List.map(potentials,intString),",")+&"\n"); 
      potentials = List.setDifference(potentials,unsolvables);  
      then listGet(potentials,1);
    case(_,_,_,_,_,3)
      equation
      potentials = potentialsCarpanzano(me,meT,m,mt,unsolvables,2);
      //potentials = List.setDifference(potentials,unsolvables);
      then listGet(potentials,1);
    else
      equation
      print("selecting tearing variable failed");
      then 
        fail();
   end matchcontinue;
 end selectTearingVar;

protected function potentialsCarpanzano
"selects potential tearing variables in accordance to one of the 3 proposed variants
author: Waurich TUD 2012-10"
  input BackendDAE.AdjacencyMatrixEnhanced me;
  input BackendDAE.AdjacencyMatrixTEnhanced meT;
  input BackendDAE.IncidenceMatrix m,mt;
  input list<Integer> unsolvables;
  input Integer variant;
  output list<Integer> potentials;
algorithm
  potentials := matchcontinue(me,meT,m,mt,unsolvables,variant)
    case(_,_,_,_,_,2)
      then Carpanzano2(me,meT,m,mt,unsolvables);
   else then {};  
  end matchcontinue;         
end potentialsCarpanzano;  

protected function Carpanzano2
"weights the vars and selects the vars with highest weight referred to Carpanzano 2
author: Waurich TUD 2012-10"
  input BackendDAE.AdjacencyMatrixEnhanced me;
  input BackendDAE.AdjacencyMatrixTEnhanced meT;
  input BackendDAE.IncidenceMatrix m,mt;
  input list<Integer> unsolvables;
  output list<Integer> potentials;
  list<Real> weights;
algorithm
  weights := arrayList(Util.arrayMap(meT,Carpanzano2help));
  print("weights_carpanzano"+& stringDelimitList(List.map(weights,realString), ",")+&"\n");
  potentials:=maxListReal(weights);
  print("potentials"+& stringDelimitList(List.map(potentials,intString), ",")+&"\n");
end Carpanzano2;
  
protected function maxListReal
  "function to find maximum Reals in inList and output a list with the indexes.
  author: Waurich TUD 2012-11"
    input list<Real> inList;
    output list<Integer> outList;
  algorithm
    ((_,_,outList)):= List.fold(inList,maxListRealhelp,(1,0.0,{}));
end maxListReal;

protected function maxListRealhelp
"helper function to maxListReal"
  input Real value;
  input tuple<Integer,Real,list<Integer>> inValue;
  output tuple<Integer,Real,list<Integer>> outValue;
algorithm
  outValue := 
    matchcontinue(value,inValue)
      local
        Integer indx;
        Real maxValue;
        list<Integer> ilst;
      case(_,(indx,maxValue,ilst))
        equation
          true = value <. maxValue;
          then ((indx+1,maxValue,ilst));
      case(_,(indx,maxValue,ilst))
        equation
          true = realEq(value,maxValue);
          then ((indx+1,maxValue,indx::ilst));
      case(_,(indx,maxValue,ilst))
        equation
          true = realGt(value,maxValue);
          then ((indx+1,value,{indx}));
    end matchcontinue;
  end maxListRealhelp;

protected function maxListInt
  "function to find maximum Integers in inList and output a list with the indexes.
  author: Waurich TUD 2012-11"
    input list<Integer> inList;
    output list<Integer> outList;
  algorithm
    ((_,_,outList)):= List.fold(inList,maxListInthelp,(1,0,{}));
end maxListInt;

protected function maxListInthelp  "helper function to maxListInt.
author: Waurich TUD 2012-10"
  input Integer value;
  input tuple<Integer,Integer,list<Integer>> inValue;
  output tuple<Integer,Integer,list<Integer>> outValue;
algorithm
  outValue := 
    matchcontinue(value,inValue)
      local
        Integer indx;
        Integer maxValue;
        list<Integer> ilst;
      case(_,(indx,maxValue,ilst))
        equation
          true = value < maxValue;
          then ((indx+1,maxValue,ilst));
      case(_,(indx,maxValue,ilst))
        equation
          true = intEq(value,maxValue);
          then ((indx+1,maxValue,indx::ilst));
      case(_,(indx,maxValue,ilst))
        equation
          true = intGt(value,maxValue);
          then ((indx+1,value,{indx}));
    end matchcontinue;
  end maxListInthelp;
        
protected function Carpanzano2help" helper function for carpanzano2
author: Waurich TUD 2012-10"
  input BackendDAE.AdjacencyMatrixElementEnhanced row;
  output Real weight;
  Integer m,mb,mnb;
  algorithm
    weight := matchcontinue(row)
  case(_)
    equation
    m = listLength(row);
    false = m == 0;
    //print("m"+&intString(m)+&"\n");
    mnb = listLength(List.select(row,checkUnsolvable));
    //print("mnb"+&intString(mnb)+&"\n");
    mb = intSub(m,mnb);
   // print("mb"+&intString(mb)+&"\n");
    weight = realAdd(realMul(realAdd(1.0,realDiv(1.0,intReal(m))),intReal(mnb)),intReal(mb));
   // print("m"+&intString(m)+&" mnb"+&intString(mnb)+&" mb"+&intString(mb)+&" weight"+&realString(weight)+&"\n");
      then
      weight;
    else
      then
        0.0;
    end matchcontinue;
  end Carpanzano2help;    

protected function checkUnsolvable"checks if tuple in enhanced incidence has a unsolvable"
  input tuple<Integer,BackendDAE.Solvability> tup;
  output Boolean out;
algorithm
  out:= matchcontinue(tup)
    case((_,BackendDAE.SOLVABILITY_UNSOLVABLE))
      then true;
    else then false;
  end matchcontinue;
end checkUnsolvable;     

protected function selectFromList" selects Ints from inList by indexes given in selList
author: Waurich TUD 2012-11"
    input List<Integer> inList,selList;    
    output List<Integer> outList;
  algorithm
    outList := selectFromList_help(1,inList,selList,{});
  end selectFromList;
 
  protected function selectFromList_help " implementation for selectFromList.
auhtor: Waurich TUD 2012-11"
  input Integer indx;
  input List<Integer> inList,selList,lst;    
  output List<Integer> outList;
algorithm
  outList :=
  matchcontinue(indx,inList,selList,lst)
    local
      Integer actual,length,num;
    case(_,_,_,_)
      equation
        length = listLength(selList);
        num = listGet(selList,indx);
        actual = listGet(inList,num);
        true = indx <= length;            
      then 
        selectFromList_help(indx+1,inList,selList,actual::lst);
    else then lst;
  end matchcontinue;
end selectFromList_help;

protected function TarjanGetAssignable " selects assignable Var and Equation.
  author: Waurich TUD 2012-11"
  input BackendDAE.IncidenceMatrix m;
  input BackendDAE.IncidenceMatrixT mt;
  input list<Integer> assEq,assVar;
  input list<list<Integer>> orderIn;
  output Integer eqOut,varOut;
  output list<list<Integer>> orderOut;
  output Boolean assignable;
algorithm
  (eqOut,varOut,orderOut,assignable) := matchcontinue(m,mt,assEq,assVar,orderIn)
  local 
    Integer eq,var,pos;
    list<Integer> order;
    case(_,_,_,_,_)
      equation
        true = listLength(assEq) > 0;
        print("assign from m\n");
        eq = listGet(assEq,1);
        var = listGet(arrayGet(m,eq),1);
        order = listGet(orderIn,1);
        order = eq::order;
        orderOut = List.replaceAt(order,0,orderIn);
      then (eq,var,orderOut,true);
    case(_,_,_,_,_)
      equation
        true = listLength(assEq) == 0;
        true = listLength(assVar) > 0;
        print("assign from mt\n");
        var = listGet(assVar,1);
        eq = listGet(arrayGet(mt,var),1);
        order = listGet(orderIn,2);
        order = eq::order;
        orderOut = List.replaceAt(order,1,orderIn);         
      then (eq,var,orderOut,true);
    else
      then (0,0,orderIn,false);
  end matchcontinue;
end TarjanGetAssignable;

protected function TarjanAssignment"
author:Waurich TUD 2012-11"
  input BackendDAE.IncidenceMatrix mIn;
  input BackendDAE.IncidenceMatrixT mtIn;
  input list<Integer> ass1In,ass2In,tvarsIn;
  input list<list<Integer>> orderIn;
  output list<Integer> ass1Out,ass2Out;
  output BackendDAE.IncidenceMatrix mOut;
  output BackendDAE.IncidenceMatrixT mtOut;
  output list<list<Integer>> orderOut;
  output Boolean assignable;
  list<Integer> assEq,assVar,tvars;
  Integer eq,var,indx;    
  list<Integer> markVar,markEq;
algorithm
  ((_,_,assEq)) := Util.arrayFold(mIn,findNEntries,(1,1,{}));
  ((_,_,assVar)) := Util.arrayFold(mtIn,findNEntries,(1,1,{}));
  markVar := List.unique(ass1In);
  markEq := List.unique(ass2In);
  (_,assEq,_) := List.intersection1OnTrue(assEq,markEq,intEq);
  print("assEq"+&stringDelimitList(List.map(assEq,intString),",")+&"\n");   
  (_,assVar,_) := List.intersection1OnTrue(assVar,markVar,intEq);      
  print("assVar"+&stringDelimitList(List.map(assVar,intString),",")+&"\n");   
  (eq,var,orderOut,assignable) := TarjanGetAssignable(mIn,mtIn,assEq,assVar,orderIn);
  print("assignment"+&intString(eq)+&"-"+&intString(var)+&"\n");
  print("order"+&stringDelimitList(List.map(listGet(orderOut,1),intString),",")+&"\n"); 
  mOut := updateIncidence(mIn,var,1); 
  mtOut := updateIncidence(mtIn,eq,1);
  ass1Out := replaceAt(var,eq-1,ass1In);
  ass2Out := replaceAt(eq,var-1,ass2In);
end TarjanAssignment;
  
protected function Tarjan"Tarjan assignment.
author:Waurich TUD 2012-11"
  input BackendDAE.IncidenceMatrix mIn;
  input BackendDAE.IncidenceMatrixT mtIn;
  input list<Integer> ass1In,ass2In,tvarsIn;
  input list<list<Integer>> orderIn;
  input Boolean assignable;
  output list<Integer> ass1Out,ass2Out;
  output BackendDAE.IncidenceMatrix mOut;
  output BackendDAE.IncidenceMatrixT mtOut;    
  output list<list<Integer>> orderOut;
  output Boolean causal;
algorithm
   (ass1Out,ass2Out,mOut,mtOut,orderOut,causal):= matchcontinue(mIn,mtIn,ass1In,ass2In,tvarsIn,orderIn,assignable)
   local
     list<Integer> ass1,ass2,subOrder;
     list<list<Integer>> order;
     BackendDAE.IncidenceMatrix m;
     BackendDAE.IncidenceMatrixT mt;
     Boolean ass;
   case(_,_,_,_,_,_,false)   
     equation
       false = listLength(List.flatten(orderIn)) == (listLength(ass1In)-listLength(tvarsIn));
       print("noncausal\n");
     then (ass1In,ass2In,mIn,mtIn,orderIn,false);
   case(_,_,_,_,_,_,false)
     equation
     true = listLength(List.flatten(orderIn)) == (listLength(ass1In)-listLength(tvarsIn));
     print("causal\n");
     subOrder = listGet(orderIn,1);
     subOrder = listReverse(subOrder);
     order = List.deletePositions(orderIn,{0});
     orderOut = subOrder::order;
     then (ass1In,ass2In,mIn,mtIn,orderOut,true);
   case(_,_,_,_,_,_,true)
     equation
       (ass1,ass2,m,mt,order,ass) = TarjanAssignment(mIn,mtIn,ass1In,ass2In,tvarsIn,orderIn);
       //print("ass1 "+&stringDelimitList(List.map(ass1,intString),",")+&"\n");
       //print("ass2 "+&stringDelimitList(List.map(ass2,intString),",")+&"\n");       
         (ass1Out,ass2Out,mOut,mtOut,orderOut,causal)= Tarjan(m,mt,ass1,ass2,tvarsIn,order,ass);
       then (ass1Out,ass2Out,mOut,mtOut,orderOut,causal);  
     end matchcontinue;
   end Tarjan;    
 
  protected function replaceAt "replaces entry at position in given list by given value
  author:Waurich TUD 2012-11"
  input Integer inElement;
  input Integer inPosition;
  input list<Integer> inList;
  output list<Integer> outList;
algorithm
  outList := match(inElement, inPosition, inList)
    local
      Integer e;
      list<Integer> rest; 
    case (_,-1, e :: rest) then inList;
    case (_, 0, e :: rest) then inElement :: rest;
    case (_, _, e :: rest)
      equation
        (inPosition >= 1) = true;
        rest = replaceAt(inElement, inPosition - 1, rest);
      then
        e :: rest;
  end match;
end replaceAt;    

protected function updateEnhancedIncidenceMatrixAssigned" deletes the assigned vars and eqs from me and met
author: Waurich TUD 2012-12"
  input BackendDAE.AdjacencyMatrixEnhanced meIn;
  input BackendDAE.AdjacencyMatrixTEnhanced metIn;
  input list<Integer> ass1,ass2;
  output BackendDAE.AdjacencyMatrixEnhanced meOut;
  output BackendDAE.AdjacencyMatrixTEnhanced metOut;
  Integer sizeEqs,sizeVars;
  list<Integer> eqs,vars;
algorithm
  eqs := List.unique(ass2);
  eqs := List.removeOnTrue(-1,intEq,eqs);
  //eqs := List.map(eqs,addOne);
  sizeEqs := listLength(eqs);
  vars := List.unique(ass1);
  vars := List.removeOnTrue(-1,intEq,vars);
  //vars := List.map(vars,addOne);
  sizeVars := listLength(vars);    
  metOut := deleteFromEnhancedIncidence(metIn,eqs,vars,sizeEqs,sizeVars,1);
  meOut := deleteFromEnhancedIncidence(meIn,vars,eqs,sizeVars,sizeEqs,1);
end updateEnhancedIncidenceMatrixAssigned;

protected function deleteFromEnhancedIncidence "deletes from enhanced
incidence matrix rowwise single entries and whole rows.
author: Waurich TUD 2012-12"
  input BackendDAE.AdjacencyMatrixEnhanced meIn;
  input list<Integer> entries,rows;
  input Integer sizeEnt,sizeRow,indx;
  output BackendDAE.AdjacencyMatrixEnhanced meOut;
algorithm
  meOut := matchcontinue(meIn,entries,rows,sizeEnt,sizeRow,indx)
  local
    Integer entry,row;
  case(_,_,_,_,_,_)
    equation
      true = indx <= sizeEnt and indx <= sizeRow;
      entry = listGet(entries,indx);
      row = listGet(rows,indx);
      meOut = Util.arrayReplaceAtWithFill(row,{},{},meIn);
      meOut = deleteEntriesFromEnhancedIncidence(meOut,entry,1);
      meOut = deleteFromEnhancedIncidence(meOut,entries,rows,sizeEnt,sizeRow,indx+1);
    then meOut;
  case(_,_,_,_,_,_)
    equation
      true = indx <= sizeEnt and indx > sizeRow;
      entry = listGet(entries,indx);
      meOut = deleteEntriesFromEnhancedIncidence(meIn,entry,1);
      meOut = deleteFromEnhancedIncidence(meOut,entries,rows,sizeEnt,sizeRow,indx+1);
    then meOut;
  case(_,_,_,_,_,_)
    equation
      true = indx > sizeEnt and indx <= sizeRow;
      row = listGet(rows,indx);
      meOut = Util.arrayReplaceAtWithFill(row,{},{},meIn);
      meOut = deleteFromEnhancedIncidence(meOut,entries,rows,sizeEnt,sizeRow,indx+1);
    then meOut;
  case(_,_,_,_,_,_)
    equation
      true = indx > sizeEnt and indx > sizeRow;
    then meIn;      
  end matchcontinue;
end deleteFromEnhancedIncidence;

protected function addOne "adds 1 to an Integer.
author: Waurich TUD 2012-12"    
  input Integer int1;
  output Integer int2;
algorithm
  int2 := intAdd(int1,1);
end addOne; 

protected function deleteEntriesFromEnhancedIncidence 
"deletes given entry (var) row-wise from the enhanced incidence 
matrix (AdjacencyMatricEnhanced)
auhtor: Waurich TUD 2012-12"
  input BackendDAE.AdjacencyMatrixEnhanced meIn;
  input Integer entry;
  input Integer indx;
  output BackendDAE.AdjacencyMatrixEnhanced meOut;
algorithm
  meOut := matchcontinue(meIn,entry,indx)
  local
    Integer size;
    list<tuple<Integer,BackendDAE.Solvability>> row;  
    BackendDAE.AdjacencyMatrixEnhanced me;
  case(_,_,_)
    equation
      size = arrayLength(meIn);
      true = indx > size;
      //print("ok1 indx"+&intString(indx)+&"\n");
      then meIn;
  case(_,_,_)
    equation
      size = arrayLength(meIn);
      true = indx <= size;
      //print("ok2 indx"+&intString(indx)+&"\n");
      row = arrayGet(meIn,indx);
      row = deleteIntMarkedTuple(row,entry,1);
      //print("ok3 indx"+&intString(indx)+&"\n");
      me = Util.arrayReplaceAtWithFill(indx,row,row,meIn);
    then deleteEntriesFromEnhancedIncidence(me,entry,indx+1);
  end matchcontinue;
end deleteEntriesFromEnhancedIncidence; 
         
protected function deleteIntMarkedTuple " deletes entry from a list<tuple>
if the tuple contains at 1st position the given Integer.
Used in updating EnhancedIncidenceMatrix.
author: Waurich TUD 12-2012" 
  input list<tuple<Integer,BackendDAE.Solvability>> lstIn;
  input Integer element,indx;
  output list<tuple<Integer,BackendDAE.Solvability>> lstOut;
algorithm
  lstOut := matchcontinue(lstIn,element,indx)
  local
    Integer entry,size;
  case(_,_,_)
    equation
      size = listLength(lstIn);
      true = indx <= size;
      ((entry,_)) = listGet(lstIn,indx);
      true = entry == element;
      //print("ready for deletion of "+&intString(entry)+&"listlength and indx"+&intString(listLength(lstIn))+&" --> "+&intString(indx)+&"\n");
      lstOut = listDelete(lstIn,indx-1);
      //print("delete entry "+&intString(indx)+&"\n");
      lstOut = deleteIntMarkedTuple(lstOut,element,size+1);
      then lstOut;
  case(_,_,_)
    equation
      size = listLength(lstIn);
      true = indx <= size;
      ((entry,_)) = listGet(lstIn,indx);
      false = entry == element;
      //print("check entry "+&intString(indx)+&"\n");
      lstOut = deleteIntMarkedTuple(lstIn,element,indx+1);
      then lstOut;
  case(_,_,_)
    equation
      size = listLength(lstIn);
      true = indx > size;
      then lstIn;
  end matchcontinue;
end deleteIntMarkedTuple;    
   
protected function updateIncidence "deletes given entry from Matrix, starts with row indx.
  applicable on Incidence and on transposed Incidence.
  author: Waurich 2012-11"
  input BackendDAE.IncidenceMatrix mIn;
  input Integer entry;
  input Integer indx;
  output BackendDAE.IncidenceMatrix mOut;
algorithm
  mOut := matchcontinue(mIn,entry,indx)
  local
    Integer size;
    list<Integer> row;
    BackendDAE.IncidenceMatrix m;
  case(_,_,_)
  equation
    size = arrayLength(mIn);
    true = indx>size;
  then mIn;
  case(_,_,_)
    equation
      size = arrayLength(mIn);
      true = indx <= size;
      row = arrayGet(mIn,indx);
      row = List.deleteMember(row,entry);
      m = Util.arrayReplaceAtWithFill(indx,row,row,mIn);
    then updateIncidence(m,entry,indx+1);
  end matchcontinue;
end updateIncidence;

protected function findMostEntries "find rows with most nonzero 
elements and put the indexes of the columns with nonzeros in a list.
the first integer gives the max number of nonzero elements found.
author: Waurich TUD 2012-10"
  input list<Integer> row;
  input tuple<Integer,list<list<Integer>>> inValue;
  output tuple<Integer,list<list<Integer>>> outValue;
algorithm        
  outValue:=
  matchcontinue(row,inValue)
    local
      Integer length,length1;
      list<list<Integer>> ilst;
    case(_,(length,ilst))
      equation
        length1 = listLength(row);          
        true = length1 > length;
      then
        ((length1,{row}));            
    case(_,(length,ilst))
      equation
        length1 = listLength(row);
        true = intEq(length1,length);
      then
        ((length1,row::ilst));            
    else then inValue;            
  end matchcontinue;
end findMostEntries;    

protected function findMostEntries2 "find rows with most nonzero
elements and put the indexes of these rows in a list.
author: Waurich TUD 2012-10"
 input list<Integer> row;
 input tuple<Integer,Integer,list<Integer>> inValue;
 output tuple<Integer,Integer,list<Integer>> outValue;
algorithm
  outValue :=
  matchcontinue(row,inValue)
    local
      Integer length,length1,indx;
      list<Integer> ilst;        
    case(_,(length,indx,ilst))
      equation
        length1 = listLength(row);
        true = length1 > length;
      then
        ((length1,indx+1,{indx}));
    case(_,(length,indx,ilst))
      equation
        length1 = listLength(row);
        true = intEq(length1,length);
      then
        ((length,indx+1,indx::ilst));
    case(_,(length,indx,ilst))
      equation
        length1 = listLength(row);
        true = length1 < length;
      then
        ((length,indx+1,ilst));
  end matchcontinue;
end findMostEntries2;   
        
protected function findNEntries " find rows with n nonzero elements and 
put the indexes of these rows in a list.
author: Waurich TUD 2012-10"
  input list<Integer> row;
  input tuple<Integer,Integer,list<Integer>> inValue; 
  output tuple<Integer,Integer,list<Integer>> outValue;       
algorithm
  outValue :=
  matchcontinue(row,inValue)        
    local
      Integer num,indx,length;
      list<Integer> ilst;        
    case(_,(num,indx,ilst))
      equation
      length = listLength(row);
      true = intEq(num,length);
      then ((num,indx+1,indx::ilst));
    case(_,(num,indx,ilst))
      equation
      length = listLength(row);
      true = num <> length;
      then ((num,indx+1,ilst));    
  end matchcontinue;
end findNEntries;    

end Tearing;