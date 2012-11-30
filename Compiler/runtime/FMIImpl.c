/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-CurrentYear, Open Source Modelica Consortium (OSMC),
 * c/o Linköpings universitet, Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3 LICENSE OR
 * THIS OSMC PUBLIC LICENSE (OSMC-PL) VERSION 1.2.
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES RECIPIENT'S ACCEPTANCE
 * OF THE OSMC PUBLIC LICENSE OR THE GPL VERSION 3, ACCORDING TO RECIPIENTS CHOICE.
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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>

#include "systemimpl.h"
#include "errorext.h"

#define FMILIB_BUILDING_LIBRARY
#include "fmilib.h"

#define BUFFER 1000
#define FMI_DEBUG

static void importlogger(jm_callbacks* c, jm_string module, jm_log_level_enu_t log_level, jm_string message)
{
  const char* tokens[3] = {module,jm_log_level_to_string(log_level),message};
  switch (log_level) {
    case jm_log_level_fatal:
    case jm_log_level_error:
      c_add_message(-1, ErrorType_scripting, ErrorLevel_error, gettext("module = %s, log level = %s: %s"), tokens, 3);
      break;
    case jm_log_level_warning:
      c_add_message(-1, ErrorType_scripting, ErrorLevel_warning, gettext("module = %s, log level = %s: %s"), tokens, 3);
      break;
    case jm_log_level_info:
    case jm_log_level_verbose:
    case jm_log_level_debug:
      c_add_message(-1, ErrorType_scripting, ErrorLevel_notification, gettext("module = %s, log level = %s: %s"), tokens, 3);
      break;
    default:
      printf("module = %s, log level = %d: %s\n", module, log_level, message);
      break;
  }
}

/* Logger function used by the FMU internally */
static void fmilogger(fmi1_component_t c, fmi1_string_t instanceName, fmi1_status_t status, fmi1_string_t category, fmi1_string_t message, ...)
{
#ifdef FMI_DEBUG
  char msg[BUFFER];
  va_list argp;
  va_start(argp, message);
  vsprintf(msg, message, argp);
  printf("fmiStatus = %d;  %s (%s): %s\n", status, instanceName, category, msg);
#endif
}

/*
 * functions that replaces the given character old with the character new in a string
 */
void charReplace(char* variable_name, unsigned int len, char old, char new)
{
  char* res = NULL;
  res = strchr(variable_name, old);
  while (res != NULL) {
    *res = new;
    res = strchr(variable_name, old);
  }
  variable_name[len] = '\0';
}

/*
 * Reads the model variable variability.
 */
const char* getModelVariableVariability(fmi1_import_variable_t* variable)
{
  fmi1_variability_enu_t variability = fmi1_import_get_variability(variable);
  switch (variability) {
    case fmi1_variability_enu_constant:
      return "constant";
    case fmi1_variability_enu_parameter:
      return "parameter";
    case fmi1_variability_enu_discrete:
    case fmi1_variability_enu_continuous:
    case fmi1_variability_enu_unknown:
    default:
      return "";
  }
}

/*
 * Reads the model variable causality.
 */
const char* getModelVariableCausality(fmi1_import_variable_t* variable)
{
  fmi1_causality_enu_t causality = fmi1_import_get_causality(variable);
  switch (causality) {
    case fmi1_causality_enu_input:
      return "input";
    case fmi1_causality_enu_output:
      return "output";
    case fmi1_causality_enu_internal:
    case fmi1_causality_enu_none:
    case fmi1_causality_enu_unknown:
    default:
      return "";
  }
}

/*
 * Reads the model variable base type.
 */
const char* getModelVariableBaseType(fmi1_import_variable_t* variable)
{
  fmi1_base_type_enu_t type = fmi1_import_get_variable_base_type(variable);
  switch (type) {
    case fmi1_base_type_real:
      return "Real";
    case fmi1_base_type_int:
      return "Integer";
    case fmi1_base_type_bool:
      return "Boolean";
    case fmi1_base_type_str:
      return "String";
    case fmi1_base_type_enum:
      return "enumeration";
    default:                    /* Should never be reached. */
      return "";
  }
}

