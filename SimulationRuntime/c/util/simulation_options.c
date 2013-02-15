/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-CurrentYear, Open Source Modelica Consortium (OSMC),
 * c/o Linköpings universitet, Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF THE BSD NEW LICENSE OR THE
 * GPL VERSION 3 LICENSE OR THE OSMC PUBLIC LICENSE (OSMC-PL) VERSION 1.2.
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES
 * RECIPIENT'S ACCEPTANCE OF THE OSMC PUBLIC LICENSE OR THE GPL VERSION 3,
 * ACCORDING TO RECIPIENTS CHOICE.
 * 
 * The OpenModelica software and the OSMC (Open Source Modelica Consortium)
 * Public License (OSMC-PL) are obtained from OSMC, either from the above
 * address, from the URLs: http://www.openmodelica.org or
 * http://www.ida.liu.se/projects/OpenModelica, and in the OpenModelica
 * distribution. GNU version 3 is obtained from:
 * http://www.gnu.org/copyleft/gpl.html. The New BSD License is obtained from:
 * http://www.opensource.org/licenses/BSD-3-Clause.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without even the implied
 * warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE, EXCEPT AS
 * EXPRESSLY SET FORTH IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE
 * CONDITIONS OF OSMC-PL.
 *
 */

#include "simulation_options.h"

const char *FLAG_NAME[FLAG_MAX] = {
  "LOG_UNKNOWN",
  
  /* FLAG_CPU */                   "cpu",
  /* FLAG_F */                     "f",
  /* FLAG_HELP */                  "help",
  /* FLAG_IIF */                   "iif",
  /* FLAG_IIM */                   "iim",
  /* FLAG_IIT */                   "iit",
  /* FLAG_ILS */                   "ils",
  /* FLAG_INTERACTIVE */           "interactive",
  /* FLAG_IOM */                   "iom",
  /* FLAG_L */                     "l",
  /* FLAG_LV */                    "lv",
  /* FLAG_MEASURETIMEPLOTFORMAT */ "measureTimePlotFormat",
  /* FLAG_NLS */                   "nls",
  /* FLAG_NOEMIT */                "noemit",
  /* FLAG_OUTPUT */                "output",
  /* FLAG_OVERRIDE */              "override",
  /* FLAG_OVERRIDE_FILE */         "overrideFile",
  /* FLAG_PORT */                  "port",
  /* FLAG_R */                     "r",
  /* FLAG_S */                     "s",
  /* FLAG_W */                     "w"
};

const char *FLAG_DESC[FLAG_MAX] = {
  "unknown",

  /* FLAG_CPU */                   "dumps the cpu-time into the results-file",
  /* FLAG_F */                     "value specifies a new setup XML file to the generated simulation code",
  /* FLAG_HELP */                  "get deteiled information the specifies the command-line flag",
  /* FLAG_IIF */                   "value specifies an external file for the initialization of the model",
  /* FLAG_IIM */                   "value specifies the initialization method",
  /* FLAG_IIT */                   "[double] value specifies a time for the initialization of the model",
  /* FLAG_ILS */                   "[int] value specifies the number of steps for the global homotopy method (required: -iim=numeric -iom=nelder_mead_ex)",
  /* FLAG_INTERACTIVE */           "specify interactive simulation",
  /* FLAG_IOM */                   "value specifies the initialization optimization method",
  /* FLAG_L */                     "value specifies a time where the linearization of the model should be performed",
  /* FLAG_LV */                    "[string list] value specifies the logging level",
  /* FLAG_MEASURETIMEPLOTFORMAT */ "value specifies the output format of the measure time functionality",
  /* FLAG_NLS */                   "value specifies the nonlinear solver",
  /* FLAG_NOEMIT */                "do not emit any results to the result file",
  /* FLAG_OUTPUT */                "output the variables a, b and c at the end of the simulation to the standard output",
  /* FLAG_OVERRIDE */              "override the variables or the simulation settings in the XML setup file",
  /* FLAG_OVERRIDE_FILE */         "will override the variables or the simulation settings in the XML setup file with the values from the file",
  /* FLAG_PORT */                  "value specifies interactive simulation port",
  /* FLAG_R */                     "value specifies a new result file than the default Model_res.mat",
  /* FLAG_S */                     "value specifies the solver",
  /* FLAG_W */                     "shows all warnings even if a related log-stream is inactive"
};

