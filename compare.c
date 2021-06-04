//Including the libraies.
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <dirent.h>
#include <math.h>
#include <ctype.h>

#include "lists.h"

//DEBUG
#ifndef CHECK
#define CHECK 0
#endif

typedef struct ana_th_args{
    int i;                  // Index for Analysis Thread comaprison array loop to begin from
    int n;                  // Index for Analysis Thread comaprison array loop to end at
    jsd_arr *jsd_arr;       // Pointer to the Array of Comparison results
} ana_args;

//Optional Arguements Variables
unsigned int d = 1,
             f = 1,
             a = 1;

char *s = NULL;

//Queues variables - Bounded queue for file and Unbounded queue for file directory
B_Queue f_Q;
UB_Queue d_Q;

WR wr;      // Word Repository Variable

//Checks for the suffix of the file. If suffix is same then returns 1 else return 0.
int suffix(char *s, char *filename)
{
    int len_s = strlen(s), 
        len_filename = strlen(filename);
    if (len_s > len_filename) {
        return 0;
    }
    else {
        for (int i = 0; i < len_s; i++)
        {
            if (s[len_s - i - 1] != filename[len_filename - i - 1])
            {
                return 0;
            }
        }
    }

    return 1;
}

//ana_TH : Gets the argument from main thread.
//          The arguement tells the thread where in the array (jsd_arr[]) to start from and where to end.
//          It then compares the two files from the array at the given index 
//          and computes the JSD in the comparison array (jsd_arr[]).
void *ana_TH(void *args){
    ana_args *arg = args;

    for(int j = arg -> i; j < arg -> n; j++){
        cmp_l(
            find(&wr, arg -> jsd_arr[j].file1),
            find(&wr, arg -> jsd_arr[j].file2),
            &arg -> jsd_arr[j]
        );
    }

    return NULL;
}

//File Thread :  Dequeues filename from the file queue and creates a linked list of each word.
//A file thread will finish once the queue is empty and all directory threads have finished.
void *file_TH(){

    sleep(1);   // Thread here sleeps for 1 sec

    char *filename;
    //Loops until the file is dequeued from the file queue.
    while(dequeue_F(&f_Q, &filename) == 0){
        if(readFile(filename, &wr) != 0){
            fprintf(stderr, "Error in Reading File: %s\n", filename);
            continue;
        }

        free(filename);     //frees the memory
        filename = NULL;

        sleep(1);
    }

    return NULL;

}

/*  * Directory Thread :Loops repeatedly to dequeue a directory name from 
                        the directory queue and reads through its directory listing.
    * Each regular file is added to the file queue with same suffix as the optional arguements(-s)
    * When the queue is empty and all the threads are waiting, the directory threads will exit.
*/                 
void *dir_TH(){

    sleep(1);   // Directory thread sleeps here for 1 sec.

    char *dirname;

    //Dequeues the directory queue
    while(dequeue_D(&d_Q, &dirname) == 0){
        DIR *dir = opendir(dirname);    //opens directory

        struct dirent *read_dir;

        //If directory is empty, it will return NULL with the error message
        if(dir == NULL){
            perror("Couldn't Open Directory");
            return NULL;
        }

        //If the directory name starts with "." continue reading.
        while( (read_dir = readdir(dir)) != NULL){
            if(read_dir -> d_name[0] == '.'){
                continue;
            }

            //Creates the string containing relative path of the file or directory. 
            char *dirinsert = (char*) malloc((strlen(dirname) + strlen(read_dir -> d_name) + 2) * sizeof(char));
            memset(dirinsert, 0, ((strlen(dirname) + strlen(read_dir -> d_name) + 2) * sizeof(char)));
            strcat(dirinsert, dirname);
            strcat(dirinsert, "/");
            strcat(dirinsert, read_dir -> d_name);

            struct stat dir_stat;
        
            if(stat(dirinsert, &dir_stat) != 0){
                perror("Failed to set Stat");
                return NULL;
            }

            //printf("[%d]: %s\n", dir_stat.st_mode, dirinsert);

            //If directory, enqueue to the directory queue
            if(S_ISDIR(dir_stat.st_mode)){
                enqueue_D(&d_Q, dirinsert);
            } 
            //If file, enqueue to file queue
            else if (S_ISREG(dir_stat.st_mode)){
                if(suffix(s, dirinsert)){
                    enqueue_F(&f_Q, dirinsert);
                };
            } else {
                continue;
            }

            free(dirinsert);    //Frees the memory
        }

        closedir(dir);  //Closes the directory

        free(dirname);  //Frees the memory of directory name
        dirname = NULL;

        sleep(1);

    }

    return NULL;
}

