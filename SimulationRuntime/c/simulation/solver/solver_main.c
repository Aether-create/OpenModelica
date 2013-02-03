/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-CurrentYear, Linköping University,
 * Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF THIS OSMC PUBLIC
 * LICENSE (OSMC-PL). ANY USE, REPRODUCTION OR DISTRIBUTION OF
 * THIS PROGRAM CONSTITUTES RECIPIENT'S ACCEPTANCE OF THE OSMC
 * PUBLIC LICENSE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from Linköping University, either from the above address,
 * from the URL: http://www.ida.liu.se/projects/OpenModelica
 * and in the OpenModelica distribution.
 *
 * This program is distributed  WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS
 * OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 */

#include "solver_main.h"
#include "simulation_runtime.h"
#include "openmodelica_func.h"
#include "initialization.h"
#include "nonlinearSystem.h"
#include "dassl.h"
#include "delay.h"
#include "events.h"
#include "varinfo.h"
#include "stateset.h"
#include "radau.h"

/*
 * #include "dopri45.h"
 */
#include "rtclock.h"
#include "omc_error.h"
#include <math.h>
#include <string.h>
#include <errno.h>
#include <float.h>

double** work_states;

const int rungekutta_s = 4;
const double rungekutta_b[4] = { 1.0 / 6.0, 1.0 / 3.0, 1.0 / 3.0, 1.0 / 6.0 };
const double rungekutta_c[4] = { 0.0, 0.5, 0.5, 1.0 };

typedef struct RK4
{
  double** work_states;
  int work_states_ndims;
}RK4;

#ifdef WITH_SUNDIALS
RADAUIIA rData;
KINSOLRADAU kData;
#endif

static int euler_ex_step(DATA* data, SOLVER_INFO* solverInfo);

static int rungekutta_step(DATA* data, SOLVER_INFO* solverInfo);

static int radauIIA_step(DATA* data, SOLVER_INFO* solverInfo);

static void checkTermination(DATA* data);

static void writeOutputVars(char* names, DATA* data);

int solver_main_step(int flag, DATA* data, SOLVER_INFO* solverInfo)
{
  switch(flag)
  {
  case 2:
    return rungekutta_step(data, solverInfo);

  case 3:
    return dasrt_step(data, solverInfo);

  case 4:
    functionODE_inline(data, solverInfo->currentStepSize);
    solverInfo->currentTime = data->localData[0]->timeValue;
    return 0;

  case 6:
    return radauIIA_step(data, solverInfo);

  default:
  case 1:
    return euler_ex_step(data, solverInfo);

  }
  return 1;
}

/**
 * The main function for a solver with synchronous event handling
 * flag 1=explicit euler
 * 2=rungekutta
 * 3=dassl
 * 4=inline
 * 5=free
 */
