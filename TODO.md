#Todo
## Basic functionality

* At the end of the initial ranging run, print the domain of the
  function covered. This gives some indication of the coverage of test
  suite

* At the end of the initial ranging run, print the number of test
  cases that hit a particular range. This could be used to show if we
  have an over representation of a kind of test case.

* When doing a targeted run time each value tested. Some numbers are
  fast and some numbers are very slow and this represents problem with
  the polynomial in this area.

* Print timing for the entire run.

* When doing targeted tests skip numbers that are within each other's
  range because that is essentially doing the same test.

* Add a usage message

* fix the random mode

* add two variable targeted range

* add small rational polynomial mode for pow

* verify that it runs on ARM and Power

## Documentation

Some documentation would be nice

* Explain the process used by this tool.  Start with a data set like
  glibc's benchtests. That should give you some initial range
  data. Then do some random tests to make sure that the ranges hold
  up. Add these numbers to the input deck for the function. Then do
  some targeted runs to detect bad polynomials.

Feb 9 2018 A mode that randomly picks numbers optionally within a
range and then calculates the probability that numbers will hit a
particular performance range.
Feb 9 2018 do timing based on papi rather than gettimeofday()
Feb 9 2018 Run on a GPU
