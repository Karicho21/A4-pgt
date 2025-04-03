
This program is for ITCS4145 Assignment 4, Parallel Graph-crawer

This program aims to find neighboring nodes of node you specify. This database holds cast members' name and quite a lot of movies. I hope you enjoy!
But before you begin, make sure you have access to:

- A Linux-based system with SLURM workload management
- gcc compiler (this is critical to execute the program)
- Access to hpc lab computer Centaurus
- UNC Charlotte VPN if you are outside of campus or not using "eduroam" network

There are two ways to run this program.

1. Using centaurus
2. Using your local machine

I highly recommend using Centaurus since all of the libraries are available already. I will get to the way to use this program win local machine later on.

Way to execute this program using Centaurus:
1. Connecting hpc lab computer by "ssh hpc-student.charlotte.edu -l your-username"
2. Authenticate with Duo
3. Type "g++ gsc.cpp -o gsc -I ~/rapidjson/include -lcurl" gsc is the name of the executable file and gsc.cpp is the source code. g++ allows us to make executable file that is binary so CPU can process the high level program.
4. Schedule the job by "sbatch gsc.sh"
5. Outcome should be something like "Submitted batch job [????]", pay attention to the number.
6. Wait a bit for command to finish running and record the time it takes.
7. There are 3 jobs scheduled in gsc.sh file. To look at the time it took for each process, open record.txt by typing "cat record.txt"

Keys: If you accidentally miss components when you try to execute, it will give you an error message that says "Missing component(s) while trying to execute gsc file. make sure to type and ". Start node can be anything as long as it is in the database. To follow the instruction, I put Tom Hanks. I don't know him but he is in a lot of movies! If you would like to run the progarm with your sets of data, you can execute it by "./gsc <node> <depth> "

Records from my end: (It vaires every time, but should be similar)

./gsc "Tom Hanks" 1: Recorded Time: 0.176366 seconds
./gsc "Tom Hanks" 2: Recorded Time: 0.769465 seconds
./gsc "Tom Hanks" 3: Recorded Time: 11.3539 seconds
