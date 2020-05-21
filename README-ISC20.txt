In the ISC20 list, we run the new C-application together with the previous scripts for comparison purposes.

Use the io500.sh script to run both a pass with the bash-script and a
second run with the C-app.  The results will be put into two separate
directories and tarred up for submission.  The io500.sh script also
uses the same .ini file for most options, but at a minimum needs the
io500_mpirun and io500_mpiargs values set for your system in the
setup_paths() function at the start of the io500.sh script.  The
paths to the installed IOR and mdtest binaries are also needed if
they were installed separately.  The script must be run from within
the Git checkout tree.

In detail, to setup the run:
1) Create a system information -- this can be done later after the run as well but please ensure it is consistent.
Visit the page: https://vi4io.org/io500-info-creator/
and store the output into "system-information.txt"

2) Setup a configuration, the minimal configuration is provided in config-minimal.ini describing the data directory.
To see available options, either see the "config-full.ini" file or run
   $ ./io500 --list
Change the MPI options in io500.sh
   io500_mpiargs='-np 2'
   io500_mpirun="mpiexec"

It may also be desirable to update the setup_directories() function 
to create the top-level output directories for the test files and set, e.g., striping parameters for Lustre.
These directories are created before the test run for both C-App and IO500 script, and may hold directory and
file layout parameters.

3) Create a batch submission file which executes the benchmark script.
   This in turn will runs both, the C-App and the shell script version by using:
   $ ./io500.sh <file.ini>
Example:
   $ ./io500.sh config-minimal.ini
   This will then automatically create a TAR file for upload.

4) Submit the TAR file together with system-information.txt and the batch file via the submission page.

   You can find examples for running the scripts and the generated output
   * for Lustre: http://vi4io.org/assets/io500/2020-06/isc-20-dkrz-10nodes.tgz
