#pragma once

/**
Auxiliary  class to handle system events
Implements the Modelica pre,edge,change operators
Holds a help vector for the discrete variables
Holds an event queue to handle all events occured at the same time
*/

class BOOST_EXTENSION_EVENTHANDLING_DECL EventHandling
{
public:
  EventHandling();
  virtual ~EventHandling(void);
  //Inits the event variables
  //void initialize(IEvent* system, int dim, init_prevars_type init_prevars);
  void initialize(IEvent* system, int dim);
  //Returns the help vector
  //void getHelpVars(double* h);
  //sets the help vector
  //void setHelpVars(const double* h);
  //returns the dimension of the help vector
  //int getDimHelpVars() const;

  //saves a variable in _pre_vars vector
  void save(double& var);
  void save(int& var);
  void save(bool& var);
  void savePreVars(double vars [], unsigned int n);

  //saves all helpvariables
  //void saveH();
  // void setHelpVar(unsigned int i,double var);
  //const double& operator[](unsigned int i) const;
  //Implementation of the Modelica pre  operator
  double pre(double& var);
  //Implementation of the Modelica edge  operator
  bool edge(double& var);
  //Implementation of the Modelica change  operator
  bool change(double& var);
  //Implementation of the Modelica pre  operator
  double pre(int& var);
  //Implementation of the Modelica edge  operator
  bool edge(int& var);
  //Implementation of the Modelica change  operator
  bool change(int& var);
  //Implementation of the Modelica pre  operator
  double pre(bool& var);
  //Implementation of the Modelica edge  operator
  bool edge(bool& var);
  //Implementation of the Modelica change  operator
  bool change(bool& var);
  //Adds an event to the eventqueue
  //void addEvent(long index);
  //removes an event from the eventqueue
  //void removeEvent(long index);
  //Handles  all events occured a the same time. Returns true if a second event iteration is needed
  bool IterateEventQueue(bool& state_vars_reinitialized);

  
  bool changeDiscreteVar(double& var);
  bool changeDiscreteVar(int& var);
  bool changeDiscreteVar(bool& var);
  getCondition_type getCondition;

private:
  //Stores all varibales occured before an event
  unordered_map<double* const, unsigned int> _pre_real_vars_idx;
  //stores all eventes
  unordered_map<int* const, unsigned int> _pre_int_vars_idx; 
   //stores all eventes
  unordered_map<bool* const, unsigned int> _pre_bool_vars_idx; 
  IEvent* _event_system;
  //Helpvarsvector for discrete variables
  //double* _h;
  //Dimesion of Helpvarsvector
  //int _dimH;
  event_times_type _time_events;
  boost::multi_array<double,1> _pre_vars;
  boost::multi_array<double,1> _pre_discrete_vars;

  IContinuous* _countinous_system; //just a cast of _event_system -> required in IterateEventQueue
  IMixedSystem* _mixed_system; //just a cast of _event_system -> required in IterateEventQueue
  bool* _conditions0;
  bool* _conditions1;
};
