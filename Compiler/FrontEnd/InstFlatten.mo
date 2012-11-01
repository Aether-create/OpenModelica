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

encapsulated package InstFlatten
" file:        InstFlatten.mo
  package:     InstFlatten
  description: Functionality for flattening the instantiated structure.

  RCS: $Id: InstSymbolTable.mo 13687 2012-10-29 12:52:19Z perost $

  
"

public import Absyn;
public import InstTypes;

protected import BaseHashTable;
protected import DAE;
protected import Debug;
protected import Flags;
protected import InstDump;
protected import InstUtil;
protected import List;
protected import System;
protected import Types;
protected import Util;

public type Element = InstTypes.Element;
public type Equation = InstTypes.Equation;
public type Class = InstTypes.Class;
public type Dimension = InstTypes.Dimension;
public type Binding = InstTypes.Binding;
public type Component = InstTypes.Component;
public type Modifier = InstTypes.Modifier;
public type Prefixes = InstTypes.Prefixes;
public type Prefix = InstTypes.Prefix;
public type Statement = InstTypes.Statement;

protected type Key = String;
protected type Value = tuple<Component, list<Absyn.Path>>;

protected type HashTableFunctionsType = tuple<FuncHashKey, FuncKeyEqual, FuncKeyStr, FuncValueStr>;

protected type SymbolTable = tuple<
  array<list<tuple<Key, Integer>>>,
  tuple<Integer, Integer, array<Option<tuple<Key, Value>>>>,
  Integer,
  Integer,
  HashTableFunctionsType
>;

partial function FuncHashKey
  input Key inKey;
  input Integer inMod;
  output Integer outHash;
end FuncHashKey;

partial function FuncKeyEqual
  input Key inKey1;
  input Key inKey2;
  output Boolean outEqual;
end FuncKeyEqual;

partial function FuncKeyStr
  input Key inKey;
  output String outString;
end FuncKeyStr;

partial function FuncValueStr
  input Value inValue;
  output String outString;
end FuncValueStr;

protected function valueStr
  input Value inValue;
  output String outString;
protected
  Component comp;
algorithm
  (comp, _) := inValue;
  outString := InstDump.componentStr(comp);
end valueStr;
  
protected function newSymbolTable
  input Integer inSize;
  output SymbolTable outSymbolTable;
algorithm
  outSymbolTable := BaseHashTable.emptyHashTableWork(inSize,
    (System.stringHashDjb2Mod, stringEq, Util.id, valueStr));
end newSymbolTable;

public function flattenElements
  input list<Element> inElements;
  input Absyn.Path inClassPath;
  input Boolean inContainsExtends;
  output list<Element> outElements;
algorithm
  outElements := matchcontinue(inElements, inClassPath, inContainsExtends)
    local
      SymbolTable st;
      Integer el_count;
      list<Element> flat_el;

    // The elements are already flattened if we have no extends.
    case (_, _, false) then inElements;

    case (_, _, _)
      equation
        el_count = listLength(inElements);
        st = newSymbolTable(intDiv(el_count * 4, 3) + 1);
        (flat_el, _) = flattenElements2(inElements, st, {}, inClassPath, {});
      then
        flat_el;
     
    else
      equation
        true = Flags.isSet(Flags.FAILTRACE);
        Debug.traceln("- InstFlatten.flattenElements failed for " +&
            Absyn.pathString(inClassPath) +& "\n");
      then
        fail();

  end matchcontinue;
end flattenElements;

protected function flattenElements2
  input list<Element> inElements;
  input SymbolTable inSymbolTable;
  input list<Absyn.Path> inExtendPath;
  input Absyn.Path inClassPath;
  input list<Element> inAccumEl;
  output list<Element> outElements;
  output SymbolTable outSymbolTable;
algorithm
  (outElements, outSymbolTable) :=
  match(inElements, inSymbolTable, inExtendPath, inClassPath, inAccumEl)
    local
      Element el;
      list<Element> rest_el, accum_el;
      SymbolTable st;
      
    case (el :: rest_el, st, _, _, accum_el)
      equation
        (accum_el, st) = flattenElement(el, st, inExtendPath, inClassPath, accum_el);
        (accum_el, st) = flattenElements2(rest_el, st, inExtendPath, inClassPath, accum_el);
      then
        (accum_el, st);

    case ({}, st, _, _, accum_el) then (listReverse(accum_el), st);

  end match;
end flattenElements2;

protected function flattenElement
  input Element inElement;
  input SymbolTable inSymbolTable;
  input list<Absyn.Path> inExtendPath;
  input Absyn.Path inClassPath;
  input list<Element> inAccumEl;
  output list<Element> outElements;
  output SymbolTable outSymbolTable;