int solver_main(DATA* data, const char* init_initMethod,
    const char* init_optiMethod, const char* init_file, double init_time,
    int flag, const char* outputVariablesAtEnd)
{
  int i;
  unsigned int ui;

  SOLVER_INFO solverInfo;
  SIMULATION_INFO *simInfo = &(data->simulationInfo);
  SIMULATION_DATA *sData = data->localData[0];

  int retValIntegrator = 0;
  int retVal = 0;

  FILE *fmt = NULL;
  unsigned int stepNo = 0;

  /* initial solverInfo */
  solverInfo.currentTime = simInfo->startTime;
  solverInfo.currentStepSize = simInfo->stepSize;
  solverInfo.laststep = 0;
  solverInfo.offset = 0;
  solverInfo.solverRootFinding = 0;
  solverInfo.eventLst = allocList(sizeof(long));
  solverInfo.didEventStep = 0;
  solverInfo.stateEvents = 0;
  solverInfo.sampleEvents = 0;
  solverInfo.stepNo = 0;
  solverInfo.callsODE = 0;
  solverInfo.callsDAE = 0;

  copyStartValuestoInitValues(data);
  /* read input vars */
  input_function(data);

  sData->timeValue = simInfo->startTime;

  /* instance all external Objects */
  callExternalObjectConstructors(data);

  /* allocate memory for non-linear system solvers */
  allocateNonlinearSystem(data);

  /* allocate memory for state selection */
  initializeStateSetJacobians(data);


  if(flag == 2)
  {
    /* Allocate RK work arrays */
    RK4 rungeData;
    rungeData.work_states_ndims = rungekutta_s;
    rungeData.work_states = (double**) malloc((rungeData.work_states_ndims + 1) * sizeof(double*));
    for(i = 0; i < rungeData.work_states_ndims + 1; i++)
      rungeData.work_states[i] = (double*) calloc(data->modelData.nStates, sizeof(double));
    solverInfo.solverData = &rungeData;
  }
  else if(flag == 3)
  {
    /* Initial DASSL solver */
    DASSL_DATA dasslData = {0};
    INFO(LOG_SOLVER, "Initializing DASSL");
    dasrt_initial(data, &solverInfo, &dasslData);
    solverInfo.solverData = &dasslData;
  }
  else if(flag == 4)
  {
    /* Enable inlining solvers */
    work_states = (double**) malloc(inline_work_states_ndims * sizeof(double*));
    for(i = 0; i < inline_work_states_ndims; i++)
      work_states[i] = (double*) calloc(data->modelData.nVariablesReal, sizeof(double));
  }
#ifdef PATHCONSTRAINTS
  else if(flag == 5)
  {
    int neqns = -1;
    /* Allocate work array for optimization*/
    pathConstraints(data, NULL, &neqns);
    /*allocateDaeIpopt(data,neqns);*/

  }
#endif
  else if (flag == 6)
  {
#ifdef WITH_SUNDIALS
    /* Allocate Radau IIA work arrays */
    allocateRadauIIA(&rData, data, &solverInfo);
    allocateKinsol(&kData,(void*)&rData);
#else
    return -1;
#endif
  }

  /* Calculate initial values from updateBoundStartValues()
   * saveall() value as pre values */
  if(measure_time_flag)
  {
    rt_accumulate(SIM_TIMER_PREINIT);
    rt_tick(SIM_TIMER_INIT);
  }

  if(initialization(data, init_initMethod, init_optiMethod, init_file, init_time))
  {
    WARNING(LOG_STDOUT, "Error in initialization. Storing results and exiting.\nUse -lv LOG_INIT for more information.");
    simInfo->stopTime = simInfo->startTime;
    retVal = -1;
  }

  /* adrpo: write the parameter data in the file once again after bound parameters and initialization! */
  sim_result_writeParameterData(&(data->modelData));
  INFO(LOG_SOLVER, "Wrote parameters to the file after initialization (for output formats that support this)");
  if(DEBUG_STREAM(LOG_DEBUG))
    printParameters(data, LOG_DEBUG);

  /* initial delay again, due to maybe change
   * parameters during Initialization */
  initDelay(data, simInfo->startTime);


  storePreValues(data);
  storeOldValues(data);
  function_storeDelayed(data);
  function_updateRelations(data,1);
  storeRelations(data);
  updateHysteresis(data);
  saveZeroCrossings(data);

  sim_result_emit(data);
  overwriteOldSimulationData(data);

  /* Initialization complete */
  if(measure_time_flag)
    rt_accumulate( SIM_TIMER_INIT);

  if(data->localData[0]->timeValue >= simInfo->stopTime)
  {
    INFO(LOG_SOLVER,"Simulation done!");
    solverInfo.currentTime = simInfo->stopTime;
  }

  INFO(LOG_SOLVER, "Performed initial value calculation.");
  INFO2(LOG_SOLVER, "Start numerical solver from %g to %g", simInfo->startTime, simInfo->stopTime);

  if(measure_time_flag)
  {
    size_t len = strlen(data->modelData.modelFilePrefix);
    char* filename = (char*) malloc((len+11) * sizeof(char));
    strncpy(filename,data->modelData.modelFilePrefix,len);
    strncpy(&filename[len],"_prof.data",11);
    fmt = fopen(filename, "wb");
    if(!fmt)
    {
      WARNING2(LOG_SOLVER, "Time measurements output file %s could not be opened: %s", filename, strerror(errno));
      fclose(fmt);
      fmt = NULL;
    }
    free(filename);
  }

  if(DEBUG_STREAM(LOG_DEBUG))
    printAllVars(data, 0, LOG_DEBUG);

  /*
   * Start main simulation loop
   */
  while(solverInfo.currentTime < simInfo->stopTime)
  {
    INDENT(LOG_SOLVER);
    if(measure_time_flag)
    {
      for(i = 0; i < data->modelData.nFunctions + data->modelData.nProfileBlocks; i++)
        rt_clear(i + SIM_TIMER_FIRST_FUNCTION);
      rt_clear(SIM_TIMER_STEP);
      rt_tick(SIM_TIMER_STEP);
    }

    rotateRingBuffer(data->simulationData, 1, (void**) data->localData);

    /******** Calculation next step size ********/
    /* Calculate new step size after an event */
    if(solverInfo.didEventStep == 1)
    {
      solverInfo.offset = solverInfo.currentTime - solverInfo.laststep;
      if(solverInfo.offset + DBL_EPSILON > simInfo->stepSize)
        solverInfo.offset = 0;
      INFO1(LOG_SOLVER, "offset value for the next step: %.10f", solverInfo.offset);
    }
    else
      solverInfo.offset = 0;
    solverInfo.currentStepSize = simInfo->stepSize - solverInfo.offset;

    /* adjust final step? */
    if(solverInfo.currentTime + solverInfo.currentStepSize > simInfo->stopTime)
      solverInfo.currentStepSize = simInfo->stopTime - solverInfo.currentTime;
    /******** End calculation next step size ********/

    /* check for next sample event */
    checkForSampleEvent(data, &solverInfo);
    INFO3(LOG_SOLVER, "call solver from %g to %g (stepSize: %g)", solverInfo.currentTime, solverInfo.currentTime + solverInfo.currentStepSize, solverInfo.currentStepSize);

    /*
     * integration step
     * determine all states by a integration method
     * update continuous system
     */
    INDENT(LOG_SOLVER);
    communicateStatus("Running", (solverInfo.currentTime-simInfo->startTime)/(simInfo->stopTime-simInfo->startTime));
    retValIntegrator = solver_main_step(flag, data, &solverInfo);
    updateContinuousSystem(data);
    saveZeroCrossings(data);
    RELEASE(LOG_SOLVER);

    /******** Event handling ********/
    if(measure_time_flag)
      rt_tick(SIM_TIMER_EVENT);

    if(checkEvents(data, solverInfo.eventLst, &(solverInfo.currentTime), &solverInfo))
    {
      INFO1(LOG_EVENTS, "event handling at time %g", solverInfo.currentTime);
      INDENT(LOG_EVENTS);
      handleEvents(data, solverInfo.eventLst, &(solverInfo.currentTime), &solverInfo);
      RELEASE(LOG_EVENTS);

      solverInfo.didEventStep = 1;
      overwriteOldSimulationData(data);
    }
    else
    {
      solverInfo.laststep = solverInfo.currentTime;
      solverInfo.didEventStep = 0;
    }
    if(measure_time_flag)
      rt_accumulate(SIM_TIMER_EVENT);
    /******** End event handling ********/

    /******** check state selection ********/
    if (stateSelection(data,1))
    {
      /* if new set is calculated reinit the solver */
      solverInfo.didEventStep = 1;
      overwriteOldSimulationData(data);
    }

    /******** Emit this time step ********/
    storePreValues(data);
    storeOldValues(data);
    saveZeroCrossings(data);


    if(fmt)
    {
      int flag = 1;
      double tmpdbl;
      unsigned int tmpint;
      rt_tick(SIM_TIMER_OVERHEAD);
      rt_accumulate(SIM_TIMER_STEP);
      /* Disable time measurements if we have trouble writing to the file... */
      flag = flag && 1 == fwrite(&stepNo, sizeof(unsigned int), 1, fmt);
      stepNo++;
      flag = flag && 1 == fwrite(&(data->localData[0]->timeValue), sizeof(double), 1, fmt);
      tmpdbl = rt_accumulated(SIM_TIMER_STEP);
      flag = flag && 1 == fwrite(&tmpdbl, sizeof(double), 1, fmt);
      for(i = 0; i < data->modelData.nFunctions + data->modelData.nProfileBlocks; i++)
      {
        tmpint = rt_ncall(i + SIM_TIMER_FIRST_FUNCTION);
        flag = flag && 1 == fwrite(&tmpint, sizeof(unsigned int), 1, fmt);
      }
      for(i = 0; i < data->modelData.nFunctions + data->modelData.nProfileBlocks; i++)
      {
        tmpdbl = rt_accumulated(i + SIM_TIMER_FIRST_FUNCTION);
        flag = flag && 1 == fwrite(&tmpdbl, sizeof(double), 1, fmt);
      }
      rt_accumulate(SIM_TIMER_OVERHEAD);
      if(!flag)
      {
        WARNING1(LOG_SOLVER, "Disabled time measurements because the output file could not be generated: %s", strerror(errno));
        fclose(fmt);
        fmt = NULL;
      }
    }
    sim_result_emit(data);

    if(DEBUG_STREAM(LOG_DEBUG))
      printAllVars(data, 0, LOG_DEBUG);

    /********* end of Emit this time step *********/

    /* save dassl stats before reset */
    if(solverInfo.didEventStep == 1 && flag == 3)
    {
      for(ui = 0; ui < numStatistics; ui++)
        ((DASSL_DATA*)solverInfo.solverData)->dasslStatistics[ui] += ((DASSL_DATA*)solverInfo.solverData)->dasslStatisticsTmp[ui];
    }

    /* Check for termination of terminate() or assert() */
    checkForAsserts(data);
    if(terminationAssert || terminationTerminate)
    {
      terminationAssert = 0;
      checkForAsserts(data);
      checkTermination(data);
      if(!terminationAssert && terminationTerminate)
      {
        INFO2(LOG_STDOUT, "Simulation call terminate() at time %f\nMessage : %s", data->localData[0]->timeValue, TermMsg);
        simInfo->stopTime = solverInfo.currentTime;
      }
    }

    /* terminate for some cases:
     * - integrator fails
     * - non-linear system failed to solve
     * - assert was called
     */
    if(data->simulationInfo.simulationSuccess != 0 || retValIntegrator != 0 || check_nonlinear_solutions(data))
    {
      data->simulationInfo.terminal = 1;
      updateDiscreteSystem(data);
      data->simulationInfo.terminal = 0;

      if(data->simulationInfo.simulationSuccess)
      {
        retVal = -1;
        INFO1(LOG_STDOUT, "model terminate | Simulation terminated at time %g", solverInfo.currentTime);
      }
      else if(retValIntegrator)
      {
        retVal = -1 + retValIntegrator;
        INFO1(LOG_STDOUT, "model terminate | Integrator failed. | Simulation terminated at time %g", solverInfo.currentTime);
      }
      else if(check_nonlinear_solutions(data))
      {
        retVal = -2;
        INFO1(LOG_STDOUT, "model terminate | non-linear system solver failed. | Simulation terminated at time %g", solverInfo.currentTime);
      }
      break;
    }
    RELEASE(LOG_SOLVER);
  } /* end while solver */


  /* Last step with terminal()=true */
  if(solverInfo.currentTime >= simInfo->stopTime)
  {
    data->simulationInfo.terminal = 1;
    updateDiscreteSystem(data);
    sim_result_emit(data);
    data->simulationInfo.terminal = 0;
  }
  communicateStatus("Finished", 1);

  /* we have output variables in the command line -output a,b,c */
  if(outputVariablesAtEnd)
  {
    writeOutputVars(strdup(outputVariablesAtEnd), data);
  }

  /* save dassl stats before print */

  if(DEBUG_STREAM(LOG_STATS))
  {
    if(flag == 3)
    {
      for(ui = 0; ui < numStatistics; ui++)
        ((DASSL_DATA*)solverInfo.solverData)->dasslStatistics[ui] += ((DASSL_DATA*)solverInfo.solverData)->dasslStatisticsTmp[ui];
    }
    rt_accumulate(SIM_TIMER_TOTAL);

    INFO(LOG_STATS, "##### Statistics #####");
    INFO1(LOG_STATS, "simulation time: %g", rt_accumulated(SIM_TIMER_TOTAL));
    INFO1(LOG_STATS, "Events: %d", solverInfo.stateEvents + solverInfo.sampleEvents);
    INFO1(LOG_STATS, "State Events: %d", solverInfo.stateEvents);
    INFO1(LOG_STATS, "Sample Events: %d", solverInfo.sampleEvents);
    if(flag == 3)
    {
      INFO(LOG_STATS, "##### Solver Statistics #####");
      INFO1(LOG_STATS, "The number of steps taken: %d", ((DASSL_DATA*)solverInfo.solverData)->dasslStatistics[0]);
      INFO1(LOG_STATS, "The number of calls to functionODE: %d", ((DASSL_DATA*)solverInfo.solverData)->dasslStatistics[1]);
      INFO1(LOG_STATS, "The evaluations of Jacobian: %d", ((DASSL_DATA*)solverInfo.solverData)->dasslStatistics[2]);
      INFO1(LOG_STATS, "The number of error test failures: %d", ((DASSL_DATA*)solverInfo.solverData)->dasslStatistics[3]);
      INFO1(LOG_STATS, "The number of convergence test failures: %d", ((DASSL_DATA*)solverInfo.solverData)->dasslStatistics[4]);
    }
    rt_tick(SIM_TIMER_TOTAL);

  }

  /* deintialize solver related workspace */
  if(flag == 2)
  {
    /* free RK work arrays */
    for(i = 0; i < ((RK4*)(solverInfo.solverData))->work_states_ndims + 1; i++)
      free(((RK4*)(solverInfo.solverData))->work_states[i]);
    free(((RK4*)(solverInfo.solverData))->work_states);
  }
  else if(flag == 3)
  {
    /* De-Initial DASSL solver */
    dasrt_deinitial(solverInfo.solverData);
  }
  else if(flag == 4)
  {
    /* De-Initial inline solver */
    for(i = 0; i < inline_work_states_ndims; i++)
      free(work_states[i]);
    free(work_states);
  }
  else if(flag == 6)
  {
#ifdef WITH_SUNDIALS
    /* free  work arrays */
    freeRadauIIA(&rData);
    freeKinsol(&kData);
#endif
  }
  else
  {
    /* free other solver memory */
  }

  /* free nonlinear system data */
  freeNonlinearSystem(data);
  /* free stateset data */
  freeStateSetData(data);

  if(fmt)
    fclose(fmt);

  return retVal;
}

