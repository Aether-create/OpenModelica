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

encapsulated package SCodeApplyRedeclare
" file:        SCodeApplyRedeclare.mo
  package:     SCodeApplyRedeclare
  description: SCode instantiation

  RCS: $Id: SCodeApplyRedeclare.mo 13614 2012-10-25 00:03:02Z perost $

  Prototype SCode transformation to SCode without:
  - redeclares 
  - modifiers
  enable with +d=scodeInstShortcut.
"

public import Absyn;
public import InstTypes;
public import SCode;
public import SCodeEnv;

protected import Debug;
protected import Flags;
protected import List;
protected import SCodeDump;
protected import System;
protected import Util;
protected import SCodeAnalyseRedeclare;

public type Binding = InstTypes.Binding;
public type Dimension = InstTypes.Dimension;
public type Element = SCode.Element;
public type Program = SCode.Program;
public type Env = SCodeEnv.Env;
public type Modifier = InstTypes.Modifier;
public type ParamType = InstTypes.ParamType;
public type Prefixes = InstTypes.Prefixes;
public type Prefix = InstTypes.Prefix;
public type Scope = Absyn.Within;
public type Item = SCodeEnv.Item;

protected type IScopes = SCodeAnalyseRedeclare.IScopes;
protected type IScope = SCodeAnalyseRedeclare.IScope;
protected type Infos = SCodeAnalyseRedeclare.Infos;
protected type Kind = SCodeAnalyseRedeclare.Kind;
 
protected constant Integer idx = 3;
protected constant Integer idxNames = 4;

protected uniontype Change
  record CLONE "clone the original class and apply changes"
    Integer i;
    Absyn.Path originalName;
    Absyn.Path newName;
    Changes changes;
    IScope s;
  end CLONE;
    
  record REPLACE "replace element with the new one"
    Integer i;
    Absyn.Path originalName;
    SCode.Element new;
    Changes changes;
    IScope s;
  end REPLACE;

end Change;

protected type Changes = list<Change>;
protected constant Changes emptyChanges = {};

public function translate
"translates a class to a class without redeclarations"
  input Absyn.Path inClassPath;
  input Env inEnv;
  input Program inProgram;
  output Program outProg;
algorithm
  outProg := matchcontinue(inClassPath, inEnv, inProgram)
    local 
      Changes changes;
      String name;
      Program classes;
      IScopes iscopes;

    // no redeclares
    case (_, _, _)
      equation
        {} = SCodeAnalyseRedeclare.analyse(inClassPath, inEnv);        
      then
        inProgram;
    
    // some redeclares
    case (_, _, _)
      equation
        (iscopes as _::_) = SCodeAnalyseRedeclare.analyse(inClassPath, inEnv);        
        changes = mkProgramChanges(iscopes, emptyChanges);
        changes = listReverse(changes);        
        // print("Changes length: " +& intString(countChanges(changes)) +& "\n");
        // print("Cloned classes: " +& intString(countCloneChanges(changes)) +& " / Replaced elements: " +& intString(countReplChanges(changes)) +& "\n");
        
        printChanges(changes, 0);        
        
        // print("Flattening changes ...\n");
        changes = flattenChanges(changes, {});        
        
        // printChanges(changes, 0);
        
        // print("Starting the new apply phase ...\n");
        classes = applyChangesToProgram(inProgram, InstTypes.EMPTY_PREFIX(NONE()), Absyn.TOP(), inClassPath, changes);
        // print("Done with the apply phase ...\n");          
      then
        classes;

    else
      equation
        true = Flags.isSet(Flags.FAILTRACE);
        name = Absyn.pathString(inClassPath);
        Debug.traceln("SCodeApplyRedeclare.translate failed on " +& name);
      then
        fail();

  end matchcontinue;
end translate;

protected function mkProgramChanges
"analyses the instantiation scopes IScopes and builds 
 a list of changes to be applied to the full program"
  input IScopes inIScopes;
  input Changes inChangesAcc;
  output Changes outChanges;
