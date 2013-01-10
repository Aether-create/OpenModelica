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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <assert.h>

#include "systemimpl.h"

/* Size of the buffer for warnings and other messages */
#define WARNINGBUFFSIZE 250

typedef struct {
  double *data;
  unsigned int n;
} DataField;

typedef struct {
  char *name;
  double data;
  double dataref;
  double time;
  double timeref;
  char interpolate;
} DiffData;

typedef struct {
  DiffData *data;
  unsigned int n;
  unsigned int n_max;
} DiffDataField;

#define DOUBLEEQUAL_TOTAL 0.0000000001
#define DOUBLEEQUAL_REL 0.00001

static SimulationResult_Globals simresglob_c = {UNKNOWN_PLOT,NULL,{NULL,NULL,0,NULL,0,NULL,0,0,0,NULL},NULL,NULL};
static SimulationResult_Globals simresglob_ref = {UNKNOWN_PLOT,NULL,{NULL,NULL,0,NULL,0,NULL,0,0,0,NULL},NULL,NULL};

/* from an array of string creates flatten 'char*'-array suitable to be */
/* stored as MAT-file matrix */
static inline void fixDerInName(char *str, size_t len)
{
  size_t i;
  char* dot;
  if (len < 6) return;

  /* check if name start with "der(" and includes at least one dot */
  while (strncmp(str,"der(",4) == 0 && (dot = strrchr(str,'.')) != NULL) {
    size_t pos = (size_t)(dot-str)+1;
    /* move prefix to the begining of string :"der(a.b.c.d)" -> "a.b.c.b.c.d)" */
    for(i = 4; i < pos; ++i)
      str[i-4] = str[i];
    /* move "der(" to the end of prefix
       "a.b.c.b.c.d)" -> "a.b.c.der(d)" */
    strncpy(&str[pos-4],"der(",4);
  }
}

static inline void fixCommaInName(char **str, size_t len)
{
  size_t nc;
  unsigned int j,k;
  char* newvar;
  if (len < 2) return;

  nc = 0;
  for (j=0;j<len;j++)
    if ((*str)[j] ==',' )
      nc +=1;

  if (nc > 0) {

    newvar = (char*) malloc(len+nc+1);
    k = 0;
    for (j=0;j<len;j++) {
      newvar[k] = (*str)[j];
      k +=1;
      if ((*str)[j] ==',' ) {
        newvar[k] = ' ';
        k +=1;
      }
    }
    newvar[k] = 0;
    free(*str);
    *str = newvar;
  }
}

double absdouble(double d)
{
  if (d > 0.0)
    return d;
  else
    return -1.0*d;
}

char ** getVars(void *vars, unsigned int* nvars)
{
  char **cmpvars=NULL;
  char *var;
  unsigned int i;
  unsigned int ncmpvars = 0;
  char **newvars=NULL;
  *nvars=0;
#ifdef DEBUGOUTPUT
   fprintf(stderr, "getVars\n");
#endif
  while (RML_NILHDR != RML_GETHDR(vars)) {
    var = RML_STRINGDATA(RML_CAR(vars));

    // if var is empty, continue
    if (strcmp(var, "\"\"") == 0) {
#ifdef DEBUGOUTPUT
        fprintf(stderr, "skip Var: %s\n", var);
#endif
       vars = RML_CDR(vars);
       continue;
    }
#ifdef DEBUGOUTPUT
     fprintf(stderr, "Var: %s\n", var);
#endif
    newvars = (char**)malloc(sizeof(char*)*(ncmpvars+1));

    for (i=0;i<ncmpvars;i++)
      newvars[i] = cmpvars[i];
    newvars[ncmpvars] = var;
    ncmpvars += 1;
    if(cmpvars) free(cmpvars);
    cmpvars = newvars;
#ifdef DEBUGOUTPUT
     fprintf(stderr, "NVar: %d\n", ncmpvars);
#endif
    vars = RML_CDR(vars);
  }
  *nvars = ncmpvars;
#ifdef DEBUGOUTPUT
    fprintf(stderr, "found %d Vars\n", ncmpvars);
#endif
  return cmpvars;
}

