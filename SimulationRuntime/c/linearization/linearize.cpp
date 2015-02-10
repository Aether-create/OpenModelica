/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-2010, Linköpings University,
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
 * from Linköpings University, either from the above address,
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

#include "util/omc_error.h"
#include "simulation_data.h"
#include "openmodelica_func.h"
#include "linearize.h"
#include <iostream>
#include <sstream>
#include <string>


using namespace std;

string array2string(double* array, int row, int col){

  int i=0;
  int j=0;
  ostringstream retVal(ostringstream::out);
  retVal.precision(16);
  for(i=0;i<row;i++){
    int k = i;
    for(j=0;j<col-1;j++){
      retVal << array[k] << ",";
      k += row;
    }
    if(col > 0) {
      retVal << array[k];
    }
    if(!((i+1) == row) && !(col == 0)) {
      retVal << ";";
    }
  }
  return retVal.str();
}

int functionJacA(DATA* data, double* jac){

  const int index = data->callback->INDEX_JAC_A;
  unsigned int i,j,k;
  k = 0;
  for(i=0; i < data->simulationInfo.analyticJacobians[index].sizeCols; i++)
  {
    data->simulationInfo.analyticJacobians[index].seedVars[i] = 1.0;
    if(ACTIVE_STREAM(LOG_JAC))
    {
      printf("Caluculate one col:\n");
      for(j=0;  j < data->simulationInfo.analyticJacobians[index].sizeCols;j++)
      {
        infoStreamPrint(LOG_JAC,0,"seed: data->simulationInfo.analyticJacobians[index].seedVars[%d]= %f",j,data->simulationInfo.analyticJacobians[index].seedVars[j]);
      }
    }

    data->callback->functionJacA_column(data);

    for(j = 0; j < data->simulationInfo.analyticJacobians[index].sizeRows; j++)
    {
      jac[k++] = data->simulationInfo.analyticJacobians[index].resultVars[j];
      infoStreamPrint(LOG_JAC,0,"write in jac[%d]-[%d,%d]=%g from row[%d]=%g",k-1,i,j,jac[k-1],i,data->simulationInfo.analyticJacobians[index].resultVars[j]);
    }

    data->simulationInfo.analyticJacobians[index].seedVars[i] = 0.0;
  }
  if(ACTIVE_STREAM(LOG_JAC))
  {
    infoStreamPrint(LOG_JAC,0,"Print jac:");
    for(i=0;  i < data->simulationInfo.analyticJacobians[index].sizeRows;i++)
    {
      for(j=0;  j < data->simulationInfo.analyticJacobians[index].sizeCols;j++) {
        printf("% .5e ",jac[i+j*data->simulationInfo.analyticJacobians[index].sizeCols]);
      }
      printf("\n");
    }
  }

  return 0;
}
int functionJacB(DATA* data, double* jac){

  const int index = data->callback->INDEX_JAC_B;
  unsigned int i,j,k;
  k = 0;
  for(i=0; i < data->simulationInfo.analyticJacobians[index].sizeCols; i++)
  {
    data->simulationInfo.analyticJacobians[index].seedVars[i] = 1.0;
    if(ACTIVE_STREAM(LOG_JAC))
    {
      printf("Caluculate one col:\n");
      for(j=0;  j < data->simulationInfo.analyticJacobians[index].sizeCols;j++)
      {
        infoStreamPrint(LOG_JAC,0,"seed: data->simulationInfo.analyticJacobians[index].seedVars[%d]= %f",j,data->simulationInfo.analyticJacobians[index].seedVars[j]);
      }
    }

    data->callback->functionJacB_column(data);

    for(j = 0; j < data->simulationInfo.analyticJacobians[index].sizeRows; j++)
    {
      jac[k++] = data->simulationInfo.analyticJacobians[index].resultVars[j];
      infoStreamPrint(LOG_JAC,0,"write in jac[%d]-[%d,%d]=%g from row[%d]=%g",k-1,i,j,jac[k-1],i,data->simulationInfo.analyticJacobians[index].resultVars[j]);
    }

    data->simulationInfo.analyticJacobians[index].seedVars[i] = 0.0;
  }
  if(ACTIVE_STREAM(LOG_JAC))
  {
    infoStreamPrint(LOG_JAC, 0, "Print jac:");
    for(i=0;  i < data->simulationInfo.analyticJacobians[index].sizeRows;i++)
    {
      for(j=0;  j < data->simulationInfo.analyticJacobians[index].sizeCols;j++)
        printf("% .5e ",jac[i+j*data->simulationInfo.analyticJacobians[index].sizeCols]);
      printf("\n");
    }
  }

  return 0;
}
int functionJacC(DATA* data, double* jac){

  const int index = data->callback->INDEX_JAC_C;
  unsigned int i,j,k;
  k = 0;
  for(i=0; i < data->simulationInfo.analyticJacobians[index].sizeCols; i++)
  {
    data->simulationInfo.analyticJacobians[index].seedVars[i] = 1.0;
    if(ACTIVE_STREAM(LOG_JAC))
    {
      printf("Caluculate one col:\n");
      for(j=0;  j < data->simulationInfo.analyticJacobians[index].sizeCols;j++)
        infoStreamPrint(LOG_JAC,0,"seed: data->simulationInfo.analyticJacobians[index].seedVars[%d]= %f",j,data->simulationInfo.analyticJacobians[index].seedVars[j]);
    }

    data->callback->functionJacC_column(data);

    for(j = 0; j < data->simulationInfo.analyticJacobians[index].sizeRows; j++)
    {
      jac[k++] = data->simulationInfo.analyticJacobians[index].resultVars[j];
      infoStreamPrint(LOG_JAC,0,"write in jac[%d]-[%d,%d]=%g from row[%d]=%g",k-1,i,j,jac[k-1],i,data->simulationInfo.analyticJacobians[index].resultVars[j]);
    }

    data->simulationInfo.analyticJacobians[index].seedVars[i] = 0.0;
  }
  if(ACTIVE_STREAM(LOG_JAC))
  {
    infoStreamPrint(LOG_JAC, 0, "Print jac:");
    for(i=0;  i < data->simulationInfo.analyticJacobians[index].sizeRows;i++)
    {
      for(j=0;  j < data->simulationInfo.analyticJacobians[index].sizeCols;j++)
        printf("% .5e ",jac[i+j*data->simulationInfo.analyticJacobians[index].sizeCols]);
      printf("\n");
    }
  }

  return 0;
}
int functionJacD(DATA* data, double* jac){

  const int index = data->callback->INDEX_JAC_D;
  unsigned int i,j,k;
  k = 0;
  for(i=0; i < data->simulationInfo.analyticJacobians[index].sizeCols; i++)
  {
    data->simulationInfo.analyticJacobians[index].seedVars[i] = 1.0;
    if(ACTIVE_STREAM(LOG_JAC))
    {
      printf("Caluculate one col:\n");
      for(j=0;  j < data->simulationInfo.analyticJacobians[index].sizeCols;j++) {
        infoStreamPrint(LOG_JAC,0,"seed: data->simulationInfo.analyticJacobians[index].seedVars[%d]= %f",j,data->simulationInfo.analyticJacobians[index].seedVars[j]);
      }
    }

    data->callback->functionJacD_column(data);

    for(j = 0; j < data->simulationInfo.analyticJacobians[index].sizeRows; j++)
    {
      jac[k++] = data->simulationInfo.analyticJacobians[index].resultVars[j];
      infoStreamPrint(LOG_JAC,0, "write in jac[%d]-[%d,%d]=%g from row[%d]=%g",k-1,i,j,jac[k-1],i,data->simulationInfo.analyticJacobians[index].resultVars[j]);
    }

    data->simulationInfo.analyticJacobians[index].seedVars[i] = 0.0;
  }
  if(ACTIVE_STREAM(LOG_JAC))
  {
    infoStreamPrint(LOG_JAC, 0, "Print jac:");
    for(i=0;  i < data->simulationInfo.analyticJacobians[index].sizeRows;i++)
    {
      for(j=0;  j < data->simulationInfo.analyticJacobians[index].sizeCols;j++)
        printf("% .5e ",jac[i+j*data->simulationInfo.analyticJacobians[index].sizeCols]);
      printf("\n");
    }
  }

  return 0;
}