algorithm
  outChanges := matchcontinue(inIScopes, inChangesAcc)
    local
      Changes changes;
      IScope is;
      IScopes rest;
    
    // what we get here is just one scope
    case ({is}, _)
      equation
        changes = mkElementChanges(is, {}, NONE(), inChangesAcc);
      then
        changes;

    else
      equation
        true = Flags.isSet(Flags.FAILTRACE);
        Debug.traceln("SCodeApplyRedeclare.mkProgramChanges failed on scope: " +& SCodeAnalyseRedeclare.iScopeStr(List.first(inIScopes)) +& "\n");
      then
        fail();

  end matchcontinue;
end mkProgramChanges;

protected function mkElementChanges
""
  input IScope inIScope;
  input IScopes inParentIScopes;
  input Option<Absyn.Path> inNewName;
  input Changes inChangesAcc;
  output Changes outChanges;
algorithm
  outChanges := matchcontinue(inIScope, inParentIScopes, inNewName, inChangesAcc)
    local
      Changes changes;
      IScope is;
      IScopes rest, parts, parentScopes;
      Kind kind;
      Infos infos;
      Prefix prefix;
      Absyn.Path name;
    
    case (SCodeAnalyseRedeclare.IS(kind as SCodeAnalyseRedeclare.CL(name), infos, parts), _, _, _)
      equation
        parentScopes = inIScope::inParentIScopes;
        changes = mkClassChanges(kind, infos, parts, parentScopes, inNewName, inChangesAcc);
      then
        changes;
    
    case (SCodeAnalyseRedeclare.IS(kind as SCodeAnalyseRedeclare.CO(prefix,_), infos, parts), _, _, _)
      equation
        parentScopes = inIScope::inParentIScopes;
        changes = mkComponentChanges(kind, infos, parts, parentScopes, inChangesAcc);
      then
        changes;

    case (SCodeAnalyseRedeclare.IS(kind as SCodeAnalyseRedeclare.EX(name), infos, parts), _, _, _)
      equation
        parentScopes = inIScope::inParentIScopes;
        changes = mkExtendsChanges(kind, infos, parts, parentScopes, inChangesAcc);
      then
        changes;

  end matchcontinue;
end mkElementChanges;

protected function mkClassChanges
  input Kind inKind;
  input Infos inInfos;
  input IScopes inKids;
  input IScopes inParentIScopes;
  input Option<Absyn.Path> inNewName;
  input Changes inChangesAcc;
  output Changes outChanges;
algorithm
  outChanges := matchcontinue(inKind, inInfos, inKids, inParentIScopes, inNewName, inChangesAcc)
    local
      Changes changes;
      Change ch;
      IScopes parts, parentScopes;
      Kind kind;
      Infos infos;
      Element e;
      Absyn.Path n, np;
      String nn;
      Integer i, j;
      IScope derivedTarget, is;
      Absyn.Path p;
      Option<Absyn.ArrayDim> ad;
    
    // referenced class, clone it
    case (kind as SCodeAnalyseRedeclare.CL(n), infos, parts, _, _, _)
      equation
        true = SCodeAnalyseRedeclare.isReferenced(inParentIScopes);
        np = mkNewName(n, inParentIScopes, inNewName);
        is = List.first(inParentIScopes);
        i = System.tmpTickIndex(idx);
        
        changes = List.fold2(parts, mkElementChanges, inParentIScopes, NONE(), {});
        
        changes = CLONE(i, n, np, changes, is)::inChangesAcc;
      then
        changes;
        
    // local class derived, clone the derived child
    case (kind as SCodeAnalyseRedeclare.CL(n), infos, {derivedTarget}, _, _, _)
      equation
        true = SCodeAnalyseRedeclare.isLocal(inParentIScopes);
        e = SCodeAnalyseRedeclare.getElementFromInfos(infos);
        true = SCode.isDerivedClass(e);
        parts = SCodeAnalyseRedeclare.iScopeParts(derivedTarget);
        parentScopes = derivedTarget::inParentIScopes;
        Absyn.TPATH(p, ad) = SCode.getDerivedTypeSpec(e);
        np = mkNewName(p, parentScopes, NONE());
        i = System.tmpTickIndex(idx);
        j = System.tmpTickIndex(idx);
        changes = List.fold2(parts, mkElementChanges, parentScopes, NONE(), {});
        is = List.first(parentScopes);
        ch = CLONE(j, p, np, changes, is);
        e = SCode.setDerivedTypeSpec(e, Absyn.TPATH(np, ad));
        is = List.first(inParentIScopes);
        changes = REPLACE(i, n, e, {ch}, is)::inChangesAcc;
      then
        changes;

    // local class not derived, replace it
    case (kind as SCodeAnalyseRedeclare.CL(n), infos, parts, _, _, _)
      equation
        true = SCodeAnalyseRedeclare.isLocal(inParentIScopes);
        e = mkNewElement(kind, infos, parts, inParentIScopes);
        is = List.first(inParentIScopes);
        i = System.tmpTickIndex(idx);
        
        changes = List.fold2(parts, mkElementChanges, inParentIScopes, NONE(), {});
        
        changes = REPLACE(i, n, e, changes, is)::inChangesAcc;
      then
        changes;

  end matchcontinue;
