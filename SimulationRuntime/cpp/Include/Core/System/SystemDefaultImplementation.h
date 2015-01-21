#pragma once

#ifdef ANALYZATION_MODE
#include <Core/Math/Functions.h>
#include <Core/System/EventHandling.h>
#include <boost/any.hpp>
#include <boost/unordered_map.hpp>
#include <boost/circular_buffer.hpp>
#include <iostream>
#include <Core/System/IContinuous.h>
#include <Core/SimulationSettings//IGlobalSettings.h>
#endif

/*****************************************************************************/
/**

Services, which can be used by systems.
Implementation of standart functions (e.g. giveRHS(...), etc.).
Provision of member variables used by all systems.

Note:
The order of variables in the extended state vector perserved (see: "Sorting
variables by using the index" in "Design proposal for a general solver interface
for Open Modelica", September, 10 th, 2008


\date     October, 1st, 2008
\author

*/
/*****************************************************************************
Copyright (c) 2008, OSMC
*****************************************************************************/

//typedef boost::unordered_map<std::string, boost::any> SValuesMap;

template <class T>
class InitVars
{
public:
  void setStartValue(T& variable,T val);
  T& getGetStartValue(T& variable);

private:
  boost::unordered_map<T*, T> _start_values;
};

class BOOST_EXTENSION_SYSTEM_DECL SystemDefaultImplementation: public virtual PreVariables
{
public:
  SystemDefaultImplementation(IGlobalSettings* globalSettings);
  virtual ~SystemDefaultImplementation();

  /// Provide number (dimension) of boolean variables
  virtual int getDimBoolean() const;

  /// Provide number (dimension) of states
  virtual int getDimContinuousStates() const;

  /// Provide number (dimension) of integer variables
  virtual int getDimInteger() const;

  /// Provide number (dimension) of real variables
  virtual int getDimReal() const;

  /// Provide number (dimension) of string variables
  virtual int getDimString() const;

  /// Provide number (dimension) of right hand sides (equations and/or residuals) according to the index
  virtual int getDimRHS() const;

  /// Provide boolean variables
  virtual void getBoolean(bool* z);

  /// Provide boolean variables
  virtual void getContinuousStates(double* z);

  /// Provide integer variables
  virtual void getInteger(int* z);

  /// Provide real variables
  virtual void getReal(double* z);

  /// Provide real variables
  virtual void getString(std::string* z);

  /// Provide the right hand side
  virtual void getRHS(double* f);

  virtual void  setConditions(bool* c);
  virtual void getConditions(bool* c);
  /// Provide boolean variables
  virtual void setBoolean(const bool* z);

  /// Provide boolean variables
  virtual void setContinuousStates(const double* z);

  /// Provide integer variables
  virtual void setInteger(const int* z);

  /// Provide real variables
  virtual void setReal(const double* z);

  /// Provide real variables
  virtual void setString(const std::string* z);

  /// Provide the right hand side
  virtual void setRHS(const double* f);

  /// (Re-) initialize the system of equations
  void initialize();
  /// Set current integration time
  void setTime(const double& t);

protected:
    void Assert(bool cond, const string& msg);
    void Terminate(string msg);
    void intDelay(vector<unsigned int> expr,vector<double> delay_max);
    void storeDelay(unsigned int expr_id,double expr_value,double time);
    void storeTime(double time);
    double delay(unsigned int expr_id,double expr_value, double delayTime, double delayMax);
    bool isConsistent();

    double& getRealStartValue(double& var);
    bool& getBoolStartValue(bool& var);
    int& getIntStartValue(int& var);
    void setRealStartValue(double& var,double val);
    void setBoolStartValue(bool& var,bool val);
    void setIntStartValue(int& var,int val);

    double
        _simTime;             ///< current simulation time (given by the solver)


    bool
        * _conditions,        ///< External conditions changed by the solver
        * _time_conditions;

    int
        _dimContinuousStates,
        _dimRHS,              ///< Dimension der rechten Seite
        _dimReal,             ///< Anzahl der reelwertigen Variablen
        _dimInteger,          ///< Anzahl der integerwertigen Variablen
        _dimBoolean,          ///< Anzahl der boolwertigen Variablen
        _dimString,           ///< Anzahl der stringwertigen Variablen
        _dimZeroFunc,         ///< Dimension (=Anzahl) Nullstellenfunktion
        _dimTimeEvent,        ///< Dimension (=Anzahl) Time event (start zeit und frequenz)
        _dimAE;               ///< Number (dimension) of algebraic equations (e.g. constraints from an algebraic loop)

    int
    * _time_event_counter;
    std::ostream *_outputStream;        ///< Output stream for results

    IContinuous::UPDATETYPE _callType;

    bool _initial;
    bool _terminal;
    bool _terminate;

    //SValuesMap _start_values;
    InitVars<double> _real_start_values;
    InitVars<int> _int_start_values;
    InitVars<bool> _bool_start_values;

    EventHandling _event_handling;

    typedef boost::circular_buffer<double> buffer_type;
    map<unsigned int, buffer_type> _delay_buffer;
    buffer_type _time_buffer;
    double _delay_max;
    double _start_time;
    IGlobalSettings* _global_settings; //this should be a reference, but this is not working if the libraries are linked statically
};


