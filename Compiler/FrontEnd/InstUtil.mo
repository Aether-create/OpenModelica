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

encapsulated package InstUtil
" file:        InstUtil.mo
  package:     InstUtil
  description: Utility functions for InstTypes.

  RCS: $Id$

  Utility functions for operating on the types in InstTypes.
"

public import Absyn;
public import ClassInf;
public import DAE;
public import InstSymbolTable;
public import InstTypes;
public import SCode;
public import SCodeEnv;

protected import ComponentReference;
protected import Debug;
protected import Error;
protected import Expression;
protected import Flags;
protected import InstDump;
protected import List;
protected import SCodeDump;
protected import Types;
protected import Util;

public type Binding = InstTypes.Binding;
public type Class = InstTypes.Class;
public type Component = InstTypes.Component;
public type Condition = InstTypes.Condition;
public type DaePrefixes = InstTypes.DaePrefixes;
public type Dimension = InstTypes.Dimension;
public type Element = InstTypes.Element;
public type Env = SCodeEnv.Env;
public type Equation = InstTypes.Equation;
public type Function = InstTypes.Function;
public type Modifier = InstTypes.Modifier;
public type ParamType = InstTypes.ParamType;
public type Prefixes = InstTypes.Prefixes;
public type Prefix = InstTypes.Prefix;
public type Statement = InstTypes.Statement;
public type SymbolTable = InstSymbolTable.SymbolTable;

public function makeClassType
  input Class inClass;
  input ClassInf.State inState;
  input Boolean inContainsSpecialExtends;
  output Class outClass;
  output DAE.Type outClassType;
algorithm
  (outClass, outClassType) := match(inClass, inState, inContainsSpecialExtends)
    local
      list<Element> elems;
      Class cls;
      DAE.Type ty;
      list<DAE.Var> vars;
      Absyn.Path name;

    case (InstTypes.COMPLEX_CLASS(components = elems), _, false)
      equation
        vars = List.accumulateMapReverse(elems, makeDaeVarsFromElement);
        ty = DAE.T_COMPLEX(inState, vars, NONE(), DAE.emptyTypeSource);
      then
        (inClass, ty);

    case (InstTypes.COMPLEX_CLASS(_, elems, {}, {}, {}, {}), _, true)
      equation
        (InstTypes.EXTENDED_ELEMENTS(cls = cls, ty = ty), elems) =
          getSpecialExtends(elems);
      then
        (cls, ty);

  end match;
end makeClassType;

public function makeDaeVarsFromElement
  input Element inElement;
  input list<DAE.Var> inAccumVars;
  output list<DAE.Var> outVars;
algorithm
  outVars := match(inElement, inAccumVars)
    local
      Component comp;
      Class cls;
      list<DAE.Var> vars;
      DAE.Var var;

    case (InstTypes.ELEMENT(component = InstTypes.PACKAGE(name = _)), vars)
      then vars;

    case (InstTypes.ELEMENT(component = comp, cls = cls), vars)
      equation
        var = componentToDaeVar(comp);
      then
        var :: vars;

    case (InstTypes.CONDITIONAL_ELEMENT(component = comp), vars)
      equation
        var = componentToDaeVar(comp);
      then
        var :: vars;

  end match;
end makeDaeVarsFromElement;

protected function componentToDaeVar
  input Component inComponent;
  output DAE.Var outVar;
algorithm
  outVar := match(inComponent)
    local
      Absyn.Path path;
      DAE.Type ty;
      String name;
      DAE.Attributes attr;
      Prefixes prefs;
      DaePrefixes dprefs;

    case InstTypes.UNTYPED_COMPONENT(name = path, prefixes = prefs)
      equation
        name = Absyn.pathLastIdent(path);
        attr = prefixesToDaeAttr(prefs);
      then
        DAE.TYPES_VAR(name, attr, DAE.T_UNKNOWN_DEFAULT, DAE.UNBOUND(), NONE());

    case InstTypes.TYPED_COMPONENT(name = path, ty = ty, prefixes = dprefs)
      equation
        name = Absyn.pathLastIdent(path);
        attr = daePrefixesToDaeAttr(dprefs);
      then
        DAE.TYPES_VAR(name, attr, ty, DAE.UNBOUND(), NONE());

    case InstTypes.CONDITIONAL_COMPONENT(name = path)
      equation
        name = Absyn.pathLastIdent(path);
      then
        DAE.TYPES_VAR(name, DAE.dummyAttrVar, DAE.T_UNKNOWN_DEFAULT,
          DAE.UNBOUND(), NONE());

    case InstTypes.OUTER_COMPONENT(name = path)
      equation
        name = Absyn.pathLastIdent(path);
      then
        DAE.TYPES_VAR(name, DAE.dummyAttrVar, DAE.T_UNKNOWN_DEFAULT,
          DAE.UNBOUND(), NONE());

    case InstTypes.COMPONENT_ALIAS(componentName = _)
      equation
        print("Got component alias in componentToDaeVar\n");
      then
        fail();

    case InstTypes.DELETED_COMPONENT(name = _)
      equation
        print("Got deleted component\n");
      then
        fail();

    case InstTypes.PACKAGE(name = _)
      equation
        print("PACKAGE\n");
      then
        fail();
  end match;
end componentToDaeVar;

protected function prefixesToDaeAttr
  input Prefixes inPrefixes;
  output DAE.Attributes outAttr;
algorithm
  outAttr := match(inPrefixes)
    local
      SCode.ConnectorType cty;
      SCode.Variability var;
      Absyn.Direction dir;
      Absyn.InnerOuter io;
      SCode.Visibility vis;

    case InstTypes.NO_PREFIXES() then DAE.dummyAttrVar;
    case InstTypes.PREFIXES(vis, var, _, io, (dir, _), (cty, _), _)
      then DAE.ATTR(cty, SCode.NON_PARALLEL(), var, dir, io, vis);

  end match;
end prefixesToDaeAttr;

protected function daePrefixesToDaeAttr
  input DaePrefixes inPrefixes;
  output DAE.Attributes outAttr;
algorithm
  outAttr := match(inPrefixes)
    local
      DAE.VarVisibility dvis;
      DAE.VarKind dvar;
      DAE.VarDirection ddir;
      DAE.ConnectorType dcty;
      SCode.ConnectorType cty;
      SCode.Variability var;
      Absyn.Direction dir;
      Absyn.InnerOuter io;
      SCode.Visibility vis;

    case InstTypes.NO_DAE_PREFIXES() then DAE.dummyAttrVar;
    case InstTypes.DAE_PREFIXES(dvis, dvar, _, io, ddir, dcty)
      equation
        vis = daeToSCodeVisibility(dvis);
        var = daeToSCodeVariability(dvar);
        dir = daeToAbsynDirection(ddir);
        cty = daeToSCodeConnectorType(dcty);
      then
        DAE.ATTR(cty, SCode.NON_PARALLEL(), var, dir, io, vis);

  end match;
end daePrefixesToDaeAttr;

public function daeToSCodeVisibility
  input DAE.VarVisibility inVisibility;
  output SCode.Visibility outVisibility;
algorithm
  outVisibility := match(inVisibility)
    case DAE.PUBLIC() then SCode.PUBLIC();
    case DAE.PROTECTED() then SCode.PROTECTED();
  end match;
end daeToSCodeVisibility;

public function daeToSCodeVariability
  input DAE.VarKind inVariability;
  output SCode.Variability outVariability;
algorithm
  outVariability := match(inVariability)
    case DAE.VARIABLE() then SCode.VAR();
    case DAE.DISCRETE() then SCode.DISCRETE();
    case DAE.PARAM() then SCode.PARAM();
    case DAE.CONST() then SCode.CONST();
  end match;
end daeToSCodeVariability;

public function daeToAbsynDirection
  input DAE.VarDirection inDirection;
  output Absyn.Direction outDirection;
algorithm
  outDirection := match(inDirection)
    case DAE.BIDIR() then Absyn.BIDIR();
    case DAE.INPUT() then Absyn.INPUT();
    case DAE.OUTPUT() then Absyn.OUTPUT();
  end match;
end daeToAbsynDirection;

protected function daeToSCodeConnectorType
  input DAE.ConnectorType inConnectorType;
  output SCode.ConnectorType outConnectorType;