end mkClassChanges;

protected function mkComponentChanges
  input Kind inKind;
  input Infos inInfos;
  input IScopes inKids;
  input IScopes inParentIScopes;
  input Changes inChangesAcc;
  output Changes outChanges;
algorithm
  outChanges := matchcontinue(inKind, inInfos, inKids, inParentIScopes, inChangesAcc)
    local
      Changes changes;
      IScopes parts;
      Kind kind;
      Infos infos;
      Element e;
      Integer i;
      Absyn.Path n, np, fp;
      String nn, on;
      Option<Absyn.ArrayDim> ad;
      IScope is;
    
    // component with no parts, no name changes!
    case (kind, infos, {}, _, _)
      equation 
        e = mkNewElement(kind, infos, {}, inParentIScopes);
        on = SCode.getElementName(e);
        is = List.first(inParentIScopes);
        i = System.tmpTickIndex(idx);
        changes = REPLACE(i, Absyn.IDENT(on), e, {}, is)::inChangesAcc;
      then
        changes;    
    
    case (kind, infos, parts, _, _)
      equation 
        e = mkNewElement(kind, infos, parts, inParentIScopes);
        on = SCode.getElementName(e);
        Absyn.TPATH(n, ad) = SCode.getComponentTypeSpec(e);
        np = mkNewName(n, inParentIScopes, NONE());
        e = SCode.setComponentTypeSpec(e, Absyn.TPATH(np, ad));
        is = List.first(inParentIScopes);
        i = System.tmpTickIndex(idx);
        
        changes = List.fold2(parts, mkElementChanges, inParentIScopes, SOME(np), {});
        
        changes = REPLACE(i, Absyn.IDENT(on), e, changes, is)::inChangesAcc;
      then
        changes;
    
  end matchcontinue;
end mkComponentChanges;

protected function mkExtendsChanges
  input Kind inKind;
  input Infos inInfos;
  input IScopes inKids;
  input IScopes inParentIScopes;
  input Changes inChangesAcc;
  output Changes outChanges;
algorithm
  outChanges := matchcontinue(inKind, inInfos, inKids, inParentIScopes, inChangesAcc)
    local
      Changes changes;
      IScopes parts;
      Kind kind;
      Infos infos;
      Element e;
      Integer i;
      Absyn.Path n, np;
      String nn;
      IScope is;
        
    case (kind as SCodeAnalyseRedeclare.EX(n), infos, parts, _, _)
      equation
        e = mkNewElement(kind, infos, parts, inParentIScopes);
        np = mkNewName(n, inParentIScopes, NONE());
        e = SCode.setBaseClassPath(e, np);
        is = List.first(inParentIScopes);
        i = System.tmpTickIndex(idx);
        
        changes = List.fold2(parts, mkElementChanges, inParentIScopes, SOME(np), {});
                
        changes = REPLACE(i, n, e, changes, is)::inChangesAcc;
      then
        changes;
    
  end matchcontinue;
end mkExtendsChanges;

protected function mkNewElement
  input Kind inKind;
  input Infos inInfos;
  input IScopes inKids;
  input IScopes inParentIScopes;
  output Element outElement;
