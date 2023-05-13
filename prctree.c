#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <ftw.h>
#include <stdbool.h>
#include <sys/types.h>

int getParentProcessId(int);
bool isDescendant(int, int);
struct ProcessStat getProcessStat(int);

struct ProcessStat {
	int pid;
	int ppid;

	bool defunct;

	int numberOfChildren;
	int * children; // points to an array of integers
};

int main(int argc, char * argv[]) {

	if (argc < 3 || argc > 4) {
		printf("Invalid arguments. Please run the command as prctree [root_process] [process_id] [-option c|s|gp|gc|z|zl ] \n");
		return 0;
	}

	int root_process, process_id;

	char * option;

    sscanf(argv[1], "%d", &root_process);
    sscanf(argv[2], "%d", &process_id);

    if (argc == 4) {
    	option = argv[3];
    }

	if (isDescendant(root_process, process_id)) {
		printf("%d %d \n", process_id, getParentProcessId(process_id));

		if (option != NULL && strcmp(option, "-c") == 0) {
			struct ProcessStat processStat = getProcessStat(process_id);

			for (int i = 0; i < processStat.numberOfChildren; i++) {
    			printf("%d\n", processStat.children[i]);
    		}
		} else if (option != NULL && strcmp(option, "-s") == 0) {
			struct ProcessStat processStat = getProcessStat(process_id);
			struct ProcessStat parentProcessStat = getProcessStat(processStat.ppid);


			for (int i = 0; i < parentProcessStat.numberOfChildren; i++) {
				int siblingPid = parentProcessStat.children[i];
				if (siblingPid != process_id) { // ignorning the process_id in sibling
    				printf("%d %d\n", parentProcessStat.children[i], parentProcessStat.pid);
    			}
    		}
		} else if (option != NULL && strcmp(option, "-gp") == 0) {
			struct ProcessStat processStat = getProcessStat(process_id);
			struct ProcessStat parentProcessStat = getProcessStat(processStat.ppid);
    		printf("%d\n", parentProcessStat.ppid);		
		} else if (option != NULL && strcmp(option, "-gc") == 0) {
			struct ProcessStat processStat = getProcessStat(process_id);
			for (int i = 0; i < processStat.numberOfChildren; i++) {
				int childPid = processStat.children[i];
 				struct ProcessStat childProcessStat = getProcessStat(childPid);

 				for(int j = 0; j < childProcessStat.numberOfChildren; j++) {
 					int grandChildPid = childProcessStat.children[j];
 					printf("%d %d\n", grandChildPid, childProcessStat.pid);
 			 	}
 			} 	
		} else if (option != NULL && strcmp(option, "-z") == 0) {
			struct ProcessStat processStat = getProcessStat(process_id);
			if (processStat.defunct) {
				printf("DEFUNCT\n");
			} else {
				printf("NOT DEFUNCT\n");
			}				
		} else if (option != NULL && strcmp(option, "-zl") == 0) {
			struct ProcessStat processStat = getProcessStat(process_id);

			for (int i = 0; i < processStat.numberOfChildren; i++) {
				int childPid = processStat.children[i];
 				struct ProcessStat childProcessStat = getProcessStat(childPid);
				if (childProcessStat.defunct) {
					printf("%d\n", childPid);
				}
    		}
		} else {
			printf("Invalid option\n");
		}

	} else {
		// no output
	}

    // struct ProcessStat processStat = getProcessStat(process_id);
    // printf("Pid: %d\n", processStat.pid);
    // printf("PPid: %d\n", processStat.ppid);
    // printf("Defunct: %d\n", processStat.defunct);
    // printf("Number of Children: %d\n", processStat.numberOfChildren);
    // for (int i = 0; i < processStat.numberOfChildren; i++) {
    // 	printf("Children #%d: %d\n", i + 1, processStat.children[i]);
    // }

	return 0;
}


int getParentProcessId(int pid) {
    struct ProcessStat processStat = getProcessStat(pid);
    return processStat.ppid;
}

bool isDescendant(int rootPid, int pid) {
	if (pid == rootPid) {
		return true;
	} else {
		if (pid == 1) {
			return false;
		}

		int ppid = getParentProcessId(pid);
		return isDescendant(rootPid, ppid);
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

  	for (int i = 0; i < numberOfChildren; i++) {
  		int childPid;
  		char *token = strtok(i == 0 ? fileContent : NULL, " ");
  		sscanf(token, "%d", &childPid);

  		processStat.children[i] = childPid;
  	}

  	fclose(f);

    return processStat;
}