/*
 * Reads the model variable name. Returns a malloc'd string that should be
 * free'd.
 */
char* getModelVariableName(fmi1_import_variable_t* variable)
{
  const char* name = fmi1_import_get_variable_name(variable);
  char* res = strdup(name);
  int length = strlen(res);

  charReplace(res, length, '.', '_');
  charReplace(res, length, '[', '_');
  charReplace(res, length, ']', '_');
  charReplace(res, length, ',', '_');
  charReplace(res, length, '(', '_');
  charReplace(res, length, ')', '_');
  return res;
}

/*
 * Reads the model variable start value.
 */
void* getModelVariableStartValue(fmi1_import_variable_t* variable, int hasStartValue)
{
  fmi1_base_type_enu_t type = fmi1_import_get_variable_base_type(variable);
  fmi1_import_real_variable_t* fmiRealModelVariable;
  fmi1_import_integer_variable_t* fmiIntegerModelVariable;
  fmi1_import_bool_variable_t* fmiBooleanModelVariable;
  fmi1_import_string_variable_t* fmiStringModelVariable;
  fmi1_import_enum_variable_t* fmiEnumerationModelVariable;
  switch (type) {
    case fmi1_base_type_real:
      if (!hasStartValue) return mk_rcon(0);
      fmiRealModelVariable = fmi1_import_get_variable_as_real(variable);
      return fmiRealModelVariable ? mk_rcon(fmi1_import_get_real_variable_start(fmiRealModelVariable)) : mk_rcon(0);
    case fmi1_base_type_int:
      if (!hasStartValue) return mk_icon(0);
      fmiIntegerModelVariable = fmi1_import_get_variable_as_integer(variable);
      return fmiIntegerModelVariable ? mk_icon(fmi1_import_get_integer_variable_start(fmiIntegerModelVariable)) : mk_icon(0);
    case fmi1_base_type_bool:
      if (!hasStartValue) return mk_bcon(0);
      fmiBooleanModelVariable = fmi1_import_get_variable_as_boolean(variable);
      return fmiBooleanModelVariable ? mk_bcon(fmi1_import_get_boolean_variable_start(fmiBooleanModelVariable)) : mk_bcon(0);
    case fmi1_base_type_str:
      if (!hasStartValue) return mk_scon("");
      fmiStringModelVariable = fmi1_import_get_variable_as_string(variable);
      return fmiStringModelVariable ? mk_scon(fmi1_import_get_string_variable_start(fmiStringModelVariable)) : mk_scon(0);
    case fmi1_base_type_enum:
      if (!hasStartValue) return mk_icon(0);
      fmiEnumerationModelVariable = fmi1_import_get_variable_as_enum(variable);
      return fmiEnumerationModelVariable ? mk_icon(fmi1_import_get_enum_variable_start(fmiEnumerationModelVariable)) : mk_icon(0);
    default:
      return 0;
  }
}

/*
 * Initializes FMI Import.
 * Reads the Model Identifier name.
 * Reads the experiment annotation.
 * Reads the model variables.
 */
