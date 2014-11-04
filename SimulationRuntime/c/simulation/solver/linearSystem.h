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

/*! \file linearSystem.h
 */


#ifndef _LINEARSYSTEM_H_
#define _LINEARSYSTEM_H_

#include "simulation_data.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef VOID
#undef VOID
#endif

enum LINEAR_SOLVER
{
  LS_NONE = 0,

  LS_LAPACK,
  LS_LIS,
  LS_TOTALPIVOT,

  LS_MAX
};

extern const char *LS_NAME[LS_MAX+1];
extern const char *LS_DESC[LS_MAX+1];

typedef void* LS_SOLVER_DATA;

int initializeLinearSystems(DATA *data);
int updateStaticDataOfLinearSystems(DATA *data);
int freeLinearSystems(DATA *data);
int solve_linear_system(DATA *data, int sysNumber);
int check_linear_solutions(DATA *data, int printFailingSystems);

void setAElementLAPACK(int row, int col, double value, int nth, void *data);
void setAElementLis(int row, int col, double value, int nth, void *data);
void setAElementTotalPivot(int row, int col, double value, int nth, void *data);

#ifdef __cplusplus
}
#endif

#endif
