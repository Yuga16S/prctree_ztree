#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <ftw.h>
#include <stdbool.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>

void killZombieParents(int, int, int);
struct ProcessStat getProcessStat(int);

struct ProcessStat {
	int pid;
	int ppid;

	bool defunct;

	int numberOfChildren;
	int * children; // points to an array of integers

	int numberOfDefunctChildren;

	int elapsedMinutes;
};

int main(int argc, char * argv[]) {

	if (!(argc == 2 || argc == 4)) {
		printf("Invalid arguments. Please run the command as ztree [root_process] [OPTION1 -t | -b] [OPTION2 PROC_ELTIME | NO_OF_DFCS] \n");
		return 0;
	}

	int root_process;

	char * option1;
	int option2;

    sscanf(argv[1], "%d", &root_process);

    if (argc == 4) {
    	option1 = argv[2];
    	sscanf(argv[3], "%d", &option2);

    	if (strcmp(option1, "-t") == 0) {
    		killZombieParents(root_process, option2, 0);
    	} else if (strcmp(option1, "-b") == 0) {
    		killZombieParents(root_process, 0, option2);
    	} else {
    		printf("Invalid option\n");
    	}

    } else { // no options
    	killZombieParents(root_process, 0, 0);
    }

	return 0;
}

void killZombieParents(int pid, int PROC_ELTIME, int NO_OF_DFCS) {
	struct ProcessStat processStat = getProcessStat(pid);
	for (int i = 0; i < processStat.numberOfChildren; i++) {
		int childPid = processStat.children[i];

		killZombieParents(childPid, PROC_ELTIME, NO_OF_DFCS);
	}

	if (processStat.numberOfDefunctChildren > 0) {
		if (PROC_ELTIME > 0 && processStat.elapsedMinutes < PROC_ELTIME) {
			return;
		}

		if (NO_OF_DFCS > 0 && processStat.numberOfDefunctChildren < NO_OF_DFCS) {
			return;
		}

		kill(pid, 9);
		printf("%d\n", pid);
	}
}

struct ProcessStat getProcessStat(int pid) {
	struct ProcessStat processStat;
	processStat.pid = pid;

	char filename[1000];
    sprintf(filename, "/proc/%d/status", pid);
    FILE *f = fopen(filename, "r");

    char line[1000];
    while(fgets(line, 1000, f) != NULL) {
    	

    	if (strstr(line, "State:") != NULL) {
    		char * linePtr = line + strlen("State:");
    		processStat.defunct = (strstr(line, "Z") != NULL);
    	}

    	if (strstr(line, "PPid:") != NULL) {
    		int ppid;
    		sscanf(line, "PPid:%d", &ppid);
    		processStat.ppid = ppid;
    	}
    }

    fclose(f);


  	struct stat statusFileStat;
  	stat(filename, &statusFileStat);

  	time_t modTime = statusFileStat.st_mtime;
	time_t currentTime;
    time(&currentTime);

    double timeElapsed = difftime(currentTime, modTime)/60;
    processStat.elapsedMinutes = timeElapsed;

    sprintf(filename, "/proc/%d/task/%d/children", pid, pid);
    f = fopen(filename, "r");


  	char fileContent[1000];
  	fread(fileContent, 1, 1000, f);
  	

  	int numberOfChildren = 0;
  	for (int i = 0; i < 1000; i++) {
  		if (fileContent[i] == ' ') {
  			numberOfChildren++;
  		} else if (fileContent[i] == '\0') {
  			break;
  		}
  	}

  	processStat.numberOfChildren = numberOfChildren;
  	processStat.children = malloc(numberOfChildren * sizeof(int));

  	int numberOfDefunctChildren = 0;
  	char * childrenString = fileContent;
  	for (int i = 0; i < numberOfChildren; i++) {
  		int childPid;
  		char *token = strtok(childrenString, " ");
  		childrenString = childrenString + (strlen(token) + 1);

  		sscanf(token, "%d", &childPid);

  		processStat.children[i] = childPid;

  		struct ProcessStat childProcessStat = getProcessStat(childPid);
  		if (childProcessStat.defunct) {
  			numberOfDefunctChildren++;
  		}
  	}

  	processStat.numberOfDefunctChildren = numberOfDefunctChildren;

  	fclose(f);

    return processStat;
}