/***************************************    EULER_EXP     *********************************/
static int euler_ex_step(DATA* data, SOLVER_INFO* solverInfo)
{
  int i;
  SIMULATION_DATA *sData = (SIMULATION_DATA*)data->localData[0];
  SIMULATION_DATA *sDataOld = (SIMULATION_DATA*)data->localData[1];
  modelica_real* stateDer = sDataOld->realVars + data->modelData.nStates;

  for(i = 0; i < data->modelData.nStates; i++)
  {
    sData->realVars[i] = sDataOld->realVars[i] + stateDer[i] * solverInfo->currentStepSize;
  }
  sData->timeValue = sDataOld->timeValue + solverInfo->currentStepSize;
  solverInfo->currentTime += solverInfo->currentStepSize;
  return 0;
}

/***************************************    RK4      ***********************************/
static int rungekutta_step(DATA* data, SOLVER_INFO* solverInfo)
{
  double** k = ((RK4*)(solverInfo->solverData))->work_states;
  double sum;
  int i,j;
  SIMULATION_DATA *sData = (SIMULATION_DATA*)data->localData[0];
  SIMULATION_DATA *sDataOld = (SIMULATION_DATA*)data->localData[1];
  modelica_real* stateDer = sData->realVars + data->modelData.nStates;
  modelica_real* stateDerOld = sDataOld->realVars + data->modelData.nStates;


  /* We calculate k[0] before returning from this function.
   * We only want to calculate f() 4 times per call */
  for(i = 0; i < data->modelData.nStates; i++)
  {
    k[0][i] = stateDerOld[i];
  }

  for(j = 1; j < rungekutta_s; j++)
  {
    for(i = 0; i < data->modelData.nStates; i++)
    {
      sData->realVars[i] = sDataOld->realVars[i] + solverInfo->currentStepSize * rungekutta_c[j] * k[j - 1][i];
    }
    sData->timeValue = sDataOld->timeValue + rungekutta_c[j] * solverInfo->currentStepSize;
    functionODE(data);
    for(i = 0; i < data->modelData.nStates; i++)
    {
      k[j][i] = stateDer[i];
    }
  }

  for(i = 0; i < data->modelData.nStates; i++)
  {
    sum = 0;
    for(j = 0; j < rungekutta_s; j++)
    {
      sum = sum + rungekutta_b[j] * k[j][i];
    }
    sData->realVars[i] = sDataOld->realVars[i] + solverInfo->currentStepSize * sum;
  }
  sData->timeValue = sDataOld->timeValue + solverInfo->currentStepSize;
  solverInfo->currentTime += solverInfo->currentStepSize;
  return 0;
}