/*  * Collection Phase : Creates directory and file threads.
    * Puts the regular arguements into the directory or file queue. 
    * Waits for the thread to return.
*/
int collection_phase(int i, int argc, char *argv[]){

    pthread_t *d_th = (pthread_t *) malloc(d * sizeof(pthread_t));  
    pthread_t *f_th = (pthread_t *) malloc(f * sizeof(pthread_t));  

    // Creates the directory threads
    for(int k = 0; k < d; k++){
        pthread_create(&d_th[k], NULL, dir_TH, NULL);
    }

    //Creates the file threads.
    for(int k = 0; k < f; k++){
        pthread_create(&f_th[k], NULL, file_TH, NULL);
    }

    // Checks for the regular arguments
    for(int j = i; j < argc; j++) {

        // If there is an optional arguement, ignore it.
        if(argv[j][0] == '-'){            
            continue;
        }

        struct stat input_stat;

        if(stat(argv[j], &input_stat) != 0){
            perror("Failed to set Stat");
            return -1;
        }

        //Checks whether the arguements provided are file or directory and alots them to respective queues.
        if(S_ISDIR(input_stat.st_mode)){
            enqueue_D(&d_Q, argv[j]);
        } else if(S_ISREG(input_stat.st_mode)){
            enqueue_F(&f_Q, argv[j]);
        }
    }

    //Joins all directory threads
    for(int k = 0; k < d; k++){
        pthread_join(d_th[k], NULL);
    }

    close_F(&f_Q);      //Closes the file queue

    //Joins all file threads
    for(int k = 0; k < f; k++){
        pthread_join(f_th[k], NULL);
    }

    free(d_th);     //Frees memory of Directory thread
    free(f_th);     //FRees memory of File thread

    return 0;

}

//Compares the combined words of the files from comparison array (jsd_arr[]) and outputs in desecnding order
int compare_jsd(const void *elem1, const void *elem2){
    if((((jsd_arr *)elem1) -> combineWord) > (((jsd_arr *)elem2) -> combineWord)){
        return -1;
    } else if ((((jsd_arr *)elem1) -> combineWord) < (((jsd_arr *)elem2) -> combineWord)) {
        return 1;
    } else {
        return 0;
    }
}

/*  
    * Analysis Phase : Creates the analysis threads, 
    * determines the number of comparisons and make an array of comparaison result struct
    * each comparison result struct gets two files and their combined word
    * creates the range buckets for thread to use for dividing the work
    * after the thread are finished computing the jsd for all comparison sort them and then print
*/
int analysis_phase(){

    //If there are less than two files than it will not be comapres and hence throw the error.
    if(wr.size < 2){
        fprintf(stderr, "Less than 2 files: Cannot Compare!");
        return -1;
    }

    // Creates the array of comparison results.
    size_t size_jsdarr = ((wr.size * (wr.size - 1)) / 2);
    jsd_arr *jsds = (jsd_arr *) malloc(size_jsdarr * sizeof(jsd_arr));

    int index_jsd = 0;

    //Assigns the files for comparison.
    for(int i = 0; i < wr.size; i++){
        for(int j = i + 1; j < wr.size; j++){
            jsds[index_jsd].file1 = wr.lists[i] -> file;
            jsds[index_jsd].file2 = wr.lists[j] -> file;
            jsds[index_jsd].combineWord = wr.lists[i] -> words + wr.lists[j] -> words;
            jsds[index_jsd].jsd = 0.0;
            index_jsd++;
        }
    }

    // Creates the array of arguments analysis struct to store the range buckets.
    ana_args *args = (ana_args *) malloc(a * sizeof(ana_args));

    int buc_s = size_jsdarr / a;// bucket side
    int buc_g = size_jsdarr % a;// which buckets have to be size + 1

    int index = 0;              // where should the thread's comparison array loop begin from

    for(int i = 0; i < a; i++){
        if(buc_g > 0){
            args[i].i = index;
            index = index + buc_s + 1;
            args[i].n = index;
            args[i].jsd_arr = jsds;
            buc_g--;
        }
        else if(buc_g == 0){
            args[i].i = index;
            index = index + buc_s;
            args[i].n = index;
            args[i].jsd_arr = jsds;
        }
        
    }

    pthread_t *a_th = (pthread_t *) malloc(a * sizeof(pthread_t));
    
    //Creates the analysis thread
    for(int i = 0; i < a; i++){
        pthread_create(&a_th[i], NULL, ana_TH, &args[i]);
    }

    // Joins the analysis threads.
    for(int i = 0; i < a; i++){
        pthread_join(a_th[i], NULL);
    }

    //Sorts the comparison results
    qsort(jsds, size_jsdarr, sizeof(jsd_arr), compare_jsd);

    // Prints the results.
    for(int i = 0; i < size_jsdarr; i++){
        printf("%lf - %s\t%s\n", jsds[i].jsd, jsds[i].file1, jsds[i].file2);
    }

    //DEBUG
    if(CHECK){
        printf("---------------------------------- Details ---------------------------------\n");
        printf("Total Files: %ld\n", wr.size);
        printf("Total Comparisons: %ld\n", size_jsdarr);
        printf("# of Directory Thread: %d\n", d);
        printf("# of File Thread: %d\n", f);
        printf("# of Analysis Thread: %d\n", a);
        printf("# of File Suffix: '%s'\n", s);
    }

    free(jsds);     // Frees the comparison result array
    free(a_th);     // Frees the analysis thread
    free(args);     // Frees the analysis thread arguments.

    return 0;

}