algorithm
  outElement := matchcontinue(inKind, inInfos, inKids, inParentIScopes)
    local
      Element e;
    
    case (_, _, _, _)
      equation
        e = SCodeAnalyseRedeclare.getElementFromInfos(inInfos);
        e = SCodeAnalyseRedeclare.removeRedeclareMods(e);
      then 
        e;
    
    else
      equation
        print("SCodeApplyRedeclare.mkNewElement failed on: " +& SCodeAnalyseRedeclare.infosStr(inInfos) +& "\n");
      then
        fail();
  end matchcontinue;
end mkNewElement;

protected function flattenChanges
"migrates all clone trees at the top level."
  input Changes inChanges;
  input Changes inChangesAcc;
  output Changes outChanges;
algorithm
  (outChanges, _) := splitChanges(inChanges, {}, {});
end flattenChanges;

protected function splitChanges
"split into two"
  input Changes inChanges;
  input Changes inClonesAcc;
  input Changes inReplsAcc;
  output Changes outClones;
  output Changes outRepls;
algorithm
  (outClones, outRepls) := matchcontinue(inChanges, inClonesAcc, inReplsAcc)
    local
      Changes cl, rp, rest, changes;
      Change ch;
      Integer i;
      Absyn.Path on;
      Absyn.Path nn;
      IScope s;
      SCode.Element n;
    
    case ({}, _, _) then (inClonesAcc, inReplsAcc);
    
    case (CLONE(i, on, nn, changes, s)::rest, _, _)
      equation
        (cl, rp) = splitChanges(changes, inClonesAcc, {});
        (cl, rp) = splitChanges(rest, CLONE(i, on, nn, rp, s)::cl, inReplsAcc); 
      then
        (cl, rp);
    
    case (REPLACE(i, on, n, changes, s)::rest, _, _)
      equation
        (cl, rp) = splitChanges(changes, inClonesAcc, {});       
        (cl, rp) = splitChanges(rest, cl, REPLACE(i, on, n, rp, s)::inReplsAcc); 
      then
        (cl, rp);
    
  end matchcontinue;
end splitChanges;

protected function getClonesByScope
"return the matching clones by scope and all the rest"
  input Scope inScope;
  input Changes inChanges;
  input Changes inMatchingAcc;
  input Changes inRestAcc;
  output Changes outMatchingChanges;
  output Changes outRestChanges;
algorithm
  (outMatchingChanges, outRestChanges) := matchcontinue(inScope, inChanges, inMatchingAcc, inRestAcc)
    local
      Changes macc, racc, rest;
      Change ch;
      Integer i;
      Absyn.Path on;
      Absyn.Path nn;
      IScope s;
      SCode.Element n;
    
    case (_, {}, _, _) then (listReverse(inMatchingAcc), listReverse(inRestAcc));
    
    case (_, ch::rest, _, _)
      equation
        true = isClone(ch);
        true = sameScope(getChangeName(ch), inScope);
        (macc, racc) = getClonesByScope(inScope, rest, ch::inMatchingAcc, inRestAcc);
      then
        (macc, racc);
    
    case (_, ch::rest, _, _)
      equation
        (macc, racc) = getClonesByScope(inScope, rest, inMatchingAcc, ch::inRestAcc);
      then
        (macc, racc);
    
  end matchcontinue;
end getClonesByScope;

protected function isClone
  input Change inChange;
  output Boolean yes;
algorithm
  yes := matchcontinue(inChange)
    case (CLONE(i = _)) then true;
    else false;
  end matchcontinue;
end isClone;

protected function isReplace
  input Change inChange;
  output Boolean yes;
algorithm
  yes := matchcontinue(inChange)
    case (REPLACE(i = _)) then true;
    else false;
  end matchcontinue;
end isReplace;

protected function getReplacementsByName
"return the matching replacement by name"
  input Absyn.Path inName;
  input Changes inChanges;
  input Changes inMatchingAcc;
  input Boolean inExactMatch "request exact match if true, otherwise suffix";
  output Changes outMatchingChanges;