#ifdef WITH_SUNDIALS
/***************************************    Radau IIA     ***********************************/
int radauIIA_step(DATA* data, SOLVER_INFO* solverInfo)
{
  kinsolRadauIIA(&rData);
  solverInfo->currentTime += solverInfo->currentStepSize;
  return 0;
}
#endif

/** function checkTermination
 *  author: wbraun
 *
 *  function checks if the model should really terminated.
 */
static void checkTermination(DATA *data)
{
  if(terminationAssert)
  {
    data->simulationInfo.simulationSuccess = -1;
    printInfo(stdout, TermInfo);
    fputc('\n', stdout);
  }

  if(terminationAssert)
  {
    if(warningLevelAssert)
    {
      /* terminated from assert, etc. */
      INFO2(LOG_STDOUT, "Simulation call assert() at time %f\nLevel : warning\nMessage : %s", data->localData[0]->timeValue, TermMsg);
    }
    else
    {
      INFO2(LOG_STDOUT, "Simulation call assert() at time %f\nLevel : error\nMessage : %s", data->localData[0]->timeValue, TermMsg);
      /* THROW1("timeValue = %f", data->localData[0]->timeValue); */
    }
  }
}

static void writeOutputVars(char* names, DATA* data)
{
  int i = 0;
  char *p = strtok(names, ",");

  fprintf(stdout, "time=%.20g", data->localData[0]->timeValue);

  while(p)
  {
    for(i = 0; i < data->modelData.nVariablesReal; i++)
      if(!strcmp(p, data->modelData.realVarsData[i].info.name))
        fprintf(stdout, ",%s=%.20g", p, (data->localData[0])->realVars[i]);
    for(i = 0; i < data->modelData.nVariablesInteger; i++)
      if(!strcmp(p, data->modelData.integerVarsData[i].info.name))
        fprintf(stdout, ",%s=%li", p, (data->localData[0])->integerVars[i]);
    for(i = 0; i < data->modelData.nVariablesBoolean; i++)
      if(!strcmp(p, data->modelData.booleanVarsData[i].info.name))
        fprintf(stdout, ",%s=%i", p, (data->localData[0])->booleanVars[i]);
    for(i = 0; i < data->modelData.nVariablesString; i++)
      if(!strcmp(p, data->modelData.stringVarsData[i].info.name))
        fprintf(stdout, ",%s=\"%s\"", p, (data->localData[0])->stringVars[i]);

    for(i = 0; i < data->modelData.nAliasReal; i++)
      if(!strcmp(p, data->modelData.realAlias[i].info.name))
      {
       if(data->modelData.realAlias[i].negate)
         fprintf(stdout, ",%s=%.20g", p, -(data->localData[0])->realVars[data->modelData.realAlias[i].nameID]);
       else
         fprintf(stdout, ",%s=%.20g", p, (data->localData[0])->realVars[data->modelData.realAlias[i].nameID]);
      }
    for(i = 0; i < data->modelData.nAliasInteger; i++)
      if(!strcmp(p, data->modelData.integerAlias[i].info.name))
      {
        if(data->modelData.integerAlias[i].negate)
          fprintf(stdout, ",%s=%li", p, -(data->localData[0])->integerVars[data->modelData.integerAlias[i].nameID]);
        else
          fprintf(stdout, ",%s=%li", p, (data->localData[0])->integerVars[data->modelData.integerAlias[i].nameID]);
      }
    for(i = 0; i < data->modelData.nAliasBoolean; i++)
      if(!strcmp(p, data->modelData.booleanAlias[i].info.name))
      {
        if(data->modelData.booleanAlias[i].negate)
          fprintf(stdout, ",%s=%i", p, -(data->localData[0])->booleanVars[data->modelData.booleanAlias[i].nameID]);
        else
          fprintf(stdout, ",%s=%i", p, (data->localData[0])->booleanVars[data->modelData.booleanAlias[i].nameID]);
      }
    for(i = 0; i < data->modelData.nAliasString; i++)
      if(!strcmp(p, data->modelData.stringAlias[i].info.name))
        fprintf(stdout, ",%s=\"%s\"", p, (data->localData[0])->stringVars[data->modelData.stringAlias[i].nameID]);

    /* parameters */
    for(i = 0; i < data->modelData.nParametersReal; i++)
      if(!strcmp(p, data->modelData.realParameterData[i].info.name))
        fprintf(stdout, ",%s=%.20g", p, data->simulationInfo.realParameter[i]);

    for(i = 0; i < data->modelData.nParametersInteger; i++)
      if(!strcmp(p, data->modelData.integerParameterData[i].info.name))
        fprintf(stdout, ",%s=%li", p, data->simulationInfo.integerParameter[i]);

    for(i = 0; i < data->modelData.nParametersBoolean; i++)
      if(!strcmp(p, data->modelData.booleanParameterData[i].info.name))
        fprintf(stdout, ",%s=%i", p, data->simulationInfo.booleanParameter[i]);

    for(i = 0; i < data->modelData.nParametersString; i++)
      if(!strcmp(p, data->modelData.stringParameterData[i].info.name))
        fprintf(stdout, ",%s=\"%s\"", p, data->simulationInfo.stringParameter[i]);

    /* move to next */
    p = strtok(NULL, ",");
  }
  fprintf(stdout, "\n"); fflush(stdout);
}