int linearize(DATA* data)
{
    /* init linearization sizes */
    int size_A = data->modelData.nStates;
    int size_Inputs = data->modelData.nInputVars;
    int size_Outputs = data->modelData.nOutputVars;
    double* matrixA = (double*)calloc(size_A*size_A,sizeof(double));
    double* matrixB = (double*)calloc(size_A*size_Inputs,sizeof(double));
    double* matrixC = (double*)calloc(size_Outputs*size_A,sizeof(double));
    double* matrixD = (double*)calloc(size_Outputs*size_Inputs,sizeof(double));
    string strA, strB, strC, strD, strX, strU, filename;

    assertStreamPrint(data->threadData,0!=matrixA,"calloc failed");
    assertStreamPrint(data->threadData,0!=matrixB,"calloc failed");
    assertStreamPrint(data->threadData,0!=matrixC,"calloc failed");;
    assertStreamPrint(data->threadData,0!=matrixD,"calloc failed");

    /* Determine Matrix A */
    if(!data->callback->initialAnalyticJacobianA(data)){
      assertStreamPrint(data->threadData,0==functionJacA(data, matrixA),"Error, can not get Matrix A ");
    }
    strA = array2string(matrixA,size_A,size_A);

    /* Determine Matrix B */
    if(!data->callback->initialAnalyticJacobianB(data)){
      assertStreamPrint(data->threadData,0==functionJacB(data, matrixB),"Error, can not get Matrix B ");
    }
    strB = array2string(matrixB,size_A,size_Inputs);

    /* Determine Matrix C */
    if(!data->callback->initialAnalyticJacobianC(data)){
      assertStreamPrint(data->threadData,0==functionJacC(data, matrixC),"Error, can not get Matrix C ");
    }
    strC = array2string(matrixC,size_Outputs,size_A);

    /* Determine Matrix D */
    if(!data->callback->initialAnalyticJacobianD(data)){
      assertStreamPrint(data->threadData,0==functionJacD(data, matrixD),"Error, can not get Matrix D ");
    }
    strD = array2string(matrixD,size_Outputs,size_Inputs);

    // The empty array {} is not valid modelica, so we need to put something
    //   inside the curly braces for x0 and u0. {for i in in 1:0} will create an
    //   empty array if needed.
    if(size_A)
      strX = array2string(data->localData[0]->realVars,1,size_A);
    else
      strX = "i for i in 1:0";

    if(size_Inputs)
      strU = array2string(data->simulationInfo.inputVars,1,size_Inputs);
    else
      strU = "i for i in 1:0";

    free(matrixA);
    free(matrixB);
    free(matrixC);
    free(matrixD);

    filename = "linear_" + string(data->modelData.modelName) + ".mo";

    FILE *fout = fopen(filename.c_str(),"wb");
    assertStreamPrint(data->threadData,0!=fout,"Cannot open File %s",filename.c_str());
    fprintf(fout, data->callback->linear_model_frame(), strX.c_str(), strU.c_str(), strA.c_str(), strB.c_str(), strC.c_str(), strD.c_str());
    if(ACTIVE_STREAM(LOG_STATS)) {
      infoStreamPrint(LOG_STATS, 0, data->callback->linear_model_frame(), strX.c_str(), strU.c_str(), strA.c_str(), strB.c_str(), strC.c_str(), strD.c_str());
    }
    fflush(fout);
    fclose(fout);

    return 0;
}