algorithm
  outMatchingChanges := matchcontinue(inName, inChanges, inMatchingAcc, inExactMatch)
    local
      Changes macc, rest;
      Change ch;
      Integer i;
      Absyn.Path on;
      Absyn.Path nn;
      IScope s;
      SCode.Element n;
    
    case (_, {}, _, _) then listReverse(inMatchingAcc);
    
    case (_, ch::rest, _, true)
      equation
        true = isReplace(ch);
        true = Absyn.pathEqual(getChangeName(ch), inName);
        macc = getReplacementsByName(inName, rest, ch::inMatchingAcc, inExactMatch);
      then
        macc;
    
    case (_, ch::rest, _, false)
      equation
        true = isReplace(ch);
        true = Absyn.pathSuffixOf(inName, getChangeName(ch));
        macc = getReplacementsByName(inName, rest, ch::inMatchingAcc, inExactMatch);
      then
        macc;    
    
    case (_, ch::rest, _, _)
      equation
        macc = getReplacementsByName(inName, rest, inMatchingAcc, inExactMatch);
      then
        macc;
    
  end matchcontinue;
end getReplacementsByName;

protected function getChangeName
  input Change inChange;
  output Absyn.Path outName;
algorithm
  outName := matchcontinue(inChange)
    local Absyn.Path on;
    case (CLONE(originalName = on)) then on;
    case (REPLACE(originalName = on)) then on;
  end matchcontinue;
end getChangeName;

protected function printChanges
  input Changes inChanges;
  input Integer inIndent;
algorithm
  _ := matchcontinue(inChanges, inIndent)
    local
      Changes rest, changes;
      Integer i;
      String str, n;
      Absyn.Path on, nn;
      Element new;
      IScope s;
    
    case (_, _)
      equation
        false = Flags.isSet(Flags.SHOW_PROGRAM_CHANGES); 
      then ();    
    
    case ({},_) then ();
    
    case (CLONE(i, on, nn, changes, s)::rest, _)
      equation
        print(stringAppendList(List.fill(" ", inIndent)));
        {str, _} = SCodeAnalyseRedeclare.kindStr(SCodeAnalyseRedeclare.iScopeKind(s));
        str = "CLONE[" +& str +& "] ";
        print(str +& Absyn.pathLastIdent(on) +& " -> " +& Absyn.pathString(nn) +& " [" +& intString(i) +& "]\n"); 
        printChanges(changes, inIndent + 1);
        printChanges(rest, inIndent);
      then 
        ();

    case (REPLACE(i, on, new, changes, s)::rest, _)
      equation
        print(stringAppendList(List.fill(" ", inIndent)));
        {str, n} = SCodeAnalyseRedeclare.kindStr(SCodeAnalyseRedeclare.iScopeKind(s));
        str = "REPL[" +& str +& "] ";
        print(str +& n +& " -> " +& SCodeDump.shortElementStr(new) +& " [" +& intString(i) +& "]\n"); 
        printChanges(changes, inIndent + 1);
        printChanges(rest, inIndent);
      then 
        ();

  end matchcontinue;
end printChanges;

protected function applyChangesToProgram
  input Program inProgram;
  input Prefix inPrefix;
  input Scope inScope;
  input Absyn.Path inClassPath;
  input Changes inChanges;
  output Program outProgram;
algorithm
  outProgram := matchcontinue(inProgram, inPrefix, inScope, inClassPath, inChanges)
    local
      Program rest, newp, newels;
      Element e, newe;
    
    case ({}, _, _, _, _) then {};
    
    case (e::rest, _, _, _, _)
      equation
        newels = applyChangesToElement(e, inPrefix, inScope, inClassPath, inChanges);
        newp = applyChangesToProgram(rest, inPrefix, inScope, inClassPath, inChanges);
        newp = listAppend(newels, newp);
      then
        newp;
  
  end matchcontinue;
end applyChangesToProgram;

protected function applyChangesToElement
  input Element inElement;
  input Prefix inPrefix;
  input Scope inScope;  
  input Absyn.Path inClassPath;
  input Changes inChanges;
  output list<Element> outElements;