algorithm
  outConnectorType := match(inConnectorType)
    case DAE.NON_CONNECTOR() then SCode.POTENTIAL();
    case DAE.POTENTIAL() then SCode.POTENTIAL();
    case DAE.FLOW() then SCode.FLOW();
    case DAE.STREAM() then SCode.STREAM();
  end match;
end daeToSCodeConnectorType;
  
public function makeDerivedClassType
  input DAE.Type inType;
  input ClassInf.State inState;
  output DAE.Type outType;
algorithm
  outType := match(inType, inState)
    local
      list<DAE.Var> vars;
      DAE.EqualityConstraint ec;
      DAE.TypeSource src;

    // TODO: Check type restrictions.
    case (DAE.T_COMPLEX(_, vars, ec, src), _)
      then DAE.T_COMPLEX(inState, vars, ec, src);

    else DAE.T_SUBTYPE_BASIC(inState, {}, inType, NONE(), DAE.emptyTypeSource);

  end match;
end makeDerivedClassType;

public function arrayElementType
  input DAE.Type inType;
  output DAE.Type outType;
algorithm
  outType := match(inType)
    local
      DAE.Type ty;
      ClassInf.State state;
      DAE.EqualityConstraint ec;
      DAE.TypeSource src;

    case DAE.T_ARRAY(ty = ty) then arrayElementType(ty);
    case DAE.T_SUBTYPE_BASIC(state, _, ty, ec, src)
      equation
        ty = arrayElementType(ty);
      then
        DAE.T_SUBTYPE_BASIC(state, {}, ty, ec, src);

    else inType;
  end match;
end arrayElementType;

public function addElementsToClass
  input list<Element> inElements;
  input Class inClass;
  output Class outClass;
algorithm
  outClass := match(inElements, inClass)
    local
      list<Element> el;
      list<Equation> eq, ieq;
      list<list<Statement>> al, ial;
      Absyn.Path name;

    case ({}, _) then inClass;

    case (_, InstTypes.COMPLEX_CLASS(name, el, eq, ieq, al, ial))
      equation
        el = listAppend(inElements, el);
      then
        InstTypes.COMPLEX_CLASS(name, el, eq, ieq, al, ial);

    case (_, InstTypes.BASIC_TYPE(name))
      equation
        Error.addMessage(Error.INTERNAL_ERROR,
          {"SCodeInst.addElementsToClass: Can't add elements to basic type.\n"});
      then
        fail();

  end match;
end addElementsToClass;

public function getElementComponent
  input Element inElement;
  output Component outComponent;
algorithm
  outComponent := match(inElement)
    local
      Component comp;

    case InstTypes.ELEMENT(component = comp) then comp;
    case InstTypes.CONDITIONAL_ELEMENT(component = comp) then comp;

  end match;
end getElementComponent;

public function getComponentName
  input Component inComponent;
  output Absyn.Path outName;
algorithm
  outName := match(inComponent)
    local
      Absyn.Path name;

    case InstTypes.UNTYPED_COMPONENT(name = name) then name;
    case InstTypes.TYPED_COMPONENT(name = name) then name;
    case InstTypes.CONDITIONAL_COMPONENT(name = name) then name;
    case InstTypes.DELETED_COMPONENT(name = name) then name;
    case InstTypes.OUTER_COMPONENT(name = name) then name;
    case InstTypes.PACKAGE(name = name) then name;

  end match;
end getComponentName;

public function setComponentName
  input Component inComponent;
  input Absyn.Path inName;
  output Component outComponent;
algorithm
  outComponent := match(inComponent, inName)
    local
      DAE.Type ty;
      array<Dimension> dims;
      Prefixes prefs;
      DaePrefixes dprefs;
      ParamType pty;
      Binding binding;
      Absyn.Info info;
      SCode.Element elem;
      Modifier mod;
      Env env;
      Prefix prefix;
      Option<Absyn.Path> inner_name;
      DAE.Exp cond;
      Option<Component> p;

    case (InstTypes.UNTYPED_COMPONENT(_, ty, dims, prefs, pty, binding, info), _)
      then InstTypes.UNTYPED_COMPONENT(inName, ty, dims, prefs, pty, binding, info);

    case (InstTypes.TYPED_COMPONENT(_, ty, p, dprefs, binding, info), _)
      then InstTypes.TYPED_COMPONENT(inName, ty, p, dprefs, binding, info);

    case (InstTypes.CONDITIONAL_COMPONENT(_, cond, elem, mod, prefs, env, prefix, info), _)
      then InstTypes.CONDITIONAL_COMPONENT(inName, cond, elem, mod, prefs, env, prefix, info);

    case (InstTypes.DELETED_COMPONENT(_), _)
      then InstTypes.DELETED_COMPONENT(inName);

    case (InstTypes.OUTER_COMPONENT(_, inner_name), _)
      then InstTypes.OUTER_COMPONENT(inName, inner_name);

    case (InstTypes.PACKAGE(_,p), _)
      then InstTypes.PACKAGE(inName,p);

  end match;
end setComponentName;

public function setTypedComponentType
  input Component inComponent;
  input DAE.Type inType;
  output Component outComponent;
algorithm
  outComponent := match(inComponent, inType)
    local
      Absyn.Path name;
      DaePrefixes dprefs;
      Binding binding;
      Absyn.Info info;
      Option<Component> p;

    case (InstTypes.TYPED_COMPONENT(name, _, p, dprefs, binding, info), _)
      then InstTypes.TYPED_COMPONENT(name, inType, p, dprefs, binding, info);

    else inComponent;

  end match;
end setTypedComponentType;

public function setComponentParamType
  input Component inComponent;
  input ParamType inParamType;
  output Component outComponent;
algorithm
  outComponent := match(inComponent, inParamType)
    local
      Absyn.Path name;
      DAE.Type ty;
      array<Dimension> dims;
      Prefixes prefs;
      Binding binding;
      Absyn.Info info;

    case (InstTypes.UNTYPED_COMPONENT(name, ty, dims, prefs, _, binding, info), _)
      then InstTypes.UNTYPED_COMPONENT(name, ty, dims, prefs, inParamType, binding, info);

    else inComponent;

  end match;
end setComponentParamType;

public function getComponentType
  input Component inComponent;
  output DAE.Type outType;
algorithm
  outType := match(inComponent)
    local
      DAE.Type ty;

    case InstTypes.UNTYPED_COMPONENT(baseType = ty) then ty;
    case InstTypes.TYPED_COMPONENT(ty = ty) then ty;

  end match;
end getComponentType;

public function getComponentTypeDimensions
  input Component inComponent;
  output DAE.Dimensions outDims;
algorithm
  outDims := match(inComponent)
    local
      DAE.Type ty;
      array<Dimension> idimensions;
      list<Dimension> idims;
      DAE.Dimensions dims;
    
    case InstTypes.TYPED_COMPONENT(ty = ty)
      equation 
        dims = Types.getDimensions(ty);
      then dims;
  end match;
end getComponentTypeDimensions;

public function getComponentBinding
  input Component inComponent;
  output Binding outBinding;
algorithm
  outBinding := match(inComponent)
    local
      Binding binding;

    case InstTypes.UNTYPED_COMPONENT(binding = binding) then binding;
    case InstTypes.TYPED_COMPONENT(binding = binding) then binding;

  end match;
end getComponentBinding;
 
public function getComponentBindingExp
  input Component inComponent;
  output DAE.Exp outExp;
algorithm
  InstTypes.TYPED_COMPONENT(binding = 
    InstTypes.TYPED_BINDING(bindingExp = outExp)) := inComponent;
end getComponentBindingExp;

public function getComponentVariability
  input Component inComponent;
  output SCode.Variability outVariability;
algorithm
  outVariability := match(inComponent)
    local
      SCode.Variability var;

    case InstTypes.UNTYPED_COMPONENT(prefixes = 
      InstTypes.PREFIXES(variability = var)) then var;

    case InstTypes.TYPED_COMPONENT(prefixes = 
      InstTypes.DAE_PREFIXES(variability = DAE.CONST())) then SCode.CONST();

    case InstTypes.TYPED_COMPONENT(prefixes = 
      InstTypes.DAE_PREFIXES(variability = DAE.PARAM())) then SCode.PARAM();

    else SCode.VAR();

  end match;