//Diver Method
int main(int argc, char *argv[]){

    //If not enough arguements are provided, return -1.
    if(argc < 2){
        fprintf(stderr, "Not enough arguments: [Optionals] [Files | Directory]\n");
        return -1;
    }

    init_F(&f_Q, 100);  //Intialize the bounded queue upto size 100 for files.
    init_D(&d_Q);       //Intializes the unbounded queue for directory.

    init_WR(&wr);       //Intializes the word repository

    int i = 0;
    // Processes the optional arguments. 
    // If wrong optional arguements found, exits with failure else it continues.
    for(i = 1; i < argc && argv[i][0] == '-' ; i++){

        char c = argv[i][1];
        if(strlen(argv[i]) == 2 && c != 's'){
            if(c != 'd' || c != 'f' || c != 'a'){
                fprintf(stderr, "Invalid Arguement\n");
                return -1;
            } else {
                continue;
            }
        }
        
        if(c == 'd'){
            char d_n[strlen(argv[i] + 2) + 1];
            d_n[0] = 0;
            strcpy(d_n, argv[i] + 2);
            d = atoi(d_n);
            if(d == 0) {
                fprintf(stderr, "Invalid Optional Arugement -d\n");
                return -1;
            }
        }
        else if(c == 'f'){
            char f_n[strlen(argv[i] + 2) + 1];
            f_n[0] = 0;
            strcpy(f_n, argv[i] + 2);
            f = atoi(f_n);
            if(f == 0) {
                fprintf(stderr, "Invalid Optional Arugement -f\n");
                return -1;
            }
        }
        else if(c == 'a'){
            char a_n[strlen(argv[i]+ 2) + 1];
            a_n[0] = 0;
            strcpy(a_n, argv[i] + 2);
            a = atoi(a_n);
            if(a == 0) {
                fprintf(stderr, "Invalid Optional Arugement -a\n");
                return -1;
            }
        }
        else if(c == 's'){
            char *p = (char*) malloc((strlen(argv[i] + 2) + 1) * sizeof(char));
            if(p == NULL){
                perror("Couldn't Allocate Memory");
                return -1;
            }
            s = p;
            strcpy(s, argv[i] + 2);
        }
        else {
            fprintf(stderr, "Invalid Arguement!\n");
            return -1;
        }
    }

    // Set the Default -s Optional Argument
    if(s == NULL){
        s =  (char*) malloc((strlen(".txt") + 1) * sizeof(char));
        if (s == NULL){
            perror("Couldn't Allocate Memory");
            return -1;
        }
        s[0] = '\0';
        strcpy(s, ".txt");
        s[strlen(s)] = '\0';
    }

    // Sets the number of directory thread active.
    d_Q.active = d;
    d_Q.activeneed = d;

    //Do the Collection Phase. If collections phase returns errors, return -1
    if(collection_phase(i, argc, argv) != 0){
        fprintf(stderr, "Error during the Collection Phase\n");
        return -1;
    }

    //Do the Analysis Phase. If analysis phase returns errors, returns -1.
    if(analysis_phase() != 0){
        fprintf(stderr, "Error during the Collection Phase\n");
        return -1;
    }

    //Frees the memory
    free(s);        
    destroy_F(&f_Q);
    destroy_D(&d_Q);
    destroy_WR(&wr);

    return 0;
}