// name: loop2.mo
// keywords:
// status: correct
// cflags:   +d=scodeInst
//


model A
  Real x = x + 1;
end A;

// Result:
// class A
//   Real x = 1.0 + x;
// end A;
// endResult