end getComponentVariability;

public function getEffectiveComponentVariability
  input Component inComponent;
  output SCode.Variability outVariability;
algorithm
  outVariability := match(inComponent)
    case InstTypes.UNTYPED_COMPONENT(paramType = InstTypes.STRUCT_PARAM())
      then SCode.CONST();

    else getComponentVariability(inComponent);

  end match;
end getEffectiveComponentVariability;

public function getComponentConnectorType
  input Component inComponent;
  output DAE.ConnectorType outConnectorType;
algorithm
  outConnectorType := match(inComponent)
    local
      DAE.ConnectorType cty;

    case InstTypes.TYPED_COMPONENT(prefixes =
      InstTypes.DAE_PREFIXES(connectorType = cty)) then cty;

    else DAE.POTENTIAL();

  end match;
end getComponentConnectorType;

protected function getSpecialExtends
  input list<Element> inElements;
  output Element outSpecialExtends;
  output list<Element> outRestElements;
algorithm
  (outSpecialExtends, outRestElements) := getSpecialExtends2(inElements, {});
end getSpecialExtends;

protected function getSpecialExtends2
  input list<Element> inElements;
  input list<Element> inAccumEl;
  output Element outSpecialExtends;
  output list<Element> outRestElements;
algorithm
  (outSpecialExtends, outRestElements) := matchcontinue(inElements, inAccumEl)
    local
      Element el;
      list<Element> rest_el;
      DAE.Type ty;

    case ((el as InstTypes.EXTENDED_ELEMENTS(ty = ty)) :: rest_el, _)
      equation
        true = isSpecialExtends(ty);
        rest_el = listAppend(listReverse(inAccumEl), rest_el);
      then
        (el, rest_el);

    // TODO: Check for illegal elements here (components, etc.).

    case (el :: rest_el, _)
      equation
        (el, rest_el) = getSpecialExtends2(rest_el, el :: inAccumEl);
      then
        (el, rest_el);

    else
      equation
        true = Flags.isSet(Flags.FAILTRACE);
        Debug.traceln("- SCodeInst.getSpecialExtends2 failed!");
      then
        fail();

  end matchcontinue;
end getSpecialExtends2;

public function isSpecialExtends
  input DAE.Type inType;
  output Boolean outResult;
algorithm
  outResult := match(inType)
    case DAE.T_COMPLEX(varLst = _) then false;
    else true;
  end match;
end isSpecialExtends;

public function getComponentBindingDimension
  input Component inComponent;
  input Integer inDimension;
  input Integer inCompDimensions;
  output DAE.Dimension outDimension;
protected
  Binding binding;
algorithm
  binding := getComponentBinding(inComponent);
  outDimension := getBindingDimension(binding, inDimension, inCompDimensions);
end getComponentBindingDimension;

public function getBindingDimension
  input Binding inBinding;
  input Integer inDimension;
  input Integer inCompDimensions;
  output DAE.Dimension outDimension;
algorithm
  outDimension := match(inBinding, inDimension, inCompDimensions)
    local
      DAE.Exp exp;
      Integer pd, index;

    case (InstTypes.TYPED_BINDING(bindingExp = exp, propagatedDims = pd), _, _)
      equation
        index = Util.if_(intEq(pd, -1), inDimension,
          inDimension + pd - inCompDimensions);
      then
        getExpDimension(exp, index);

  end match;
end getBindingDimension;
  
public function getExpDimension
  input DAE.Exp inExp;
  input Integer inDimIndex;
  output DAE.Dimension outDimension;
algorithm
  outDimension := matchcontinue(inExp, inDimIndex)
    local
      DAE.Type ty;
      list<DAE.Dimension> dims;
      DAE.Dimension dim;

    case (_, _)
      equation
        ty = Expression.typeof(inExp);
        dims = Types.getDimensions(ty);
        dim = listGet(dims, inDimIndex);
      then
        dim;

    // TODO: Error on index out of bounds!

    else DAE.DIM_UNKNOWN();

  end matchcontinue;
end getExpDimension;
 
public function getBindingExp
  input Binding inBinding;
  output DAE.Exp outExp;
algorithm
  outExp := match(inBinding)
    local
      DAE.Exp exp;

    case InstTypes.TYPED_BINDING(bindingExp = exp) then exp;
    else DAE.ICONST(0);
  end match;
end getBindingExp;

public function getBindingExpOpt
  input Binding inBinding;
  output Option<DAE.Exp> outExp;
algorithm
  outExp := match(inBinding)
    local
      DAE.Exp exp;

    case InstTypes.UNTYPED_BINDING(bindingExp = exp) then SOME(exp);
    case InstTypes.TYPED_BINDING(bindingExp = exp) then SOME(exp);
    else NONE();

  end match;
end getBindingExpOpt;

public function getBindingTypeOpt
  input Binding inBinding;
  output Option<DAE.Type> outTy;
algorithm
  outTy := match(inBinding)
    local
      DAE.Type ty;

    case InstTypes.TYPED_BINDING(bindingType = ty) then SOME(ty);
    else NONE();

  end match;
end getBindingTypeOpt;

public function getBindingPropagatedDimsOpt
  input Binding inBinding;
  output Option<Integer> outPropagatedDimsOpt;
algorithm
  outPropagatedDimsOpt := match(inBinding)
    local
      Integer pd;

    case InstTypes.TYPED_BINDING(propagatedDims = pd) then SOME(pd);
    case InstTypes.RAW_BINDING(propagatedDims = pd) then SOME(pd);
    case InstTypes.UNTYPED_BINDING(propagatedDims = pd) then SOME(pd);
    else NONE();

  end match;
end getBindingPropagatedDimsOpt;

public function makeEnumType
  input list<SCode.Enum> inEnumLiterals;
  input Absyn.Path inEnumPath;
  output DAE.Type outType;
protected
  list<String> names;
algorithm
  names := List.map(inEnumLiterals, SCode.enumName);
  outType := DAE.T_ENUMERATION(NONE(), inEnumPath, names, {}, {}, DAE.emptyTypeSource);
end makeEnumType;

public function makeEnumLiteralComp
  input Absyn.Path inName;
  input DAE.Type inType;
  input Integer inIndex;
  output Component outComponent;
protected
  Binding binding;
algorithm
  binding := InstTypes.TYPED_BINDING(DAE.ENUM_LITERAL(inName, inIndex), inType,
    0, Absyn.dummyInfo);
  outComponent := InstTypes.TYPED_COMPONENT(inName, inType, NONE(),
    InstTypes.DEFAULT_CONST_DAE_PREFIXES, binding, Absyn.dummyInfo);
end makeEnumLiteralComp;

public function makeDimension
  input DAE.Exp inExp;
  output DAE.Dimension outDimension;
algorithm
  outDimension := match(inExp)
    local
      Integer idim;

    case DAE.ICONST(idim) then DAE.DIM_INTEGER(idim);
    else DAE.DIM_EXP(inExp);
  end match;
end makeDimension;

public function makeDimensionArray
  input list<DAE.Dimension> inDimensions;
  output array<Dimension> outDimensions;
protected
  list<Dimension> dims;
algorithm
  dims := List.map(inDimensions, wrapDimension);
  outDimensions := listArray(dims);
end makeDimensionArray;

public function wrapDimension
  input DAE.Dimension inDimension;
  output Dimension outDimension;
algorithm
  outDimension := InstTypes.UNTYPED_DIMENSION(inDimension, false);
end wrapDimension;

public function wrapTypedDimension
  input DAE.Dimension inDimension;
  output Dimension outDimension;
algorithm
  outDimension := InstTypes.TYPED_DIMENSION(inDimension);
end wrapTypedDimension;
  
public function unwrapDimension
  input Dimension inDimension;
  output DAE.Dimension outDimension;
algorithm
  outDimension := match inDimension
    local
      DAE.Dimension dim;
    case InstTypes.UNTYPED_DIMENSION(dimension=dim) then dim;
    case InstTypes.TYPED_DIMENSION(dimension=dim) then dim;
  end match;
end unwrapDimension;

public function makeIterator
  input Absyn.Path inName;
  input DAE.Type inType;
  input Absyn.Info inInfo;
  output Component outIterator;