algorithm
  outElements := matchcontinue(inElement, inPrefix, inScope, inClassPath, inChanges)
    local
      Element e;
      String n;
      SCode.Prefixes p;
      SCode.Encapsulated ep;
      SCode.Partial pp;
      SCode.Restriction rp;
      SCode.ClassDef cd;
      Absyn.Info i;
      SCode.Attributes a;
      Absyn.TypeSpec t;
      SCode.Mod m;
      Option<SCode.Comment> cmt;
      Option<Absyn.Exp> cnd;
      SCode.Visibility v;
      Option<SCode.Annotation> ann;
      Prefix prefix;
      Scope scope;
      Absyn.Path bcp;
      Changes clones, changes;
      Program classes;
      Boolean b;
       
    // any other class
    case (SCode.CLASS(n, p, ep, pp, rp, cd, i), _, _, _, _)
      equation
        scope = openScope(n, inScope);
        (clones, changes) = getClonesByScope(scope, inChanges, {}, {});

        cd = applyChangesToClassDef(cd, inPrefix, scope, inClassPath, changes);
        e = SCode.CLASS(n, p, ep, pp, rp, cd, i);
        e = replaceClass(e, inPrefix, inScope, inClassPath, changes);
        (classes, b) = cloneAndChange(e, inPrefix, inScope, inClassPath, clones);
        classes = Util.if_(b, classes, e::classes);
      then
        classes;
        
    case (SCode.COMPONENT(n, p, a, t, m, cmt, cnd, i), _, _, _, _)
      equation
        REPLACE(new = e)::changes = getReplacementsByName(Absyn.IDENT(n), inChanges, {}, true);
      then
        {e};
    
    case (SCode.EXTENDS(bcp, v, m, ann, i), _, _, _, _)
      equation
        REPLACE(new = e)::changes = getReplacementsByName(bcp, inChanges, {}, false);
      then
        {e};
        
    case (e, _, _, _, _)
      equation
        //print("ignored: " +& SCodeDump.unparseElementStr(e) +& "\n"); 
      then
        {e}; 
  end matchcontinue;
end applyChangesToElement;

protected function replaceClass
  input Element inElement;
  input Prefix inPrefix;
  input Scope inScope;  
  input Absyn.Path inClassPath;
  input Changes inChanges;
  output Element outElement;
algorithm
  outElement := matchcontinue(inElement, inPrefix, inScope, inClassPath, inChanges)
    local
      Element e;
      String n;
      Changes changes;
      Change ch;
       
    // any other class
    case (SCode.CLASS(name = n), _, _, _, _)
      equation
        REPLACE(new = e)::changes = getReplacementsByName(Absyn.IDENT(n), inChanges, {}, false);
      then
        e;
    
    else inElement; 

  end matchcontinue;
end replaceClass;

protected function cloneAndChange
  input Element inElement;
  input Prefix inPrefix;
  input Scope inScope;  
  input Absyn.Path inClassPath;
  input Changes inChanges;
  output list<Element> outElements;
  output Boolean topClassReplaced;
algorithm
  (outElements, topClassReplaced) := matchcontinue(inElement, inPrefix, inScope, inClassPath, inChanges)
    local
      Element e;
      Changes rest, changes;
      Absyn.Path nn, on;
      SCode.Ident n;
      SCode.Program els, classes;
      Boolean b;

    case (e, _, _, _, {}) then ({}, false);

    // handle the top class that we should instantiate, do not clone, replace!
    case (e, _, _, _, CLONE(originalName = on, newName = nn, changes = changes)::rest)
      equation
        true = Absyn.pathEqual(on, inClassPath);
        els = applyChangesToElement(e, inPrefix, inScope, inClassPath, changes);
        (classes, _) = cloneAndChange(e, inPrefix, inScope, inClassPath, rest);
        classes = listAppend(els, classes);
      then
        (classes, true);

    case (e, _, _, _, CLONE(newName = nn, changes = changes)::rest)
      equation
        n = Absyn.pathLastIdent(nn);
        e = SCode.setClassName(n, e);
        els = applyChangesToElement(e, inPrefix, inScope, inClassPath, changes);
        (classes, b) = cloneAndChange(e, inPrefix, inScope, inClassPath, rest);
        classes = listAppend(els, classes);
      then
        (classes, b);
    
  end matchcontinue;
  
end cloneAndChange;

protected function applyChangesToClassDef
  input SCode.ClassDef inClassDef;
  input Prefix inPrefix;
  input Scope inScope;
  input Absyn.Path inClassPath;
  input Changes inChanges;
  output SCode.ClassDef outClassDef;