algorithm
  (outElements, outSymbolTable) :=
  match(inElement, inSymbolTable, inExtendPath, inClassPath, inAccumEl)
    local
      Element el;
      list<Element> ext_el, accum_el;
      Component comp;
      SymbolTable st;
      String name;
      Boolean add_el;
      list<DAE.Var> vars;
      list<String> var_names;
      Absyn.Path bc;

    // Extending from a class with no components, no elements to flatten.
    case (InstTypes.EXTENDED_ELEMENTS(
        cls = InstTypes.COMPLEX_CLASS(components = {})), st, _, _, accum_el)
      then (accum_el, st);

    case (InstTypes.EXTENDED_ELEMENTS(cls = InstTypes.BASIC_TYPE(name = _)),
        st, _, _, accum_el)
      then (inElement :: accum_el, st);

    case (InstTypes.EXTENDED_ELEMENTS(baseClass = bc,
        cls = InstTypes.COMPLEX_CLASS(components = ext_el), 
        ty = DAE.T_COMPLEX(varLst = vars)), st, _, _, accum_el)
      equation
        // For extended elements we can use the names from the type, which are
        // already the last identifiers.
        var_names = List.map(vars, Types.getVarName); 
        (accum_el, st) = flattenExtendedElements(ext_el, var_names,
          bc :: inExtendPath, inClassPath, st, accum_el);
      then
        (accum_el, st);
  
    case (InstTypes.ELEMENT(component = InstTypes.PACKAGE(name = _)),
        st, _, _, accum_el)
      then 
        (inElement :: accum_el, st);

    case (el, st, _, _, accum_el)
      equation
        comp = InstUtil.getElementComponent(el);
        name = Absyn.pathLastIdent(InstUtil.getComponentName(comp));
        (add_el, st) =
          flattenElement2(name, comp, inExtendPath, inClassPath, st, accum_el);
        accum_el = List.consOnTrue(add_el, el, accum_el);
      then
        (accum_el, st);

  end match;
end flattenElement;

protected function flattenElement2
  input String inName;
  input Component inComponent;
  input list<Absyn.Path> inExtendPath;
  input Absyn.Path inClassPath;
  input SymbolTable inSymbolTable;
  input list<Element> inAccumEl;
  output Boolean outShouldAdd;
  output SymbolTable outSymbolTable;
algorithm
  (outShouldAdd, outSymbolTable) :=
  matchcontinue(inName, inComponent, inExtendPath, inClassPath, inSymbolTable, inAccumEl)
    local
      list<Element> accum_el;
      SymbolTable st;

    // Try to add the component to the 
    case (_, _, _, _, st, accum_el)
      equation
        st = BaseHashTable.addUnique((inName, (inComponent, inExtendPath)), st); 
      then
        (true, st);

    case (_, _, _, _, st, accum_el)
      equation
        print("Removing equal " +& inName +& "\n");
        // Look up the already existing component here and check that they are
        // equal.
      then
        (false, st);
        
  end matchcontinue;
end flattenElement2;

protected function flattenExtendedElements
  input list<Element> inElements;
  input list<String> inNames;
  input list<Absyn.Path> inExtendPath;
  input Absyn.Path inClassPath;
  input SymbolTable inSymbolTable;
  input list<Element> inAccumEl;
  output list<Element> outElements;
  output SymbolTable outSymbolTable;
algorithm
  (outElements, outSymbolTable) :=
  match(inElements, inNames, inExtendPath, inClassPath, inSymbolTable, inAccumEl)
    local
      Element el;
      list<Element> rest_el, accum_el;
      Component comp;
      String name;
      list<String> rest_names;
      SymbolTable st;
      Boolean add_el;

    case ((el as InstTypes.ELEMENT(component = InstTypes.PACKAGE(name = _))) :: rest_el,
        _, _, _, st, accum_el)
      equation
        accum_el = el :: accum_el;
        (accum_el, st) = flattenExtendedElements(rest_el, inNames,
          inExtendPath, inClassPath, st, accum_el);
      then
        (accum_el, st);

    // Extended elements should not contain nested extended elements, since they
    // should have been flattened in instElementList. So we can assume that we
    // only have normal elements here.
    case (el :: rest_el, name :: rest_names, _, _, st, accum_el)
      equation
        comp = InstUtil.getElementComponent(el);
        (add_el, st) =
          flattenElement2(name, comp, inExtendPath, inClassPath, st, accum_el);
        accum_el = List.consOnTrue(add_el, el, accum_el);
        (accum_el, st) = flattenExtendedElements(rest_el, rest_names,
          inExtendPath, inClassPath, st, accum_el);
      then
        (accum_el, st);

    case ({}, {}, _, _, st, accum_el) then (accum_el, st);

  end match;
end flattenExtendedElements;

end InstFlatten;