algorithm
  outIterator := InstTypes.TYPED_COMPONENT(inName, inType, NONE(),
    InstTypes.NO_DAE_PREFIXES(), InstTypes.UNBOUND(), inInfo);
end makeIterator;

public function mergePrefixesFromComponent
  "Merges a component's prefixes with the given prefixes, with the component's
   prefixes having priority."
  input Absyn.Path inComponentName;
  input SCode.Element inComponent;
  input Prefixes inPrefixes;
  output Prefixes outPrefixes;
algorithm
  outPrefixes := match(inComponentName, inComponent, inPrefixes)
    local
      SCode.Prefixes pf;
      SCode.Attributes attr;
      Prefixes prefs;
      Absyn.Info info;
      Option<SCode.Comment> comment;
      String err_str;

    case (_, SCode.COMPONENT(prefixes = pf, attributes = attr, comment = comment, info = info), _)
      equation
        prefs = makePrefixes(pf, attr, comment, info);
        prefs = mergePrefixes(prefs, inPrefixes, inComponentName, "variable");
      then
        prefs;

    else
      equation
        err_str = Absyn.pathString(inComponentName);
        err_str = "InstUtil.mergePrefixesFromComponent got " +& err_str +&
          " which is not a component!";
        Error.addMessage(Error.INTERNAL_ERROR, {err_str});
      then
        fail();

  end match;
end mergePrefixesFromComponent;

protected function makePrefixes
  "Creates an InstTypes.Prefixes record from SCode.Prefixes and SCode.Attributes."
  input SCode.Prefixes inPrefixes;
  input SCode.Attributes inAttributes;
  input Option<SCode.Comment> inComment;
  input Absyn.Info inInfo;
  output Prefixes outPrefixes;
algorithm
  outPrefixes := match(inPrefixes, inAttributes, inComment, inInfo)
    local
      SCode.Visibility vis;
      SCode.Variability var;
      SCode.Final fp;
      Absyn.InnerOuter io;
      Absyn.Direction dir;
      SCode.ConnectorType ct;
      Absyn.Info info;
      InstTypes.VarArgs va;

    // All prefixes are the default ones, same as having no prefixes.
    case (SCode.PREFIXES(visibility = SCode.PUBLIC(), finalPrefix =
        SCode.NOT_FINAL(), innerOuter = Absyn.NOT_INNER_OUTER()), SCode.ATTR(
        connectorType = SCode.POTENTIAL(), variability = SCode.VAR(),
        direction = Absyn.BIDIR()), _, _)
      then InstTypes.NO_PREFIXES();

    // Otherwise, select the prefixes we are interested in and build a PREFIXES
    // record.
    case (SCode.PREFIXES(visibility = vis, finalPrefix = fp, innerOuter = io),
          SCode.ATTR(connectorType = ct, variability = var, direction = dir), _, info)
      equation
        va = makeVarArg(dir,inComment);
      then InstTypes.PREFIXES(vis, var, fp, io, (dir, info), (ct, info), va);

  end match;
end makePrefixes;

protected function makeVarArg "Checks if the component might be a varargs type of component"
  input Absyn.Direction inDir;
  input Option<SCode.Comment> inComment;
  output InstTypes.VarArgs varArgs;
algorithm
  varArgs := match (inDir,inComment)
    case (Absyn.INPUT(),_)
      then
        Util.if_(SCode.optCommentHasBooleanNamedAnnotation(inComment,"__OpenModelica_varArgs"),InstTypes.IS_VARARG(),InstTypes.NO_VARARG());
    else InstTypes.NO_VARARG();
  end match;
end makeVarArg;

public function mergePrefixesWithDerivedClass
  "Merges the attributes of a derived class with the given prefixes."
  input Absyn.Path inClassName;
  input SCode.Element inClass;
  input Prefixes inPrefixes;
  output Prefixes outPrefixes;
algorithm
  outPrefixes := match(inClassName, inClass, inPrefixes)
    local
      SCode.Attributes attr;
      Absyn.Info info;
      Prefixes prefs;

    case (_, SCode.CLASS(classDef = SCode.DERIVED(attributes = attr), info = info), _)
      equation
        prefs = makePrefixesFromAttributes(attr, info);
        prefs = mergePrefixes(prefs, inPrefixes, inClassName, "class");
      then
        prefs;

  end match;
end mergePrefixesWithDerivedClass;

protected function makePrefixesFromAttributes
  "Creates an InstTypes.Prefixes record from an SCode.Attributes."
  input SCode.Attributes inAttributes;
  input Absyn.Info inInfo;
  output Prefixes outPrefixes;
algorithm
  outPrefixes := match(inAttributes, inInfo)
    local
      SCode.ConnectorType ct;
      SCode.Variability var;
      Absyn.Direction dir;

    // All attributes are the default ones, same as having no prefixes.
    case (SCode.ATTR(connectorType = SCode.POTENTIAL(), variability = SCode.VAR(),
        direction = Absyn.BIDIR()), _)
      then InstTypes.NO_PREFIXES();

    // Otherwise, select the attributes we are interested in and build a
    // PREFIXES record with the parts not covered by SCode.Attributes set to the
    // default values.
    case (SCode.ATTR(connectorType = ct, variability = var, direction = dir), _)
      then InstTypes.PREFIXES(SCode.PUBLIC(), var, SCode.NOT_FINAL(),
        Absyn.NOT_INNER_OUTER(), (dir, inInfo), (ct, inInfo), InstTypes.NO_VARARG());

  end match;
end makePrefixesFromAttributes;

public function mergePrefixes
  "Merges two InstTypes.Prefixes records, with the outer having priority over
   the inner. inElementName and inElementType are used for error reporting, where
   inElementName is the name of the element that the outer prefixes comes from
   and inElementType the type of that element as a string (variable or class)."
  input Prefixes inOuterPrefixes;
  input Prefixes inInnerPrefixes;
  input Absyn.Path inElementName;
  input String inElementType;
  output Prefixes outPrefixes;
algorithm
  outPrefixes :=
  match(inOuterPrefixes, inInnerPrefixes, inElementName, inElementType)
    local
      SCode.Visibility vis1, vis2;
      SCode.Variability var1, var2;
      SCode.Final fp1, fp2;
      Absyn.InnerOuter io1, io2;
      tuple<Absyn.Direction, Absyn.Info> dir1, dir2;
      tuple<SCode.ConnectorType, Absyn.Info> ct1, ct2;
      InstTypes.VarArgs va2;

    // No outer prefixes => no change.
    case (InstTypes.NO_PREFIXES(), _, _, _) then inInnerPrefixes;
    // No inner prefixes => overwrite with outer prefixes.
    case (_, InstTypes.NO_PREFIXES(), _, _) then inOuterPrefixes;

    // Both outer and inner prefixes => merge them.
    case (InstTypes.PREFIXES(vis1, var1, fp1, io1, dir1, ct1, _),
          InstTypes.PREFIXES(vis2, var2, fp2, io2, dir2, ct2, va2), _, _)
      equation
        vis2 = mergeVisibility(vis1, vis2);
        var2 = mergeVariability(var1, var2);
        fp2 = mergeFinal(fp1, fp2);
        dir2 = mergeDirection(dir1, dir2, inElementName, inElementType);
        ct2 = mergeConnectorType(ct1, ct2, inElementName, inElementType);
      then
        InstTypes.PREFIXES(vis2, var2, fp2, io1, dir2, ct2, va2);

  end match;
end mergePrefixes;

public function mergePrefixesFromExtends
  input SCode.Element inExtends;
  input Prefixes inPrefixes;
  output Prefixes outPrefixes;
protected
  SCode.Visibility vis;
algorithm
  SCode.EXTENDS(visibility = vis) := inExtends;
  outPrefixes := setPrefixVisibility(vis, inPrefixes);
end mergePrefixesFromExtends;

protected function setPrefixVisibility
  input SCode.Visibility inVisibility;
  input Prefixes inPrefixes;
  output Prefixes outPrefixes;
algorithm
  outPrefixes := match(inVisibility, inPrefixes)
    local
      SCode.Variability var;
      SCode.Final fp;
      Absyn.InnerOuter io;
      tuple<Absyn.Direction, Absyn.Info> dir;
      tuple<SCode.ConnectorType, Absyn.Info> ct;
      InstTypes.VarArgs va;

    case (SCode.PUBLIC(), _) then inPrefixes;

    case (_, InstTypes.PREFIXES(_, var, fp, io, dir, ct, va))
      then InstTypes.PREFIXES(inVisibility, var, fp, io, dir, ct, va);

    else InstTypes.DEFAULT_PROTECTED_PREFIXES;

  end match;
