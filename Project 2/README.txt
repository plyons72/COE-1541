Experiment 1:

  This experiment was conducted by running each trace file with a different
  cache_config.txt file. In each case, we tested every combination where the
  I Cache Size = D Cache size, where values can be (1KB, 16KB, or 128KB), and
  I Block Size = D Block size, where values can be (4B, 16B, 64B, or 256B). The
  associativity is constantly 4.

  Here is an example of our output:

    ** opening file sample_large1.tr

    Cache Size: 1 KB
    Associativity: 4
    Block Size: 4 Bytes

    + Simulation terminates at cycle : 388618939
    I-cache accesses 93672795 and misses 434006
    D-cache Read accesses 20813032 and misses 9098182
    D-cache Write accesses 9625833 and misses 1861519

  The first line tells us which trace file is being used. The next 3 tell us the
  various sizes of cache properties, and finally, we see the execution time in
  cycles, along with the access and miss stats for the caches. To get the miss
  rates, we simply take I_Misses/I_Accesses * 100% to get the miss rate for
  the I Cache, and (D_Read_Miss + D_Write_Miss)/(D_Read_Access + D_Write_Access)
  * 100% for the D Cache. All of this information is shown in the excel sheets.
  The first sheet in the workbook is for sample_large1.tr output, and the second
  is for sample_large2.tr.

  In this workbook, the data near the top is the miss rates as percentages, and
  the data below that is without the * 100% calculation.



Experiment 2:
  In this experiment, we looked at the values from experiment 1, and simply
  chose values for the cache size that gave us comparatively low miss rates
  while not being too large. Below is the output for the first trace file.

    ** opening file sample_large1.tr

    I Cache Size: 16 KB
    I Cache Associativity: 4
    I Cache Block Size: 4 Bytes


    D Cache Size: 16 KB
    D Cache Associativity: 4
    D Cache Block Size: 256 Bytes

    + Simulation terminates at cycle : 233357279
    I-cache accesses 93672795 and misses 1667
    D-cache Read accesses 20813032 and misses 4879923
    D-cache Write accesses 9625833 and misses 296516

    I-cache miss rate = 0.000018
    D-cache miss rate: 0.170060

  We added output to the CPU+cache.c file to differentiate between the cache
  sizes, since they are no longer implicitly the same. Associativity remained 4,
  but for I Cache, we used a 16KB Cache size with a 4 Byte block size. For
  D Cache, we used 16KB and 256 Bytes. Looking at the excel sheet for the first
  trace file, we see that a 4 Byte block size already has a small miss rate for
  the I type cache, but the 16KB cache size miss rate is nearly insignificant
  compared to the 1 KB cache size miss rate (about 200x smaller). Any block size
  larger than 4 Bytes gives a smaller miss rate, but the value here is already
  so insignificant, we don't gain much from the increased size.

  Using the same reasoning for the D-Cache, we chose 16KB and 256 Bytes for the
  cache/block sizes, based on the Long Trace 2 results. The lowest miss rate
  would be with 128KB/256 Bytes, but that still leaves a 22% miss rate. We can
  decrease the cache size by 8x, and only increase out miss rate to 24%. Any
  other decreases in cache or block size leave us with at least a 42% miss rate,
  which is a bit high, and could hurt our execution time.

  Speaking of, with these values, we get an execution time of 233357279 for the
  first trace, which gives it a lower execution time than most of the others for
  the values used in the first experiment.


  Experiment 3:
    This experiment was pretty simple. We simply ran the trace files with an
    appropriate cache_config.txt file, where the cache sizes and block sizes
    were 32KB and 32 Bytes respectively, and we changed the Associativity
    between 1, 4, and 8. We then plotted these results in a bar graph to show
    how the miss rates changed. It is much more noticeable in the first trace
    file results, but the miss rate decreased consistently as we increase
    associativity. The graphs and data are on the third sheet of our excel
    workbook (3rd tab on bottom of the sheet labeled "Experiment 3"). The output
    of the trace files is in Experiment 3.txt.


  NOTE THAT OUTPUT MAY BE SLIGHTLY DIFFERENT ON YOUR END. WE ADDED AND REMOVED SOME
  PRINTF STATEMENTS TO HELP WITH CALCULATIONS AND LABELING CERTAIN THINGS IN A NEATER
  WAY FOR TABULATION AND EVALUATION ON A PER-EXPERIMENT BASIS.