DataField getData(const char *varname,const char *filename, unsigned int size, SimulationResult_Globals* srg)
{
  DataField res;
  void *cmpvar,*dataset,*lst,*datasetBackup;
  unsigned int i;
  res.n = 0;
  res.data = NULL;

  /* fprintf(stderr, "getData of Var: %s from file %s\n", varname,filename);  */
  cmpvar = mk_nil();
  cmpvar =  mk_cons(mk_scon(varname),cmpvar);
  dataset = SimulationResultsImpl__readDataset(filename,cmpvar,size,srg);
  if (dataset==NULL) {
    /* fprintf(stderr, "getData of Var: %s failed!\n",varname); */
    return res;
  }

  /* fprintf(stderr, "Data of Var: %s\n", varname); */
  /*  First calculate the length of the matrix */
  datasetBackup = dataset;
  while (RML_NILHDR != RML_GETHDR(dataset)) {
    lst = RML_CAR(dataset);
    while (RML_NILHDR != RML_GETHDR(lst)) {
      res.n++;
      lst = RML_CDR(lst);
    }
    dataset = RML_CDR(dataset);
  }
  if (res.n == 0) return res;

  /* The allocate and read the values */
  dataset = datasetBackup;
  i = res.n;
  res.data = (double*) malloc(sizeof(double)*res.n);
  while (RML_NILHDR != RML_GETHDR(dataset)) {
    lst = RML_CAR(dataset);
    while (RML_NILHDR != RML_GETHDR(lst)) {
      res.data[--i] = rml_prim_get_real(RML_CAR(lst));
      lst = RML_CDR(lst);
    }
    dataset = RML_CDR(dataset);
  }
  assert(i == 0);

  /* for (i=0;i<res.n;i++)
    fprintf(stderr, "%d: %.6g\n",  i, res.data[i]); */

  return res;
}

/* see http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/ */
char AlmostEqualRelativeAndAbs(double A, double B)
{
  /* Check if the numbers are really close -- needed
  when comparing numbers near zero. */
  double diff = absdouble(A - B);
  if (diff <= DOUBLEEQUAL_TOTAL)
    return 1;

  A = absdouble(A);
  B = absdouble(B);
  double largest = (B > A) ? B : A;

  if (diff <= largest * DOUBLEEQUAL_REL)
    return 1;
  return 0;
}