end setPrefixVisibility;

protected function mergeVisibility
  "Merges an outer and inner visibility prefix."
  input SCode.Visibility inOuterVisibility;
  input SCode.Visibility inInnerVisibility;
  output SCode.Visibility outVisibility;
algorithm
  outVisibility := match(inOuterVisibility, inInnerVisibility)
    // If the outer is protected, return protected.
    case (SCode.PROTECTED(), _) then inOuterVisibility;
    // Otherwise, no change.
    else inInnerVisibility;
  end match;
end mergeVisibility;

protected function mergeVariability
  "Merges an outer and inner variability prefix. The most restrictive
   variability is returned (with constant most restrictive, variable least)."
  input SCode.Variability inOuterVariability;
  input SCode.Variability inInnerVariability;
  output SCode.Variability outVariability;
algorithm
  outVariability := match(inOuterVariability, inInnerVariability)
    case (SCode.CONST(), _) then inOuterVariability;
    case (_, SCode.CONST()) then inInnerVariability;
    case (SCode.PARAM(), _) then inOuterVariability;
    case (_, SCode.PARAM()) then inInnerVariability;
    case (SCode.DISCRETE(), _) then inOuterVariability;
    case (_, SCode.DISCRETE()) then inInnerVariability;
    else inInnerVariability;
  end match;
end mergeVariability;

protected function mergeFinal
  "Merges an outer and inner final prefix."
  input SCode.Final inOuterFinal;
  input SCode.Final inInnerFinal;
  output SCode.Final outFinal;
algorithm
  outFinal := match(inOuterFinal, inInnerFinal)
    // If the outer prefix is final, return final.
    case (SCode.FINAL(), _) then inOuterFinal;
    // Otherwise, no change.
    else inInnerFinal;
  end match;
end mergeFinal;

protected function mergeDirection
  "Merges an outer and inner direction prefix."
  input tuple<Absyn.Direction, Absyn.Info> inOuterDirection;
  input tuple<Absyn.Direction, Absyn.Info> inInnerDirection;
  input Absyn.Path inElementName;
  input String inElementType;
  output tuple<Absyn.Direction, Absyn.Info> outDirection;
algorithm
  outDirection :=
  match(inOuterDirection, inInnerDirection, inElementName, inElementType)
    local
      Absyn.Direction dir1, dir2;
      Absyn.Info info1, info2;
      String dir_str1, dir_str2, el_name;

    // If either prefix is unset, return the other.
    case (_, (Absyn.BIDIR(), _), _, _) then inOuterDirection;
    case ((Absyn.BIDIR(), _), _, _, _) then inInnerDirection;
    
    // we need this for now, see i.e. Modelica.Blocks.Math.Add3
    case ((Absyn.INPUT(), _), (Absyn.INPUT(), _),  _, _) then inInnerDirection;
    case ((Absyn.OUTPUT(), _), (Absyn.OUTPUT(), _),  _, _) then inInnerDirection;

    // Otherwise we have an error, since it's not allowed to overwrite
    // input/output prefixes.
    case ((dir1, info1), (dir2, info2), _, _)
      equation
        Error.addSourceMessage(Error.ERROR_FROM_HERE, {}, info2);
        dir_str1 = directionString(dir1);
        dir_str2 = directionString(dir2);
        el_name = Absyn.pathString(inElementName);
        Error.addSourceMessage(Error.INVALID_TYPE_PREFIX,
          {dir_str1, inElementType, el_name, dir_str2}, info1);
      then
        fail();

  end match;
end mergeDirection;
   
protected function directionString
  input Absyn.Direction inDirection;
  output String outString;
algorithm
  outString := match(inDirection)
    case Absyn.INPUT() then "input";
    case Absyn.OUTPUT() then "output";
    else "";
  end match;
end directionString;

protected function mergeConnectorType
  "Merges outer and inner connector type prefixes (flow, stream)."
  input tuple<SCode.ConnectorType, Absyn.Info> inOuterConnectorType;
  input tuple<SCode.ConnectorType, Absyn.Info> inInnerConnectorType;
  input Absyn.Path inElementName;
  input String inElementType;
  output tuple<SCode.ConnectorType, Absyn.Info> outConnectorType;
algorithm
  outConnectorType := matchcontinue(inOuterConnectorType, inInnerConnectorType,
      inElementName, inElementType)
    local
      SCode.ConnectorType ct1, ct2;
      Absyn.Info info1, info2;
      String ct1_str, ct2_str, el_name;

    // If either of the prefixes are unset, return the others.
    case ((SCode.POTENTIAL(), _), _, _, _) then inInnerConnectorType;
    case (_, (SCode.POTENTIAL(), _), _, _) then inOuterConnectorType;

    // Trying to overwrite a flow/stream prefix => show error.
    case ((ct1, info1), (ct2, info2), _, _)
      equation
        Error.addSourceMessage(Error.ERROR_FROM_HERE, {}, info1);
        ct1_str = SCodeDump.connectorTypeStr(ct1);
        ct2_str = SCodeDump.connectorTypeStr(ct2);
        el_name = Absyn.pathString(inElementName);
        Error.addSourceMessage(Error.INVALID_TYPE_PREFIX,
          {ct2_str, inElementType, el_name, ct1_str}, info2);
      then
        fail();

  end matchcontinue;
end mergeConnectorType;

public function addPrefix
  input String inName;
  input list<DAE.Dimension> inDimensions;
  input Prefix inPrefix;
  output Prefix outPrefix;
algorithm
  outPrefix := InstTypes.PREFIX(inName, inDimensions, inPrefix);
end addPrefix;

public function prefixCref
  input DAE.ComponentRef inCref;
  input Prefix inPrefix;
  output DAE.ComponentRef outCref;
algorithm
  outCref := match(inCref, inPrefix)
    local
      String name;
      Prefix rest_prefix;
      DAE.ComponentRef cref;

    case (_, InstTypes.EMPTY_PREFIX(classPath = _)) then inCref;

    case (_, InstTypes.PREFIX(name = name, 
        restPrefix = InstTypes.EMPTY_PREFIX(classPath = _)))
      then DAE.CREF_QUAL(name, DAE.T_UNKNOWN_DEFAULT, {}, inCref);

    case (_, InstTypes.PREFIX(name = name, restPrefix = rest_prefix))
      equation
        cref = DAE.CREF_QUAL(name, DAE.T_UNKNOWN_DEFAULT, {}, inCref);
      then
        prefixCref(cref, rest_prefix);

  end match;
end prefixCref;
 
public function prefixToCref
  input Prefix inPrefix;
  output DAE.ComponentRef outCref;
algorithm
  outCref := match(inPrefix)
    local
      String name;
      Prefix rest_prefix;
      DAE.ComponentRef cref;

    case (InstTypes.PREFIX(name = name,
        restPrefix = InstTypes.EMPTY_PREFIX(classPath = _)))
      then
        DAE.CREF_IDENT(name, DAE.T_UNKNOWN_DEFAULT, {});

    case (InstTypes.PREFIX(name = name, restPrefix = rest_prefix))
      equation
        cref = DAE.CREF_IDENT(name, DAE.T_UNKNOWN_DEFAULT, {});
      then
        prefixCref(cref, rest_prefix);

  end match;
end prefixToCref;

public function prefixPath
  input Absyn.Path inPath;
  input Prefix inPrefix;
  output Absyn.Path outPath;
algorithm
  outPath := match(inPath, inPrefix)
    local
      String name;
      Prefix rest_prefix;
      Absyn.Path path;

    case (_, InstTypes.EMPTY_PREFIX(classPath = _)) then inPath;
    case (_, InstTypes.PREFIX(name = name,
        restPrefix = InstTypes.EMPTY_PREFIX(classPath = _)))
      then
        Absyn.QUALIFIED(name, inPath);

    case (_, InstTypes.PREFIX(name = name, restPrefix = rest_prefix))
      equation
        path = Absyn.QUALIFIED(name, inPath);
      then
        prefixPath(path, rest_prefix);

  end match;