algorithm
  outClassDef := matchcontinue(inClassDef, inPrefix, inScope, inClassPath, inChanges)
    local
      list<Element> el;
      list<SCode.Equation> eq;
      list<SCode.Equation> ieq;
      list<SCode.AlgorithmSection> alg;
      list<SCode.AlgorithmSection> ialg;
      list<SCode.ConstraintSection> cs;
      list<Absyn.NamedArg> clsattr;
      Option<SCode.ExternalDecl> ed;
      list<SCode.Annotation> al;
      Option<SCode.Comment> cmt;
      SCode.ClassDef cd;
      Absyn.TypeSpec t;
      SCode.Mod m;
      SCode.Attributes a;
      Absyn.Path p;
      String n;
      Boolean b;
      Changes changes;
      list<Program> els;
      
    case (SCode.PARTS(el, eq, ieq, alg, ialg, cs, clsattr, ed, al, cmt), _, _, _, _)
      equation
        els = List.map4(el, applyChangesToElement, inPrefix, inScope, inClassPath, inChanges);
        el = List.flatten(els);
        cd = SCode.PARTS(el, eq, ieq, alg, ialg, cs, clsattr, ed, al, cmt);
      then
        cd;
        
    case (SCode.CLASS_EXTENDS(n, m, cd), _, _, _, _)
      equation
        cd = applyChangesToClassDef(cd, inPrefix, inScope, inClassPath, inChanges);
        cd = SCode.CLASS_EXTENDS(n, m, cd);
      then
        cd;

    case (SCode.DERIVED(t, m, a, cmt), _, _, _, _)
      equation
        cd = SCode.DERIVED(t, m, a, cmt);
      then
        cd;

    case (cd as SCode.ENUMERATION(enumLst = _), _, _, _, _)
      then
        cd;
        
    case (cd as SCode.OVERLOAD(pathLst = _), _, _, _, _)
      then
        cd;

    case (cd as SCode.PDER(functionPath = _), _, _, _, _)
      then
        cd;
  end matchcontinue;
end applyChangesToClassDef;

protected function sameScope
  input Absyn.Path inPath;
  input Scope inScope;
  output Boolean isSameScope;
algorithm
  isSameScope := matchcontinue(inPath, inScope)
    local Absyn.Path p1, p2;
    case (p1, Absyn.WITHIN(p2)) then Absyn.pathEqual(p1, p2);
    else false; 
  end matchcontinue;
end sameScope;

protected function openScope
  input String inName;
  input Scope inScope;
  output Scope outScope;
algorithm
  outScope := matchcontinue(inName, inScope)
    local
      Absyn.Path p;
      Scope w;
      String n;
    // top scope, add name
    case (n, Absyn.TOP()) then Absyn.WITHIN(Absyn.IDENT(n));
    case (n, Absyn.WITHIN(p))
      equation
        p = Absyn.joinPaths(p, Absyn.IDENT(n));
        w = Absyn.WITHIN(p);
      then
        w;
  end matchcontinue;
end openScope;

protected function scopeStr
  input Scope inScope;
  output String outStr;
algorithm
  outStr := matchcontinue(inScope)
    local
      Absyn.Path p;
      Scope w;
      String n;
    
    // top scope, add name
    case (Absyn.TOP()) then "";
    case (Absyn.WITHIN(p)) then Absyn.pathString(p);
    
  end matchcontinue;
end scopeStr;

protected function countChanges
  input Changes inChanges;
  output Integer outCount;
algorithm
  outCount := matchcontinue(inChanges)
    local
      Changes rest, changes;
      Integer n;
    
    case ({}) then 0;
    
    case (CLONE(changes = changes)::rest)
      equation
        n = 1 + countChanges(changes) + countChanges(rest);
      then 
        n;
    
    case (REPLACE(changes = changes)::rest)
      equation
        n = 1 + countChanges(changes) + countChanges(rest);
      then 
        n;
  end matchcontinue;
end countChanges;

protected function countCloneChanges
  input Changes inChanges;
  output Integer outCount;