unsigned int cmpData(char* varname, DataField *time, DataField *reftime, DataField *data, DataField *refdata, double reltol, double abstol, DiffDataField *ddf, char **cmpdiffvars, unsigned int vardiffindx)
{
  unsigned int i,j,k,j_event;
  double t,tr,d,dr,err,d_left,d_right,dr_left,dr_right,t_event;
  char increased = 0;
  char interpolate = 0;
  char isdifferent = 0;
  char refevent = 0;
  double average=0;
  for (i=0;i<refdata->n;i++){
    average += absdouble(refdata->data[i]);
  }
  average = average/((double)refdata->n);
#ifdef DEBUGOUTPUT
   fprintf(stderr, "average: %.6g\n",average);
#endif
  average = reltol*fabs(average)+abstol;
  j = 0;
  tr = reftime->data[j];
  dr = refdata->data[j];
#ifdef DEBUGOUTPUT
   fprintf(stderr, "compare: %s\n",varname);
#endif
  for (i=0;i<data->n;i++){
    t = time->data[i];
    d = data->data[i];
    increased = 0;
#ifdef DEBUGOUTPUT
     fprintf(stderr, "i: %d t: %.6g   d:%.6g\n",i,t,d);
#endif
    while(tr < t){
      if (j +1< reftime->n) {
        j += 1;
        tr = reftime->data[j];
        increased = 1;
        if (tr == t) {
          break;
        }
#ifdef DEBUGOUTPUT
         fprintf(stderr, "j: %d tr:%.6g\n",j,tr);
#endif
      }
      else
        break;
    }
    if (increased==1) {
      if ( (absdouble((t-tr)/tr) > reltol) || (absdouble(t-tr) > absdouble(t-reftime->data[j-1]))) {
        j = j- 1;
        tr = reftime->data[j];
      }
    }
#ifdef DEBUGOUTPUT
    fprintf(stderr, "i: %d t: %.6g   d:%.6g  j: %d tr:%.6g\n",i,t,d,j,tr);
#endif
    /* events, in case of an event compare only the left and right values of the absolute event time range,
    * this means ta_left = min(t_left,tr_left) and
    * ta_right = max(t_right,ta_right) */
    if(i<time->n) {
#ifdef DEBUGOUTPUT
       fprintf(stderr, "check event: %.6g  - %.6g = %.6g\n",t,time->data[i+1],absdouble(t-time->data[i+1]));
#endif
      /* an event */
      if (AlmostEqualRelativeAndAbs(t,time->data[i+1])) {
#ifdef DEBUGOUTPUT
         fprintf(stderr, "event: %.6g  %d  %.6g\n",t,i,d);
#endif
        /* left value */
        d_left = d;
#ifdef DEBUGOUTPUT
         fprintf(stderr, "left value: %.6g  %d %.6g\n",t,i,d_left);
#endif
        /* right value */
        if (i+1<data->n) {
          while (AlmostEqualRelativeAndAbs(t,time->data[i+1])) {
            i +=1;
            if (i+1>=data->n) break;
          }
        }
        t = time->data[i];
        d_right = data->data[i];
#ifdef DEBUGOUTPUT
        fprintf(stderr, "right value: %.6g  %d %.6g\n",t,i,d_right);
#endif
        /* search event in reference forwards */
        refevent = 0;
        t_event = t + t*reltol*0.1;
        /* do not exceed next time step */
        t_event = (t_event > time->data[i+1])?time->data[i+1]:t_event;
        j_event = j;
        while(tr < t_event) {
          if (j+1<reftime->n) {
            if (AlmostEqualRelativeAndAbs(tr,reftime->data[j+1])) {
              dr_left = refdata->data[j];
#ifdef DEBUGOUTPUT
              fprintf(stderr, "ref left value: %.6g  %d %.6g\n",tr,j,dr_left);
#endif
              refevent = 1;

              do {
                j +=1;
                if (j+1>=reftime->n) break;
              } while (AlmostEqualRelativeAndAbs(tr,reftime->data[j+1]));
            }
          }
          if (refevent == 0) {
            j += 1;
            if (j >= reftime->n)
              break;
            tr = reftime->data[j];
          }
          else {
            tr = reftime->data[j];
            break;
          }
        }
        if (refevent==1) {
          tr = reftime->data[j];
          dr_right = refdata->data[j];
#ifdef DEBUGOUTPUT
           fprintf(stderr, "ref right value: %.6g  %d %.6g\n",tr,j,dr_right);
#endif

          err = absdouble(d_left-dr_left);
#ifdef DEBUGOUTPUT
           fprintf(stderr, "delta:%.6g  reltol:%.6g\n",err,average);
#endif
          if ( err < average){
            err = absdouble(d_right-dr_right);
#ifdef DEBUGOUTPUT
             fprintf(stderr, "delta:%.6g  reltol:%.6g\n",err,average);
#endif
            if ( err < average){
              continue;
            }
          }
        }
        else {
          /* search event in reference backwards */
          j = j_event;
          tr = reftime->data[j];
          refevent = 0;
          t_event = t - t*reltol*0.1;
          while(tr > t_event) {
            if (j-1>0) {
              if (AlmostEqualRelativeAndAbs(tr,reftime->data[j-1])) {
                dr_right = refdata->data[j];
#ifdef DEBUGOUTPUT
                fprintf(stderr, "ref right value: %.6g  %d %.6g\n",tr,j,dr_right);
#endif
                refevent = 1;

                do {
                  j -=1;
                  if (j-1<=0) break;
                } while (AlmostEqualRelativeAndAbs(tr,reftime->data[j-1]));
              }
            }
            if (refevent == 0) {
              j -= 1;
              if (j == 0)
                break;
              tr = reftime->data[j];
            }
            else {
              tr = reftime->data[j];
              break;
            }
          }
          if (refevent==1) {
            tr = reftime->data[j];
            dr_left = refdata->data[j];
#ifdef DEBUGOUTPUT
            fprintf(stderr, "ref left value: %.6g  %d %.6g\n",tr,j,dr_left);
#endif
            err = absdouble(d_left-dr_left);
#ifdef DEBUGOUTPUT
            fprintf(stderr, "delta:%.6g  reltol:%.6g\n",err,average);
#endif
            if ( err < average){
              err = absdouble(d_right-dr_right);
#ifdef DEBUGOUTPUT
              fprintf(stderr, "delta:%.6g  reltol:%.6g\n",err,average);
#endif
              if ( err < average){
                j = j_event;
                tr = reftime->data[j];
                continue;
              }
            }
          }
          j = j_event;
          tr = reftime->data[j];
        }
      }
    }

    interpolate = 0;
#ifdef DEBUGOUTPUT
     fprintf(stderr, "interpolate? %d %.6g:%.6g  %.6g:%.6g\n",i,t,tr,absdouble((t-tr)/tr),abstol);
#endif
    if (absdouble(t-tr) > 0.00001) {
      interpolate = 1;
    }

    dr = refdata->data[j];
    if (interpolate==1){
#ifdef DEBUGOUTPUT
      fprintf(stderr, "interpolate %.6g:%.6g  %.6g:%.6g %d",t,d,tr,dr,j);
#endif
      unsigned int jj = j;
      /* look for interpolation partner */
      if (tr > t) {
        if (j-1 > 0) {
          jj = j-1;
          increased = 0;
          if (reftime->data[jj] == tr){
            increased = 1;
            do {
              jj -= 1;
              if (jj<=0) break;
            } while (reftime->data[jj] == tr);
          }
        }
#ifdef DEBUGOUTPUT
        fprintf(stderr, "-> %d %.6g %.6g\n",jj,reftime->data[jj],refdata->data[jj]);
#endif
        if (reftime->data[jj] != tr){
          dr = refdata->data[jj] + ((dr-refdata->data[jj])/(tr-reftime->data[jj]))*(t-reftime->data[jj]);
        }
#ifdef DEBUGOUTPUT
        fprintf(stderr, "-> dr:%.6g\n",dr);
#endif
      }
      else {
        if (j+1<reftime->n) {
          jj = j+1;
          increased = 0;
          if (reftime->data[jj] == tr){
            increased = 1;
            do {
              jj += 1;
              if (jj>=reftime->n) break;
            } while (reftime->data[jj] == tr);
          }
        }
#ifdef DEBUGOUTPUT
        fprintf(stderr, "-> %d %.6g %.6g\n",jj,reftime->data[jj],tr);
#endif
        if (reftime->data[jj] != tr){
          dr = dr + ((refdata->data[jj] - dr)/(reftime->data[jj] - tr))*(t-tr);
        }
#ifdef DEBUGOUTPUT
        fprintf(stderr, "-> dr:%.6g\n",dr);
#endif
      }
    }
#ifdef DEBUGOUTPUT
    fprintf(stderr, "j: %d tr: %.6g  dr:%.6g  t:%.6g  d:%.6g\n",j,tr,dr,t,d);
#endif
    err = absdouble(d-dr);
#ifdef DEBUGOUTPUT
    fprintf(stderr, "delta:%.6g  reltol:%.6g\n",err,average);
#endif
    if ( err > average){
      if (j+1<reftime->n) {
        if (reftime->data[j+1] == tr) {
          dr = refdata->data[j+1];
          err = absdouble(d-dr);
        }
      }

      if (err < average){
        continue;
      }

      isdifferent = 1;
      if (ddf->n >= ddf->n_max) {
        DiffData *diffdatafild;
        ddf->n_max += 10;
        diffdatafild = (DiffData*) malloc(sizeof(DiffData)*(ddf->n_max));
        for (k=0;k<ddf->n;k++) {
          diffdatafild[k] = ddf->data[k];
        }
        if(ddf->data) free(ddf->data);
        ddf->data = diffdatafild;
      }
      ddf->data[ddf->n].name = varname;
      ddf->data[ddf->n].data = d;
      ddf->data[ddf->n].dataref = dr;
      ddf->data[ddf->n].time = t;
      ddf->data[ddf->n].timeref = tr;
      ddf->data[ddf->n].interpolate = interpolate?'1':'0';
      ddf->n +=1;
    }
  }
  if (isdifferent)
  {
    cmpdiffvars[vardiffindx] = varname;
    vardiffindx++;
  }
  return vardiffindx;
}

