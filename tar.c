#define _POSIX_C_SOURCE 200809L // required for PATH_MAX on cslab

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <limits.h>

int firstcall; 

/* create a new string dir/file */
char *build_path(char *dir, char *file);

/* create an archive of dir in the archive file */
void create(FILE *archive, char *dir, int verb);

/* read through the archive file and recreate files and directories from it */
void extract(FILE *archive, int verb);

/* standard usage message and exit */
void usage();

/* create a new string dir/file */
char *build_path(char *dir, char *file)
{
    // allocate space for dir + "/" + file + \0 
    int path_length = strlen(dir) + 1 + strlen(file) + 1;
    char *full_path = (char *)malloc(path_length*sizeof(char));

    // fill in full_path with dir then "/" then file
    strcpy(full_path, dir);
    strcat(full_path, "/");
    strcat(full_path, file);

    // be sure to free the returned memory when it's no longer needed
    return full_path;
}

/* create an archive of dir in the archive file */
void create(FILE *archive, char *dir, int verb)
{
	//open directory
	struct dirent *de; 
	DIR *d = opendir(dir);
        //Error checking of directory open	
	if (d == NULL){
		perror(dir);
		exit(1);
	}
	struct stat firststat;
	if (stat(dir, &firststat) ==-1){
		perror(dir);
	}
	if (S_ISDIR(firststat.st_mode) && strcmp(dir, ".") != 0 && firstcall ==0){
		if (verb == 1)
			printf("processing: %s\n", dir);
		fprintf(archive, "%s\n", dir);
		fwrite(&firststat, sizeof(struct stat), 1, archive);
		firstcall +=1; 
	}
	//loop through directory 
	for (de = readdir(d); de != NULL; de = readdir(d)){
		//Skip parent directory
		
		if (strcmp(de->d_name, "..")==0)
			continue;
		if (strcmp(de->d_name, ".") ==0)
			continue;
		
		char *path = build_path(dir, de->d_name);
		struct stat finfo;
		//stat call
		if (stat(path, &finfo) == -1){
			perror(path);
			continue;
		}
		if (verb ==1)
			printf("processing: %s\n", path);
		//if directory, put into archive and recursive call
		if (S_ISDIR(finfo.st_mode) && strcmp(de->d_name, ".") != 0)
		{
			fprintf(archive, "%s\n", path);
			fwrite(&finfo, sizeof(struct stat), 1, archive);
			create(archive, path, verb);
		}
		//Not a directory, so add the needed info to archive
		else if (S_ISREG(finfo.st_mode)){
			fprintf(archive, "%s\n", path);  
			fwrite(&finfo, sizeof(struct stat), 1, archive); 
			//actual file data
			//open file, malloc the length
			int length = finfo.st_size;  
			char *buf = (char *)malloc(length*sizeof(char));
			//open file
			FILE *in_fd;
			in_fd = fopen(path, "r");
			if (in_fd == NULL){
				perror(path);
				exit(1);
			}
			//read file
			
			int read_return = fread(buf, sizeof(char), length, in_fd);
			if (read_return != length){
				perror("fread");
				exit(1);
			}
			//write file to archive
			fwrite(buf, sizeof(char), length, archive);
			if (fclose(in_fd) == EOF)
				perror("close");
		else{
			continue;
		}
		}
		free(path);
	}

	//close dir
	if (closedir(d) !=0){
		perror("closedir");
	}
}

/* read through the archive file and recreate files and directories from it */
void extract(FILE *archive, int verb)
{
	char filename[PATH_MAX];
	struct stat einfo; 
	int scanreturn;

	scanreturn = fscanf(archive, "%s\n", filename); 
	if (scanreturn == 0){
		perror("fscanf");
		exit(1);
	}
	

	scanreturn = fread(&einfo, sizeof(struct stat), 1, archive);
	if (scanreturn < 0){
		perror("fread");
	}
	while(scanreturn != 0){
		if (verb ==1)
			printf("%s: processing\n", filename);
		if (S_ISDIR(einfo.st_mode)){
			if (mkdir(filename, einfo.st_mode)!=0)
				perror("mkdir");
		}
		if (S_ISREG(einfo.st_mode)){
			
			int length = einfo.st_size;
			char *buf = (char *)malloc(length*sizeof(char));
			scanreturn = fread(buf, sizeof(char), length, archive);
			FILE *fp = fopen(filename, "w");
			fwrite(buf, sizeof(char), length, fp);
			
		}
	
		fscanf(archive, "%s\n", filename);
		scanreturn = fread(&einfo, sizeof(struct stat), 1, archive);
		if (scanreturn < 0)
			perror("fread"); 
	}
	

}

/* standard usage message and exit */
void usage()
{
    printf("usage to create an archive:  tar c[v] ARCHIVE DIRECTORY\n");
    printf("usage to extract an archive: tar x[v] ARCHIVE\n\n");
    printf("  the v parameter is optional, and if given then every file name\n");
    printf("  will be printed as the file is processed\n");
    exit(1);
}

int main(int argc, char **argv)
{
    int verbose;
    FILE *arch;

    // must have exactly 2 or 3 real arguments
    if (argc < 3 || argc > 4)
    {
        usage();
    } 

    // 3 real arguments means we are creating a new archive
    if (argc == 4)
    { 
        // check for the create flag without the verbose flag
        if (strcmp(argv[1],"c") == 0)
        {
            verbose = 0;
        }
        // check for the create flag with the verbose flag
        else if (strcmp(argv[1],"cv") == 0)
        {
            verbose = 1;
        }
        // anything else is an error
        else
        {
            usage();
        }

        // get the archive name and the directory we are going to archive
        char *archive = argv[2];
        char *dir_name = argv[3];

        // remove any trailing slashes from the directory name
        while (dir_name[strlen(dir_name)-1] == '/')
        {
            dir_name[strlen(dir_name)-1] = '\0';
        }

        // open the archive
        arch = fopen(archive, "w");
        if (arch == NULL)
        {
            perror(archive);
            exit(1);
        }

        // start the recursive process of filling in the archive
        create(arch, dir_name, verbose);
    }

    // 2 real arguments means we are creating a new archive
    else if (argc == 3)
    { 
        // check for the extract flag without the verbose flag
        if (strcmp(argv[1],"x") == 0)
        {
            verbose = 0;
        }
        // check for the extract flag with the verbose flag
        else if (strcmp(argv[1],"xv") == 0)
        {
            verbose = 1;
        }
        // anything else is an error
        else
        {
            usage();
        }

        // get the archive file name
        char *archive = argv[2];

        // open the archive
        arch = fopen(archive, "r");
        if (arch == NULL)
        {
            perror(archive);
            exit(1);
        }

        // start the recursive processing of this archive
        extract(arch, verbose);
    }

    return 0;
}
