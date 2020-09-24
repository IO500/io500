In the SC20 list, only the new C-application benchmark will be used.

The io500.sh script can optionally be used to prepare the output
directories, launch the io500 application, and collect the test results
and logs for submission, or the io500 binary can be run directly.

The test results will be put into a separate directory and tarred up for
submission.  At a minimum, the io500.sh script needs the io500_mpirun and
io500_mpiargs values set for your system in the setup_paths() function
at the start.  The paths to the installed IOR and mdtest binaries are
also needed if they were installed separately.  The script must be run
from within the Git checkout tree.

In detail, to setup the run:
1) Create a system information -- this can be done later after the
   run as well but please ensure it is consistent.
   Visit the page: https://vi4io.org/io500-info-creator/
   and store the output into "system-information.txt"

2) Setup a configuration, the minimal configuration is provided in
   config-minimal.ini describing the data directory.
   To see available options, either see the "config-full.ini" file or run
      $ ./io500 --list
   Change the MPI options in io500.sh
      io500_mpiargs='-np 2'
      io500_mpirun="mpiexec"

It may also be desirable to update the setup_directories() function 
to create the top-level output directories for the test files and set,
e.g., striping parameters for Lustre.  These directories are created
before the run, and may hold directory and file layout parameters.

3) Create a batch submission file which executes the benchmark script.
   This in turn runs the io500 application using:
      $ ./io500.sh <file.ini>
   Example:
      $ ./io500.sh config-minimal.ini
   This will then automatically create a .tar file for upload.

4) Submit the .tgz file together with system-information.txt and the
   batch file via the submission page.

   You can find examples for running the scripts and the generated output
   * for Lustre: http://vi4io.org/assets/io500/2020-06/isc-20-dkrz-10nodes.tgz