int writeLogFile(const char *filename,DiffDataField *ddf,const char *f,const char *reff,double reltol,double abstol)
{
  FILE* fout;
  unsigned int i;
  /* fprintf(stderr, "writeLogFile: %s\n",filename); */
  fout = fopen(filename, "w");
  if (!fout)
    return -1;

  fprintf(fout, "\"Generated by OpenModelica\";;;;;\n");
  fprintf(fout, "\"Compared Files\";;;\"absolute tolerance\";%.6g;relative tolerance;%.6g\n",abstol,reltol);
  fprintf(fout, "\"%s\";;;;;;\n",f);
  fprintf(fout, "\"%s\";;;;;;\n",reff);
  fprintf(fout, "\"Name\";\"Time\";\"DataPoint\";\"RefTime\";\"RefDataPoint\";\"absolute error\";\"relative error\";interpolate;\n");
  for (i=0;i<ddf->n;i++){
    fprintf(fout, "%s;%.6g;%.6g;%.6g;%.6g;%.6g;%.6g;%c;\n",ddf->data[i].name,ddf->data[i].time,ddf->data[i].data,ddf->data[i].timeref,ddf->data[i].dataref,
      fabs(ddf->data[i].data-ddf->data[i].dataref),fabs((ddf->data[i].data-ddf->data[i].dataref)/ddf->data[i].dataref),ddf->data[i].interpolate);
  }
  fclose(fout);
  /* fprintf(stderr, "writeLogFile: %s finished\n",filename); */
  return 0;
}

