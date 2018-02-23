# mathbench

This is a little program designed to understand the perormance characteristics
of libm math functions. It is designed to answer questions like:
* Does the input deck have suitable coverage?
* How many performance steps does this implementation have?

Ultimately, this tool is intended to compare different implementations
of functions. so that the various implementations of the math
libraries can be compared in terms of things like:
* What are the absolute times to compute the value using this implementation
* What is the probability of hitting the various performance steps.
* How well does this implementation perform in selected targeted ranges.

Math functions tend to have very specific performance characteristics
where different input parameters take different amounts of time than
others. Plotted along a graph these numbers fall into a step function
with gently sloping lines. This program gathers the time needed to
calculate the sum of some number of calls to a particular math
function and then tries to identify ranges of times which represent
those various steps.

## Usage

The general idea is:

./time_math2 -t -d input-decks/exp-inputs-hard

The general idea is it will take the list of numbers fed into it via
the input deck and then it runs something like your time_math1.c and
then it groups the numbers into ranges based upon time.

9 Ranges:
2-101 c:100     m:1445  r:1357:1495
102-117 c:16    m:2014  r:1981:2105
118-213 c:96    m:2647  r:2473:2736
215-357 c:143   m:3332  r:3220:3501
361-538 c:178   m:275710        r:231875:366838
539-554 c:16    m:444913        r:382567:481886
557-746 c:190   m:2404787       r:2280278:2866830
748-750 c:3     m:3270449       r:3227605:3327688
751-755 c:4     m:3955378       r:3927310:5240862

What you can see from this is glibc's libm really tends to fall into 9
ranges of time to compute. The first column is the range of numbers in
the input-deck. The 'c' is the count of items that fall in that
range. 'm' is the median of all the numbers that fall in that range
and then 'r' is the actual range of times that the values that fall in
that range.

-d is dump the numbers used to make the range

-t is target the numbers provided in the input deck. It looks at the
 numbers adjacent to the ones provided in the input deck and then
 tries those out. The idea with this is to see how likely we are to
 hit these worst case values. Is it just one particular number that is
 bad or is it a whole range of numbers. In that area which perform
 badly which can suggest a problem with the polynomial or function
 used to calculate in that vicinity.

I also have a -r mode which basically tries random numbers.This
currently doesn't work really well. I think that I need to constrain
it to a range so that it just checks something like 0.0-2.0 or
something.

The rest are just tuning parameters.

## Author

* **Ben Woodard <woodard@redhat.com>**

## License

This is a toy program. Giving it a license seems excessive. Never the
less consider it under the GPLv3. If someone wants some other license
contact me.