algorithm
  outCount := matchcontinue(inChanges)
    local
      Changes rest, changes;
      Integer n;
    
    case ({}) then 0;
    
    case (CLONE(changes = changes)::rest)
      equation
        n = 1 + countCloneChanges(changes) + countCloneChanges(rest);
      then 
        n;
    
    case (REPLACE(changes = changes)::rest)
      equation
        n = countCloneChanges(changes) + countCloneChanges(rest);
      then 
        n;
  end matchcontinue;
end countCloneChanges;

protected function countReplChanges
  input Changes inChanges;
  output Integer outCount;
algorithm
  outCount := matchcontinue(inChanges)
    local
      Changes rest, changes;
      Integer n;
    
    case ({}) then 0;
    
    case (CLONE(changes = changes)::rest)
      equation
        n = countReplChanges(changes) + countReplChanges(rest);
      then 
        n;
    
    case (REPLACE(changes = changes)::rest)
      equation
        n = 1 + countReplChanges(changes) + countReplChanges(rest);
      then 
        n;
  end matchcontinue;
end countReplChanges;

/*
public function setElementName
  input Element inElement;
  input String inName;
  output Element outElement;
algorithm
  outElement := matchcontinue(inElement, inName)
    local
      SCode.Prefixes pf;
      SCode.Encapsulated ep;
      SCode.Partial pp;
      SCode.Restriction res;
      SCode.ClassDef cdef;
      Absyn.Info i;
      SCode.Attributes attr;
      SCode.Visibility v;
      Absyn.TypeSpec ty;
      SCode.Mod mod;
      Option<SCode.Comment> cmt;
      Option<SCode.Annotation> ann;
      Option<Absyn.Exp> cond;
      Absyn.Path bcp, p;
      Element e;
      String n, str;
      Option<Absyn.ArrayDim> ad;

    case (SCode.CLASS(n, pf, ep, pp, res, SCode.DERIVED(Absyn.TPATH(p, ad), mod, attr, cmt), i), _)
      equation
        str = "'" +& Absyn.pathLastIdent(p) +& "/" +& inName +& "'";
        p = Absyn.pathSetLastIdent(p, Absyn.IDENT(str));
      then 
        SCode.CLASS(n, pf, ep, pp, res, SCode.DERIVED(Absyn.TPATH(p, ad), mod, attr, cmt), i);

    case (SCode.CLASS(n, pf, ep, pp, res, cdef, i), _)
      equation
        n = "'" +& n +& "/" +& inName +& "'";
      then 
        SCode.CLASS(n, pf, ep, pp, res, cdef, i);

    case (SCode.COMPONENT(n, pf, attr, ty, mod, cmt, cond, i), _)
      equation
        n = "'" +& n +& "/" +& inName +& "'";
      then 
        SCode.COMPONENT(n, pf, attr, ty, mod, cmt, cond, i);

    case (SCode.EXTENDS(bcp, v, mod, ann, i), _)
      equation
        n = "'" +& Absyn.pathLastIdent(bcp) +& "/" +& inName +& "'";
        bcp = Absyn.pathSetLastIdent(bcp, Absyn.IDENT(n));
      then 
        SCode.EXTENDS(bcp, v, mod, ann, i);

    case (e, _)
      equation
        print("SCodeApplyRedeclare.setElementName failed on applying name: " +&
        inName +& " for element:\n" +& 
        SCodeDump.unparseElementStr(e) +& "\n"); 
      then 
        fail();
  end matchcontinue;
end setElementName;
*/

protected function mkNewName
"changes the last indent in a pat to a unique name"
  input Absyn.Path p;
  input IScopes parentScopes;
  input Option<Absyn.Path> served;
  output Absyn.Path new;
protected
  String nn;
  Integer i;
algorithm
  new := matchcontinue(p, parentScopes, served)
    case (_, _, SOME(new)) then new;
    case (_, _, _) 
      equation
        i = System.tmpTickIndex(idxNames);  
        nn = Absyn.pathLastIdent(p) +& "__OMC__" +& intString(i);
        // nn = "'" +& Absyn.pathLastIdent(n) +& "_" +& SCodeAnalyseRedeclare.iScopesStrNoParts(listReverse(parentScopes)) +& "'"; 
        new = Absyn.pathSetLastIdent(p, Absyn.IDENT(nn));
     then
       new;
  end matchcontinue;
end mkNewName;

end SCodeApplyRedeclare;