end prefixPath;

public function prefixToPath
  input Prefix inPrefix;
  output Absyn.Path outPath;
algorithm
  outPath := match(inPrefix)
    local
      String name;
      Prefix rest_prefix;
      Absyn.Path path;

    case InstTypes.PREFIX(name = name,
        restPrefix = InstTypes.EMPTY_PREFIX(classPath = _))
      then Absyn.IDENT(name);

    case InstTypes.PREFIX(name = name, restPrefix = rest_prefix)
      equation
        path = Absyn.IDENT(name);
      then
        prefixPath(path, rest_prefix);

  end match;
end prefixToPath;

public function pathPrefix
  input Absyn.Path inPath;
  output Prefix outPrefix;
algorithm
  outPrefix := pathPrefix2(inPath, InstTypes.emptyPrefix);
end pathPrefix;

protected function pathPrefix2
  input Absyn.Path inPath;
  input Prefix inPrefix;
  output Prefix outPrefix;
algorithm
  outPrefix := match(inPath, inPrefix)
    local
      Absyn.Path path;
      String name;

    case (Absyn.QUALIFIED(name, path), _)
      then pathPrefix2(path, InstTypes.PREFIX(name, {}, inPrefix));

    case (Absyn.IDENT(name), _)
      then InstTypes.PREFIX(name, {}, inPrefix);

    case (Absyn.FULLYQUALIFIED(path), _)
      then pathPrefix2(path, inPrefix);

  end match;
end pathPrefix2;

public function envPrefix
  input Env inEnv;
  output Prefix outPrefix;
algorithm
  outPrefix := matchcontinue(inEnv)
    local
      Absyn.Path path;

    case _
      equation
        path = SCodeEnv.getEnvPath(inEnv);
      then
        pathPrefix(path);

    else InstTypes.emptyPrefix;

  end matchcontinue;
end envPrefix;

public function prefixElement
  input Element inElement;
  input Prefix inPrefix;
  output Element outElement;
algorithm
  outElement := match(inElement, inPrefix)
    local
      Component comp;
      Class cls;
      Absyn.Path bc;
      DAE.Type ty;

    case (InstTypes.ELEMENT(comp, cls), _)
      equation
        comp = prefixComponent(comp, inPrefix);
        cls = prefixClass(cls, inPrefix);
      then
        InstTypes.ELEMENT(comp, cls);

    case (InstTypes.CONDITIONAL_ELEMENT(comp), _)
      equation
        comp = prefixComponent(comp, inPrefix);
      then
        InstTypes.CONDITIONAL_ELEMENT(comp);

  end match;
end prefixElement;

public function prefixComponent
  input Component inComponent;
  input Prefix inPrefix;
  output Component outComponent;
protected
  Absyn.Path name;
algorithm
  name := getComponentName(inComponent);
  name := prefixPath(name, inPrefix);
  outComponent := setComponentName(inComponent, name);
end prefixComponent;
        
public function prefixClass
  input Class inClass;
  input Prefix inPrefix;
  output Class outClass;
algorithm
  outClass := match(inClass, inPrefix)
    local
      list<Element> comps;
      list<Equation> eq, ieq;
      list<list<Statement>> al, ial;
      Absyn.Path name;

    case (InstTypes.COMPLEX_CLASS(name, comps, eq, ieq, al, ial), _)
      equation
        comps = List.map1(comps, prefixElement, inPrefix);
      then
        InstTypes.COMPLEX_CLASS(name, comps, eq, ieq, al, ial);

    else inClass;

  end match;
end prefixClass;

public function countElementsInClass
  input Class inClass;
  output Integer outElements;
algorithm
  outElements := match(inClass)
    local
      list<Element> comps;
      Integer count;

    case InstTypes.BASIC_TYPE(_) then 0;

    case InstTypes.COMPLEX_CLASS(components = comps)
      equation
        count = List.fold(comps, countElementsInElement, 0);
      then
        count;

  end match;
end countElementsInClass;

public function countElementsInElement
  input Element inElement;
  input Integer inCount;
  output Integer outCount;
algorithm
  outCount := match(inElement, inCount)
    local
      Class cls;

    case (InstTypes.ELEMENT(cls = cls), _)
      then 1 + countElementsInClass(cls) + inCount;

    case (InstTypes.CONDITIONAL_ELEMENT(component = _), _)
      then 1 + inCount;

  end match;
end countElementsInElement;

public function removeCrefOuterPrefix
  input Absyn.Path inInnerPath;
  input DAE.ComponentRef inOuterCref;
  output DAE.ComponentRef outInnerCref;
algorithm
  outInnerCref := match(inInnerPath, inOuterCref)
    local
      Absyn.Path path;
      DAE.ComponentRef cref;
      String id, err_msg;
      DAE.Type ty;
      list<DAE.Subscript> subs;

    case (Absyn.IDENT(name = _), _)
      equation
        cref = ComponentReference.crefLastCref(inOuterCref);
      then
        cref;

    case (Absyn.QUALIFIED(path = path), DAE.CREF_QUAL(id, ty, subs, cref))
      equation
        cref = removeCrefOuterPrefix(path, cref);
      then
        DAE.CREF_QUAL(id, ty, subs, cref);

    else
      equation
        true = Flags.isSet(Flags.FAILTRACE);
        err_msg = "SCodeInst.removeCrefOuterPrefix failed on inner path " +&
          Absyn.pathString(inInnerPath) +& " and outer cref " +&
          ComponentReference.printComponentRefStr(inOuterCref);
        Debug.traceln(err_msg);
      then
        fail();

  end match;
end removeCrefOuterPrefix;

public function replaceCrefOuterPrefix
  input DAE.ComponentRef inCref;
  input SymbolTable inSymbolTable;
  output DAE.ComponentRef outCref;
  output SymbolTable outSymbolTable;
algorithm
  (outCref, outSymbolTable) := match(inCref, inSymbolTable)
    local
      DAE.ComponentRef prefix_cref, rest_cref, cref;
      SymbolTable st;
    
    case (_, st)
      equation
        (prefix_cref, rest_cref) = ComponentReference.splitCrefLast(inCref);
        (cref, st) = replaceCrefOuterPrefix2(prefix_cref, rest_cref, st);
      then
        (cref, st);
        
  end match;
end replaceCrefOuterPrefix;

protected function replaceCrefOuterPrefix2
  input DAE.ComponentRef inPrefixCref;
  input DAE.ComponentRef inSuffixCref;
  input SymbolTable inSymbolTable;
  output DAE.ComponentRef outNewCref;
  output SymbolTable outSymbolTable;
algorithm
  (outNewCref, outSymbolTable) :=
  matchcontinue(inPrefixCref, inSuffixCref, inSymbolTable)
    local
      Absyn.Path inner_name;
      Component comp;
      SymbolTable st;
      DAE.ComponentRef inner_cref, new_cref, prefix_cref, rest_cref;

    case (_, _, st)
      equation
        comp = InstSymbolTable.lookupCref(inPrefixCref, st);
        (inner_name, _, st) = InstSymbolTable.updateInnerReference(comp, st);
        inner_cref = removeCrefOuterPrefix(inner_name, inPrefixCref);
        new_cref = ComponentReference.joinCrefs(inner_cref, inSuffixCref);
      then
        (new_cref, st);

    case (_, _, st)
      equation
        (prefix_cref, rest_cref) = ComponentReference.splitCrefLast(inPrefixCref);
        rest_cref = ComponentReference.joinCrefs(rest_cref, inSuffixCref);
        (new_cref, st) = replaceCrefOuterPrefix2(prefix_cref, rest_cref, st);
      then
        (new_cref, st);
         
  end matchcontinue;
end replaceCrefOuterPrefix2;

public function isInnerComponent
  input Component inComponent;
  output Boolean outIsInner;
algorithm
  outIsInner := match(inComponent)
    local
      SCode.Element el;
      Absyn.InnerOuter io;

    case InstTypes.UNTYPED_COMPONENT(prefixes = InstTypes.PREFIXES(innerOuter = io))
      then Absyn.isInner(io);

    case InstTypes.TYPED_COMPONENT(prefixes = InstTypes.DAE_PREFIXES(innerOuter = io))
      then Absyn.isInner(io);

    case InstTypes.CONDITIONAL_COMPONENT(element = el)
      then SCode.isInnerComponent(el);
        
    else false;
  end match;
