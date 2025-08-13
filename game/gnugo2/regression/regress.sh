#/bin/sh

echo -----------------------------------------------------------------------
echo Here is a test of an annotated game test-tree.sgf
echo -----------------------------------------------------------------------
echo ../src/gnugo --mode test --testmode annotation --infile test-tree.sgf --quiet
../src/gnugo --mode test --testmode annotation --infile test-tree.sgf --quiet

echo -----------------------------------------------------------------------
echo Here is a test of a full game to see if GNU Go would have considered
echo the moves that were made.
echo -----------------------------------------------------------------------
echo ../src/gnugo --mode test --testmode game --infile gnugo_gnugo.sgf --quiet
../src/gnugo --mode test --testmode game --infile gnugo_gnugo.sgf --quiet