int FMIImpl__initializeFMIImport(const char* file_name, const char* working_directory, int fmi_log_level, int input_connectors, int output_connectors,
    void** fmiContext, void** fmiInstance, void** fmiInfo, void** experimentAnnotation, void** modelVariablesInstance, void** modelVariablesList)
{
  *fmiContext = NULL;
  *fmiInstance = NULL;
  *fmiInfo = NULL;
  *experimentAnnotation = NULL;
  *modelVariablesInstance = NULL;
  *modelVariablesList = NULL;
  static int init = 0;
  // JM callbacks
  static jm_callbacks callbacks;
  // FMI callback functions
  static fmi1_callback_functions_t callback_functions;
  if (!init) {
    init = 1;
    callbacks.malloc = malloc;
    callbacks.calloc = calloc;
    callbacks.realloc = realloc;
    callbacks.free = free;
    callbacks.logger = importlogger;
    callbacks.log_level = fmi_log_level;
    callbacks.context = 0;
    callback_functions.logger = fmilogger;
    callback_functions.allocateMemory = calloc;
    callback_functions.freeMemory = free;
  }
  fmi_import_context_t* context = fmi_import_allocate_context(&callbacks);
  *fmiContext = mk_some(context);
  // extract the fmu file and read the version
  fmi_version_enu_t version;
  version = fmi_import_get_fmi_version(context, file_name, working_directory);
  if (version > fmi_version_1_enu) {
    fmi_import_free_context(context);
    const char* tokens[1] = {fmi_version_to_string(version)};
    c_add_message(-1, ErrorType_scripting, ErrorLevel_error, gettext("The FMU version is %s. Only version 1.0 is supported so far."), tokens, 1);
    return 0;
  }
  // parse the xml file
  fmi1_import_t* fmi;
  fmi = fmi1_import_parse_xml(context, working_directory);
  if(!fmi) {
    fmi_import_free_context(context);
    c_add_message(-1, ErrorType_scripting, ErrorLevel_error, gettext("Error parsing the modelDescription.xml file."), NULL, 0);
    return 0;
  }
  *fmiInstance = mk_some(fmi);
  // Load the binary (dll/so)
  jm_status_enu_t status;
  status = fmi1_import_create_dllfmu(fmi, callback_functions, 0);
  if (status == jm_status_error) {
    fmi1_import_free(fmi);
    fmi_import_free_context(context);
    c_add_message(-1, ErrorType_scripting, ErrorLevel_error, gettext("Could not create the DLL loading mechanism(C-API)."), NULL, 0);
    return 0;
  }
  /* Read the model name from FMU's modelDescription.xml file. */
  const char* modelName = fmi1_import_get_model_name(fmi);
  /* Read the FMI type */
  fmi1_fmu_kind_enu_t fmiType = fmi1_import_get_fmu_kind(fmi);
  /* Read the model identifier from FMU's modelDescription.xml file. */
  const char* modelIdentifier = fmi1_import_get_model_identifier(fmi);
  /* Read the FMI GUID from FMU's modelDescription.xml file. */
  const char* guid = fmi1_import_get_GUID(fmi);
  /* Read the FMI description from FMU's modelDescription.xml file. */
  const char* description = fmi1_import_get_description(fmi);
  description = (description && omc__escapedStringLength(description,0) > strlen(description)) ? omc__escapedString(description,0) : "";
  /* Read the FMI generation tool from FMU's modelDescription.xml file. */
  const char* generationTool = fmi1_import_get_generation_tool(fmi);
  /* Read the FMI generation date and time from FMU's modelDescription.xml file. */
  const char* generationDateAndTime = fmi1_import_get_generation_date_and_time(fmi);
  /* Read the FMI Variable Naming convention from FMU's modelDescription.xml file. */
  const char* namingConvention = fmi1_naming_convention_to_string(fmi1_import_get_naming_convention(fmi));
  /* Read the FMI number of continuous states from FMU's modelDescription.xml file. */
  unsigned int numberOfContinuousStates = fmi1_import_get_number_of_continuous_states(fmi);
  /* Read the FMI number of event indicators from FMU's modelDescription.xml file. */
  unsigned int numberOfEventIndicators = fmi1_import_get_number_of_event_indicators(fmi);
  /* construct continuous states list record */
  int i = 1;
  void* continuousStatesList = mk_nil();
  for (; i <= numberOfContinuousStates ; i++) {
    continuousStatesList = mk_cons(mk_icon(i), continuousStatesList);
  }
  /* construct event indicators list record */
  i = 1;
  void* eventIndicatorsList = mk_nil();
  for (; i <= numberOfEventIndicators ; i++) {
    eventIndicatorsList = mk_cons(mk_icon(i), eventIndicatorsList);
  }
  /* construct FMIINFO record */
  *fmiInfo = FMI__INFO(mk_scon(fmi_version_to_string(version)), mk_icon(fmiType), mk_scon(modelName), mk_scon(modelIdentifier), mk_scon(guid), mk_scon(description),
      mk_scon(generationTool), mk_scon(generationDateAndTime), mk_scon(namingConvention), continuousStatesList, eventIndicatorsList);
  /* Read the FMI Default Experiment Start value from FMU's modelDescription.xml file. */
  double experimentStartTime = fmi1_import_get_default_experiment_start(fmi);
  /* Read the FMI Default Experiment Stop value from FMU's modelDescription.xml file. */
  double experimentStopTime = fmi1_import_get_default_experiment_stop(fmi);
  /* Read the FMI Default Experiment Tolerance value from FMU's modelDescription.xml file. */
  double experimentTolerance = fmi1_import_get_default_experiment_tolerance(fmi);
  *experimentAnnotation = FMI__EXPERIMENTANNOTATION(mk_rcon(experimentStartTime), mk_rcon(experimentStopTime), mk_rcon(experimentTolerance));
  /* Read the model variables from the FMU's modelDescription.xml file and create a list of it. */
  fmi1_import_variable_list_t* model_variables_list = fmi1_import_get_variable_list(fmi);
  *modelVariablesInstance = mk_some(model_variables_list);
  size_t model_variables_list_size = fmi1_import_get_variable_list_size(model_variables_list);
  /* get model variables value reference list */
  const fmi1_value_reference_t* model_variables_value_reference_list = fmi1_import_get_value_referece_list(model_variables_list);
  i = 0;
  *modelVariablesList = mk_nil();
  int xInputPlacement = -120;
  int yInputPlacement = 60;
  int xOutputPlacement = 100;
  int yOutputPlacement = 60;
  for (; i < model_variables_list_size ; i++) {
    fmi1_import_variable_t* model_variable = fmi1_import_get_variable(model_variables_list, i);
    void* variable_instance = mk_icon((intptr_t)model_variable);
    char *name = getModelVariableName(model_variable);
    void* variable_name = mk_scon(name);
    free(name);
    const char* description = fmi1_import_get_variable_description(model_variable);
    description = (description && omc__escapedStringLength(description,0) > strlen(description)) ? omc__escapedString(description,0) : "";
    void* variable_description = mk_scon(description);
    const char* base_type = getModelVariableBaseType(model_variable);
    void* variable_base_type = mk_scon(base_type);
    void* variable_variability = mk_scon(getModelVariableVariability(model_variable));
    const char* causality = getModelVariableCausality(model_variable);
    void* variable_causality = mk_scon(causality);
    int hasStartValue = fmi1_import_get_variable_has_start(model_variable);
    void* variable_has_start_value = mk_bcon(hasStartValue);
    void* variable_start_value = getModelVariableStartValue(model_variable, hasStartValue);
    void* variable_is_fixed = mk_bcon(fmi1_import_get_variable_is_fixed(model_variable));
    void* variable_value_reference = mk_rcon((double)model_variables_value_reference_list[i]);
    void* variable_placement_annotation = mk_scon("");
    void* variable_x1_placement = mk_icon(0);
    void* variable_x2_placement = mk_icon(0);
    void* variable_y1_placement = mk_icon(0);
    void* variable_y2_placement = mk_icon(0);
    if ((strcmp(causality,"input") == 0) && input_connectors) {
      variable_x1_placement = mk_icon(xInputPlacement);
      variable_x2_placement = mk_icon(xInputPlacement+20);
      variable_y1_placement = mk_icon(yInputPlacement);
      variable_y2_placement = mk_icon(yInputPlacement+20);
      yInputPlacement -= 25;
    } else if ((strcmp(causality,"output") == 0) && output_connectors) {
      variable_x1_placement = mk_icon(xOutputPlacement);
      variable_x2_placement = mk_icon(xOutputPlacement+20);
      variable_y1_placement = mk_icon(yOutputPlacement);
      variable_y2_placement = mk_icon(yOutputPlacement+20);
      yOutputPlacement -= 25;
    }
    //fprintf(stderr, "%s Variable name = %s, valueReference = %d\n", getModelVariableBaseType(model_variable), getModelVariableName(model_variable), model_variables_value_reference_list[i]);fflush(NULL);
    void* variable;
    fmi1_base_type_enu_t type = fmi1_import_get_variable_base_type(model_variable);
    switch (type) {
      case fmi1_base_type_real:
        variable = FMI__REALVARIABLE(variable_instance, variable_name, variable_description, variable_base_type, variable_variability, variable_causality,
            variable_has_start_value, variable_start_value, variable_is_fixed, variable_value_reference, variable_x1_placement, variable_x2_placement, variable_y1_placement, variable_y2_placement);
        break;
      case fmi1_base_type_int:
        variable = FMI__INTEGERVARIABLE(variable_instance, variable_name, variable_description, variable_base_type, variable_variability, variable_causality,
            variable_has_start_value, variable_start_value, variable_is_fixed, variable_value_reference, variable_x1_placement, variable_x2_placement, variable_y1_placement, variable_y2_placement);
        break;
      case fmi1_base_type_bool:
        variable = FMI__BOOLEANVARIABLE(variable_instance, variable_name, variable_description, variable_base_type, variable_variability, variable_causality,
            variable_has_start_value, variable_start_value, variable_is_fixed, variable_value_reference, variable_x1_placement, variable_x2_placement, variable_y1_placement, variable_y2_placement);
        break;
      case fmi1_base_type_str:
        variable = FMI__STRINGVARIABLE(variable_instance, variable_name, variable_description, variable_base_type, variable_variability, variable_causality,
            variable_has_start_value, variable_start_value, variable_is_fixed, variable_value_reference, variable_x1_placement, variable_x2_placement, variable_y1_placement, variable_y2_placement);
        break;
      case fmi1_base_type_enum:
        variable = FMI__ENUMERATIONVARIABLE(variable_instance, variable_name, variable_description, variable_base_type, variable_variability, variable_causality,
            variable_has_start_value, variable_start_value, variable_is_fixed, variable_value_reference, variable_x1_placement, variable_x2_placement, variable_y1_placement, variable_y2_placement);
        break;
    }
    *modelVariablesList = mk_cons(variable, *modelVariablesList);
  }
  /* everything is OK return success */
  return 1;
}

/*
 * Releases all the instances of FMI Import.
 * From FMIL docs; Free a variable list. Note that variable lists are allocated dynamically and must be freed when not needed any longer.
 */
void FMIImpl__releaseFMIImport(void *ptr1, void *ptr2, void *ptr3)
{
  intptr_t fmiModeVariablesInstance = RML_FETCH(RML_OFFSET(RML_UNTAGPTR(ptr1),1));
  intptr_t fmiInstance = RML_FETCH(RML_OFFSET(RML_UNTAGPTR(ptr2),1));
  intptr_t fmiContext = RML_FETCH(RML_OFFSET(RML_UNTAGPTR(ptr3),1));
  free((fmi1_import_variable_list_t*)fmiModeVariablesInstance);
  fmi1_import_t* fmi = (fmi1_import_t*)fmiInstance;
  fmi1_import_destroy_dllfmu(fmi);
  fmi1_import_free(fmi);
  fmi_import_free_context((fmi_import_context_t*)fmiContext);
}

#ifdef __cplusplus
}
#endif