static const char* getTimeVarName(void *vars) {
  const char *res = "time";
  while (RML_NILHDR != RML_GETHDR(vars)) {
    char *var = RML_STRINGDATA(RML_CAR(vars));
    if (0==strcmp("time",var)) break;
    if (0==strcmp("Time",var)) {
      res="Time";
      break;
    }
    vars = RML_CDR(vars);
  }
  return res;
}

void* SimulationResultsCmp_compareResults(int runningTestsuite, const char *filename, const char *reffilename, const char *resultfilename, double reltol, double abstol,  void *vars)
{
  char **cmpvars=NULL;
  char **cmpdiffvars=NULL;
  unsigned int vardiffindx=0;
  unsigned int ncmpvars = 0;
  unsigned int ngetfailedvars = 0;
  void *allvars,*allvarsref,*res;
  unsigned int i,size,size_ref,len,oldlen,j,k;
  char *var,*var1,*var2;
  DataField time,timeref,data,dataref;
  DiffDataField ddf;
  const char *msg[2] = {"",""};
  const char *timeVarName, *timeVarNameRef;
  ddf.data=NULL;
  ddf.n=0;
  ddf.n_max=0;
  oldlen = 0;
  len = 1;

  /* open files */
  /*  fprintf(stderr, "Open File %s\n", filename); */
  if (UNKNOWN_PLOT == SimulationResultsImpl__openFile(filename,&simresglob_c)) {
    char *str = (char*) malloc(21+strlen(filename));
    *str = 0;
    strcat(strcat(str,"Error opening file: "),runningTestsuite ? SystemImpl__basename(filename) : filename);
    void *res = mk_scon(str);
    free(str);
    return mk_cons(res,mk_nil());
  }
  /* fprintf(stderr, "Open File %s\n", reffilename); */
  if (UNKNOWN_PLOT == SimulationResultsImpl__openFile(reffilename,&simresglob_ref)) {
    char *str = (char*) malloc(31+strlen(reffilename));
    *str = 0;
    strcat(strcat(str,"Error opening reference file: "),runningTestsuite ? SystemImpl__basename(reffilename) : reffilename);
    void *res = mk_scon(str);
    free(str);
    return mk_cons(res,mk_nil());
  }

  size = SimulationResultsImpl__readSimulationResultSize(filename,&simresglob_c);
  /* fprintf(stderr, "Read size of File %s size= %d\n", filename,size); */
  size_ref = SimulationResultsImpl__readSimulationResultSize(reffilename,&simresglob_ref);
  /* fprintf(stderr, "Read size of File %s size= %d\n", reffilename,size_ref); */

  /* get vars to compare */
  cmpvars = getVars(vars,&ncmpvars);
  /* if no var compare all vars */
  allvars = SimulationResultsImpl__readVars(filename,&simresglob_c);
  allvarsref = SimulationResultsImpl__readVars(reffilename,&simresglob_ref);
  if (ncmpvars==0){
    cmpvars = getVars(allvarsref,&ncmpvars);
    if (ncmpvars==0) return mk_cons(mk_scon("Error Get Vars!"),mk_nil());
  }
  cmpdiffvars = (char**)malloc(sizeof(char*)*(ncmpvars));
#ifdef DEBUGOUTPUT
  fprintf(stderr, "Compare Vars:\n");
  for(i=0;i<ncmpvars;i++)
    fprintf(stderr, "Var: %s\n", cmpvars[i]);
#endif
  /*  get time */
  /* fprintf(stderr, "get time\n"); */
  timeVarName = getTimeVarName(allvars);
  timeVarNameRef = getTimeVarName(allvarsref);
  time = getData(timeVarName,filename,size,&simresglob_c);
  if (time.n==0)
  {
    return mk_cons(mk_scon("Error get time!"),mk_nil());
  }
  /* fprintf(stderr, "get reftime\n"); */
  timeref = getData(timeVarNameRef,reffilename,size_ref,&simresglob_ref);
  if (timeref.n==0)
  {
    return mk_cons(mk_scon("Error get ref time!"),mk_nil());
  }
  /* check if time is larger or less reftime */
  res = mk_nil();
  if (absdouble(time.data[time.n-1]-timeref.data[timeref.n-1]) > reltol*fabs(timeref.data[timeref.n-1])) {
    char buf[WARNINGBUFFSIZE];
#ifdef DEBUGOUTPUT
    fprintf(stderr, "max time value=%.6g ref max time value: %.6g\n",time.data[time.n-1],timeref.data[timeref.n-1]);
#endif
    res = mk_cons(mk_scon("Warning: Resultfile and Reference have different end time points!\n"),res);
    snprintf(buf,WARNINGBUFFSIZE,"Reffile[%d]=%f\n",timeref.n,timeref.data[timeref.n-1]);
    res = mk_cons(mk_scon(buf),res);
    snprintf(buf,WARNINGBUFFSIZE,"File[%d]=%f\n",time.n,time.data[time.n-1]);
    res = mk_cons(mk_scon(buf),res);
  }
  var1=NULL;
  var2=NULL;
  /* compare vars */
  /* fprintf(stderr, "compare vars\n"); */
  for (i=0;i<ncmpvars;i++) {
    var = cmpvars[i];
    len = strlen(var);
    if (oldlen < len) {
      if (var1) free(var1);
      var1 = (char*) malloc(len+1);
      oldlen = len;
    }
    k = 0;
    for (j=0;j<len;j++) {
      if (var[j] !='\"' ) {
        var1[k] = var[j];
        k +=1;
      }
    }
    var1[k] = 0;
    /* fprintf(stderr, "compare var: %s\n",var); */
    /* check if in ref_file */
    dataref = getData(var1,reffilename,size_ref,&simresglob_ref);
    if (dataref.n==0) {
      if (var2) free(var2);
      var2 = (char*) malloc(len+1);
      strncpy(var2,var1,len+1);
      fixDerInName(var2,len);
      fixCommaInName(&var2,len);
      dataref = getData(var2,reffilename,size_ref,&simresglob_ref);
      if (dataref.n==0) {
        fprintf(stderr, "Get Data of Var %s from file %s failed\n",var,reffilename);
        c_add_message(-1, ErrorType_scripting, ErrorLevel_warning, gettext("Get Data of Var failed!\n"), msg, 0);
        ngetfailedvars++;
        continue;
      }
    }
    /*  check if in file */
    data = getData(var1,filename,size,&simresglob_c);
    if (data.n==0)  {
      fixDerInName(var1,len);
      fixCommaInName(&var1,len);
      data = getData(var1,filename,size,&simresglob_c);
      if (data.n==0)  {
        if (data.data) free(data.data);
        fprintf(stderr, "Get Data of Var %s from file %s failed\n",var,filename);
        c_add_message(-1, ErrorType_scripting, ErrorLevel_warning, gettext("Get Data of Var failed!\n"), msg, 0);
        ngetfailedvars++;
        continue;
      }
    }
    /* compare */
    vardiffindx = cmpData(var,&time,&timeref,&data,&dataref,reltol,abstol,&ddf,cmpdiffvars,vardiffindx);
    /* free */
    if (dataref.data) free(dataref.data);
    if (data.data) free(data.data);
  }

  if (writeLogFile(resultfilename,&ddf,filename,reffilename,reltol,abstol))
  {
    c_add_message(-1, ErrorType_scripting, ErrorLevel_warning, gettext("Cannot write result file!\n"), msg, 0);
  }

  if ((ddf.n > 0) || (ngetfailedvars > 0)){
    /* fprintf(stderr, "diff: %d\n",ddf.n); */
    /* for (i=0;i<vardiffindx;i++)
    fprintf(stderr, "diffVar: %s\n",cmpdiffvars[i]); */
    for (i=0;i<vardiffindx;i++){
      res = (void*)mk_cons(mk_scon(cmpdiffvars[i]),res);
    }
    res = mk_cons(mk_scon("Files not Equal!"),res);
    c_add_message(-1, ErrorType_scripting, ErrorLevel_warning, gettext("Files not Equal\n"), msg, 0);
  }
  else
    res = mk_cons(mk_scon("Files Equal!"),res);

  if (var1) free(var1);
  if (var2) free(var2);
  if (ddf.data) free(ddf.data);
  if (cmpvars) free(cmpvars);
  if (time.data) free(time.data);
  if (timeref.data) free(timeref.data);
  if (cmpdiffvars) free(cmpdiffvars);
  /* close files */
  SimulationResultsImpl__close(&simresglob_c);
  SimulationResultsImpl__close(&simresglob_ref);

  return res;
}

