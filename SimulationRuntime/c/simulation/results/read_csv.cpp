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

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

extern "C"
{
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "read_csv.h"

int read_csv_dataset_size(const char* filename)
{
  string header;
  int length;
  int size = -1;

  ifstream stream(filename);

  /* first line start with " */
  getline(stream,header);
  length = strlen(header.c_str());

  /* second line has to be a number */
  while (length > 0) {
    if (length != 0)
      size+=1;
    getline(stream,header);
    length = strlen(header.c_str());
  }
  return size;
}

char** read_csv_variables(const char* filename)
{
  string header;
  string variable;
  vector<string> variablesList;
  bool startReading = false;
  int length;

  ifstream stream(filename);

  getline(stream,header);
  length = strlen(header.c_str());

  for (int i = 0 ; i < length ; i++)
  {
    if (startReading)
      variable.append(1,header[i]);

    if (header[i] == '"') {
      if (startReading) {
        if (header[i+1] == ',') {
          startReading = false;
          variablesList.push_back(variable.erase((strlen(variable.c_str()) - 1), 1));
          variable.clear();
        }
      } else {
        startReading = true;
      }
    }
  }

  char **res = (char**)malloc((1+variablesList.size())*sizeof(char));
  for (unsigned int k = 0 ; k < variablesList.size(); k++)
  {
    res[k] = strdup(variablesList.at(k).c_str());
  }
  res[variablesList.size()] = NULL;
  return res;
}

double* read_csv_dataset(const char *filename, const char *var, int dimsize)
{
  string header;
  string variable;
  string value;
  unsigned int varpos=0;
  unsigned int datapos=0;
  bool startReading = false;
  int length=0;
  unsigned int stringlen=0;
  vector<double> data;
  char found=0;

  /* get Position of var */
  ifstream stream(filename);

  getline(stream,header);
  length = strlen(header.c_str());

  for (int i = 0 ; i < length ; i++)
  {
    if (startReading) {
      if (header[i] != '"') {
        variable.append(1,header[i]);
        stringlen+=1;
      }
    }

    if (header[i] == '"') {
      if (startReading) {
        if (header[i+1] == ',') {
          startReading = false;
          if (strncmp(variable.c_str(),var,stringlen)==0) {
            found = 1;
            break;
          }
          varpos+=1;
          stringlen = 0;
          variable.clear();
        }
      } else {
        startReading = true;
      }
    }
  }
  if (found==0)
  return NULL;
  /* collect data */
  getline(stream,header);
  length = strlen(header.c_str());
  while (length > 0) {
    for (int i = 0 ; i < length ; i++)
    {
      if (header[i] == ',') {
        if (datapos == varpos) {
          data.push_back(atof(value.c_str()));
          break;
        }
        value.clear();
        datapos +=1;
      }
      else
        value.append(1,header[i]);
    }
    getline(stream,header);
    length = strlen(header.c_str());
    datapos = 0;
    value.clear();
  }

  double *res = (double*)malloc((data.size())*sizeof(double));
  for (unsigned int k = 0 ; k < data.size(); k++)
  {
    res[k] = data.at(k);
  }

  return res;
}

}