end isInnerComponent;

public function isConnectorComponent
  input Component inComponent;
  output Boolean outIsConnector;
algorithm
  outIsConnector := match(inComponent)
    local
      DAE.Type ty;

    case InstTypes.TYPED_COMPONENT(ty = ty)
      equation
        ty = arrayElementType(ty);
      then
        Types.isConnector(ty);

    case InstTypes.UNTYPED_COMPONENT(baseType = ty)
      then Types.isConnector(ty);

    else
      equation
        Error.addMessage(Error.INTERNAL_ERROR,
          {"InstUtil.isConnectorComponent: Unknown component\n"});
      then
        fail();

  end match;
end isConnectorComponent;

replaceable type TraverseArgType subtypeof Any;

partial function TraverseFuncType
  input Component inComponent;
  input TraverseArgType inArg;
  output Component outComponent;
  output TraverseArgType outArg;
end TraverseFuncType;

public function traverseClassComponents
  input Class inClass;
  input TraverseArgType inArg;
  input TraverseFuncType inFunc;
  output Class outClass;
  output TraverseArgType outArg;
algorithm
  (outClass, outArg) := match(inClass, inArg, inFunc)
    local
      TraverseArgType arg;
      list<Element> comps;
      list<Equation> eq, ieq;
      list<list<Statement>> al, ial;
      Absyn.Path name;

    case (InstTypes.COMPLEX_CLASS(name, comps, eq, ieq, al, ial), arg, _)
      equation
        (comps, arg) = traverseClassComponents2(comps, arg, inFunc, {});
      then
        (InstTypes.COMPLEX_CLASS(name, comps, eq, ieq, al, ial), arg);

    else (inClass, inArg);

  end match;
end traverseClassComponents;

protected function traverseClassComponents2
  input list<Element> inElements;
  input TraverseArgType inArg;
  input TraverseFuncType inFunc;
  input list<Element> inAccumEl;
  output list<Element> outElements;
  output TraverseArgType outArg;
algorithm
  (outElements, outArg) := match(inElements, inArg, inFunc, inAccumEl)
    local
      Element el;
      list<Element> rest_el;
      TraverseArgType arg;

    case (el :: rest_el, arg, _, _)
      equation
        (el, arg) = traverseClassElement(el, inArg, inFunc);
        (rest_el, arg) = traverseClassComponents2(rest_el, arg, inFunc, el :: inAccumEl);
      then
        (rest_el, arg);

    else (listReverse(inAccumEl), inArg);

  end match;
end traverseClassComponents2;
  
protected function traverseClassElement
  input Element inElement;
  input TraverseArgType inArg;
  input TraverseFuncType inFunc;
  output Element outElement;
  output TraverseArgType outArg;
algorithm
  (outElement, outArg) := match(inElement, inArg, inFunc)
    local
      Component comp;
      Class cls;
      Absyn.Path bc;
      DAE.Type ty;
      TraverseArgType arg;

    case (InstTypes.ELEMENT(comp, cls), arg, _)
      equation
        (comp, arg) = inFunc(comp, arg);
        (cls, arg) = traverseClassComponents(cls, arg, inFunc);
      then
        (InstTypes.ELEMENT(comp, cls), arg);

    case (InstTypes.CONDITIONAL_ELEMENT(comp), arg, _)
      equation
        (comp, arg) = inFunc(comp, arg);
      then
        (InstTypes.CONDITIONAL_ELEMENT(comp), arg);

    case (InstTypes.EXTENDED_ELEMENTS(bc, cls, ty), arg, _)
      equation
        (cls, arg) = traverseClassComponents(cls, arg, inFunc);
      then
        (InstTypes.EXTENDED_ELEMENTS(bc, cls, ty), arg);

  end match;
end traverseClassElement;

public function paramTypeFromPrefixes
  input Prefixes inPrefixes;
  output ParamType outParamType;
algorithm
  outParamType := match(inPrefixes)
    case InstTypes.PREFIXES(variability = SCode.PARAM())
      then InstTypes.NON_STRUCT_PARAM();

    else InstTypes.NON_PARAM();

  end match;
end paramTypeFromPrefixes;

public function translatePrefixes
  input Prefixes inPrefixes;
  output DaePrefixes outPrefixes;
algorithm
  outPrefixes := match(inPrefixes)
    local
      SCode.Visibility vis1;
      DAE.VarVisibility vis2;
      SCode.Variability var1;
      DAE.VarKind var2;
      SCode.Final fp;
      Absyn.InnerOuter io;
      Absyn.Direction dir1;
      DAE.VarDirection dir2;
      SCode.ConnectorType ct1;
      DAE.ConnectorType ct2;

    case InstTypes.NO_PREFIXES() then InstTypes.NO_DAE_PREFIXES();
    case InstTypes.PREFIXES(vis1, var1, fp, io, (dir1, _), (ct1, _), _)
      equation
        vis2 = translateVisibility(vis1);
        var2 = translateVariability(var1);
        dir2 = translateDirection(dir1);
        ct2 = translateConnectorType(ct1);
      then
        InstTypes.DAE_PREFIXES(vis2, var2, fp, io, dir2, ct2);

  end match;
end translatePrefixes;

public function translateVisibility
  input SCode.Visibility inVisibility;
  output DAE.VarVisibility outVisibility;
algorithm
  outVisibility := match(inVisibility)
    case SCode.PUBLIC() then DAE.PUBLIC();
    else DAE.PROTECTED();
  end match;
end translateVisibility;

public function translateVariability
  input SCode.Variability inVariability;
  output DAE.VarKind outVariability;
algorithm
  outVariability := match(inVariability)
    case SCode.VAR() then DAE.VARIABLE();
    case SCode.PARAM() then DAE.PARAM();
    case SCode.CONST() then DAE.CONST();
    case SCode.DISCRETE() then DAE.DISCRETE();
  end match;
end translateVariability;

public function translateDirection
  input Absyn.Direction inDirection;
  output DAE.VarDirection outDirection;
algorithm
  outDirection := match(inDirection)
    case Absyn.BIDIR() then DAE.BIDIR();
    case Absyn.OUTPUT() then DAE.OUTPUT();
    case Absyn.INPUT() then DAE.INPUT();
  end match;
end translateDirection;

public function translateConnectorType
  input SCode.ConnectorType inConnectorType;
  output DAE.ConnectorType outConnectorType;
algorithm
  outConnectorType := match(inConnectorType)
    case SCode.FLOW() then DAE.FLOW();
    case SCode.STREAM() then DAE.STREAM();
    else DAE.NON_CONNECTOR();
  end match;
end translateConnectorType;

public function conditionTrue
  input Condition inCondition;
  output Boolean outCondition;
algorithm
  outCondition := matchcontinue(inCondition)
    local
      Boolean cond;
      list<Condition> condl;

    case InstTypes.SINGLE_CONDITION(condition = cond) then cond;
    case InstTypes.ARRAY_CONDITION(conditions = condl)
      equation
        _ = List.selectFirst(condl, conditionFalse);
      then
        false;

    else true;
  end matchcontinue;
end conditionTrue;

public function conditionFalse
  input Condition inCondition;
  output Boolean outCondition;
algorithm
  outCondition := matchcontinue(inCondition)
    local
      Boolean cond;
      list<Condition> condl;

    case InstTypes.SINGLE_CONDITION(condition = cond) then not cond;
    case InstTypes.ARRAY_CONDITION(conditions = condl)
      equation
        _ = List.selectFirst(condl, conditionTrue);
      then
        false;

    else true;
  end matchcontinue;
end conditionFalse;

public function isArrayAllocation
  input InstTypes.Statement stmt;
  output Boolean b;
algorithm
  b := match stmt case InstTypes.FUNCTION_ARRAY_INIT(name=_) then true; else false; end match;
end isArrayAllocation;

public function isFlowComponent
  input Component inComponent;
  output Boolean outIsFlow;
algorithm
  outIsFlow := match(inComponent)
    case InstTypes.UNTYPED_COMPONENT(prefixes =
      InstTypes.PREFIXES(connectorType = (SCode.FLOW(), _))) then true;
    case InstTypes.TYPED_COMPONENT(prefixes =
      InstTypes.DAE_PREFIXES(connectorType = DAE.FLOW())) then true;
    else false;
  end match;