const char *FLAG_DETAILED_DESC[FLAG_MAX] = {
  "unknown",

  /* FLAG_CPU */                   "  - dumps the cpu-time into the result-file\n  - $cpuTime is the variable name inside the result-file",
  /* FLAG_F */                     "value specifies a new setup XML file to the generated simulation code",
  /* FLAG_HELP */                  "get deteiled information the specifies the command-line flag\n  e.g. -help=f prints detaild information for command-line flag f",
  /* FLAG_IIF */                   "value specifies an external file for the initialization of the model",
  /* FLAG_IIM */                   "value specifies the initialization method\n  none\n  numeric\n  symbolic",
  /* FLAG_IIT */                   "value specifies a time for the initialization of the model",
  /* FLAG_ILS */                   "value specifies the number of steps for the global homotopy method (required: -iim=numeric -iom=nelder_mead_ex)",
  /* FLAG_INTERACTIVE */           "specify interactive simulation",
  /* FLAG_IOM */                   "value specifies the initialization optimization method\n  nelder_mead_ex\n  nelder_mead_ex2\n  simplex\n  newuoa",
  /* FLAG_JAC */                   "specify jacobian",
  /* FLAG_L */                     "value specifies a time where the linearization of the model should be performed",
  /* FLAG_LV */                    "value specifies the logging level",
  /* FLAG_MEASURETIMEPLOTFORMAT */ "value specifies the output format of the measure time functionality\n  svg\n  jpg\n  ps\n  gif\n  ...",
  /* FLAG_NLS */                   "value specifies the nonlinear solver",
  /* FLAG_NOEMIT */                "do not emit any results to the result file",
  /* FLAG_NUMJAC */                "specify numerical jacobian",                                                                                                      
  /* FLAG_OUTPUT */                "output the variables a, b and c at the end of the simulation to the standard output\n  time = value, a = value, b = value, c = value",
  /* FLAG_OVERRIDE */              "override the variables or the simulation settings in the XML setup file\n  e.g. var1=start1,var2=start2,par3=start3,startTime=val1,stopTime=val2,stepSize=val3,\n       tolerance=val4,solver=\"see -s\",outputFormat=\"mat|plt|csv|empty\",variableFilter=\"filter\"",
  /* FLAG_OVERRIDE_FILE */         "will override the variables or the simulation settings in the XML setup file with the values from the file\n  note that: -overrideFile CANNOT be used with -override\n  use when variables for -override are too many and do not fit in command line size\n  overrideFileName contains lines of the form: var1=start1",
  /* FLAG_PORT */                  "value specifies interactive simulation port",
  /* FLAG_R */                     "value specifies a new result file than the default Model_res.mat",
  /* FLAG_S */                     "value specifies the solver\n  dassl\n  euler\n  rungekutta\n  inline-euler\n  inline-rungekutta\n  dasslwort\n  dasslSymJac\n  dasslNumJac\n  dasslColorSymJac\n  dasslInternalNumJac\n  qss",
  /* FLAG_W */                     "shows all warnings even if a related log-stream is inactive"
};

const int FLAG_TYPE[FLAG_MAX] = {
  FLAG_TYPE_UNKNOWN,
  
  /* FLAG_CPU */                   FLAG_TYPE_FLAG,
  /* FLAG_F */                     FLAG_TYPE_OPTION,
  /* FLAG_HELP */                  FLAG_TYPE_OPTION,
  /* FLAG_IIF */                   FLAG_TYPE_OPTION,
  /* FLAG_IIM */                   FLAG_TYPE_OPTION,
  /* FLAG_IIT */                   FLAG_TYPE_OPTION,
  /* FLAG_ILS */                   FLAG_TYPE_OPTION,
  /* FLAG_INTERACTIVE */           FLAG_TYPE_FLAG,
  /* FLAG_IOM */                   FLAG_TYPE_OPTION,
  /* FLAG_JAC */                   FLAG_TYPE_FLAG,
  /* FLAG_L */                     FLAG_TYPE_OPTION,
  /* FLAG_LV */                    FLAG_TYPE_OPTION,
  /* FLAG_MEASURETIMEPLOTFORMAT */ FLAG_TYPE_OPTION,
  /* FLAG_NLS */                   FLAG_TYPE_OPTION,
  /* FLAG_NOEMIT */                FLAG_TYPE_FLAG,
  /* FLAG_NUMJAC */                FLAG_TYPE_FLAG,
  /* FLAG_OUTPUT */                FLAG_TYPE_FLAG_VALUE,
  /* FLAG_OVERRIDE */              FLAG_TYPE_FLAG_VALUE,
  /* FLAG_OVERRIDE_FILE */         FLAG_TYPE_FLAG_VALUE,
  /* FLAG_PORT */                  FLAG_TYPE_OPTION,
  /* FLAG_R */                     FLAG_TYPE_OPTION,
  /* FLAG_S */                     FLAG_TYPE_OPTION,
  /* FLAG_W */                     FLAG_TYPE_FLAG
};