end isFlowComponent;
  
public function getFunctionInputs
  input Function inFunction;
  output list<Element> outInputs;
algorithm
  outInputs := match(inFunction)
    local
      list<Element> inputs;

    case InstTypes.FUNCTION(inputs = inputs) then inputs;
    case InstTypes.RECORD(components = inputs) then inputs;

  end match;
end getFunctionInputs;

public function getComponentParent
  input Component inComponent;
  output Option<Component> outParent;
algorithm
  outParent := match(inComponent)
    local
      Option<Component> parent;

    case InstTypes.TYPED_COMPONENT(parent = parent) then parent;
    case InstTypes.PACKAGE(parent = parent) then parent;
    else NONE();

  end match;
end getComponentParent;

public function setComponentParent
  input Component inComponent;
  input Option<Component> inParent;
  output Component outComponent;
algorithm
  outComponent := matchcontinue(inComponent, inParent)
    local
      Absyn.Path name;
      DAE.Type ty;
      DaePrefixes pref;
      Binding binding;
      Absyn.Info info;
    
    case (_, NONE()) then inComponent; 
    
    case (InstTypes.TYPED_COMPONENT(name, ty, _, pref, binding, info), _)
    then InstTypes.TYPED_COMPONENT(name, ty, inParent, pref, binding, info);
    
    case (InstTypes.PACKAGE(name, _), _)
    then InstTypes.PACKAGE(name, inParent);
    
    case (_, _) then inComponent;
    
  end matchcontinue;
end setComponentParent;
      
public function makeTypedComponentCref
  input Component inComponent;
  output DAE.ComponentRef outCref;
algorithm
  outCref := match(inComponent)
    local
      Absyn.Path name;
      DAE.ComponentRef cref;

    case InstTypes.TYPED_COMPONENT(name = name)
      equation
        (cref, _) = makeTypedComponentCref2(name, inComponent);
      then
        cref;

    else
      equation
        true = Flags.isSet(Flags.FAILTRACE);
        Debug.trace("- InstUtil.makeTypedComponentCref failed on component ");
        Debug.traceln(InstDump.componentStr(inComponent));
      then
        fail();

  end match;
end makeTypedComponentCref;

protected function makeTypedComponentCref2
  input Absyn.Path inPath;
  input Component inComponent;
  output DAE.ComponentRef outCref;
  output Option<Component> outParent;
algorithm
  (outCref, outParent) := match(inPath, inComponent)
    local
      Absyn.Ident id;
      Absyn.Path rest_path;
      DAE.ComponentRef cref;
      DAE.Type ty;
      Option<Component> parent;

    case (Absyn.QUALIFIED(name = id, path = rest_path), _)
      equation
        (cref, SOME(InstTypes.TYPED_COMPONENT(ty = ty, parent = parent))) =
          makeTypedComponentCref2(rest_path, inComponent);
      then
        (DAE.CREF_QUAL(id, ty, {}, cref), parent);

    case (Absyn.IDENT(name = id),
        InstTypes.TYPED_COMPONENT(ty = ty, parent = parent))
      then (DAE.CREF_IDENT(id, ty, {}), parent);

    else
      equation
        true = Flags.isSet(Flags.FAILTRACE);
        Debug.trace("- InstUtil.makeTypedComponentCref2 failed on path ");
        Debug.traceln(Absyn.pathString(inPath));
      then
        fail();

  end match;
end makeTypedComponentCref2;

public function typeCrefWithComponent
  input DAE.ComponentRef inCref;
  input Component inComponent;
  output DAE.ComponentRef outCref;
algorithm
  (outCref, _) := typeCrefWithComponent2(inCref, inComponent);
end typeCrefWithComponent;

protected function typeCrefWithComponent2
  input DAE.ComponentRef inCref;
  input Component inComponent;
  output DAE.ComponentRef outCref;
  output Option<Component> outParent;
algorithm
  (outCref, outParent) := match(inCref, inComponent)
    local
      Option<Component> parent;
      DAE.Ident id;
      DAE.Type ty;
      list<DAE.Subscript> subs;
      DAE.ComponentRef rest_cref;

    case (DAE.CREF_IDENT(id, _, subs),
        InstTypes.TYPED_COMPONENT(ty = ty, parent = parent))
      then (DAE.CREF_IDENT(id, ty, subs), parent);

    case (DAE.CREF_QUAL(id, _, subs, rest_cref), _)
      equation
        (rest_cref, SOME(InstTypes.TYPED_COMPONENT(ty = ty, parent = parent))) =
          typeCrefWithComponent2(rest_cref, inComponent);
      then
        (DAE.CREF_QUAL(id, ty, subs, rest_cref), parent);

    else
      equation
        true = Flags.isSet(Flags.FAILTRACE);
        Debug.trace("- InstUtil.typeCrefWithComponent2 failed on cref ");
        Debug.trace(ComponentReference.printComponentRefStr(inCref));
        Debug.trace(" and component ");
        Debug.traceln(InstDump.componentStr(inComponent));
      then
        fail();

  end match;
end typeCrefWithComponent2;

public function toConst
"Translates SCode.Variability to DAE.Const"
input SCode.Variability inVar;
output DAE.Const outConst;
algorithm
  outConst := matchcontinue (inVar)
    case(SCode.CONST()) then DAE.C_CONST();
    case(SCode.PARAM()) then DAE.C_PARAM();
    case _ then DAE.C_VAR();
  end matchcontinue;
end toConst;

public function setClassName
  input Class inClass;
  input Absyn.Path inClassName;
  output Class outClass;
algorithm
  outClass := match(inClass, inClassName)
    local
      list<Element> el;
      list<Equation> eq, ieq;
      list<list<Statement>> al, ial;
      Absyn.Path name;

    case (InstTypes.COMPLEX_CLASS(_, el, eq, ieq, al, ial), _)
      then
        InstTypes.COMPLEX_CLASS(inClassName, el, eq, ieq, al, ial);

    case (InstTypes.BASIC_TYPE(_), _)
      then
        InstTypes.BASIC_TYPE(inClassName);

  end match;
end setClassName;

public function getClassName
  input Class inClass;
  output Absyn.Path outClassName;
algorithm
  outClassName := match(inClass)
    local
      Absyn.Path name;

    case (InstTypes.COMPLEX_CLASS(name = name))
      then
        name;

    case (InstTypes.BASIC_TYPE(name))
      then
        name;

  end match;
end getClassName;

public function prefixToStr
  input Prefix inPrefix;
  output String outStr;
algorithm
  outStr := match(inPrefix)
    local
      String name, str;
      Prefix rest_prefix;
      Absyn.Path path;

    case InstTypes.EMPTY_PREFIX(classPath = NONE()) then "E()";
    
    case InstTypes.EMPTY_PREFIX(classPath = SOME(path))
      equation
        str = "E(" +& Absyn.pathLastIdent(path) +& ")";
      then
        str;
    
    case InstTypes.PREFIX(name = name, restPrefix = rest_prefix)
      equation
        str = prefixToStr(rest_prefix) +& "." +& name;
      then
        str;

  end match;
end prefixToStr;

public function prefixToStrNoEmpty
  input Prefix inPrefix;
  output String outStr;
algorithm
  outStr := match(inPrefix)
    local
      String name, str;
      Prefix rest_prefix;
      Absyn.Path path;

    case InstTypes.EMPTY_PREFIX(classPath = _) then "";

    case InstTypes.PREFIX(name = name, 
         restPrefix = InstTypes.EMPTY_PREFIX(classPath = _))
      then
        name;

    case InstTypes.PREFIX(name = name, restPrefix = rest_prefix)
      equation
        str = prefixToStrNoEmpty(rest_prefix) +& "." +& name;
      then
        str;

  end match;
end prefixToStrNoEmpty;

public function prefixFirstName
  input Prefix inPrefix;
  output String outStr;
algorithm
  outStr := match(inPrefix)
    local
      String name, str;
      Prefix rest_prefix;
      Absyn.Path path;

    case InstTypes.EMPTY_PREFIX(classPath = _) then "";
    case InstTypes.PREFIX(name = name) then name;

  end match; 
end prefixFirstName;

end InstUtil;
