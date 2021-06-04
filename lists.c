//Including all the libraries required. 
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

// ************************** Bounded Queue **************************

size_t Qsize;//Size of the queue.

//Intializes the queue and the variables
int init_F(B_Queue *Q, size_t size) {
    Q -> data = (char**) malloc(size * sizeof(char*));
    Q -> size = 0;
    Qsize = size;
    Q -> open = 1;

    pthread_mutex_init(&Q->lock, NULL);
	pthread_cond_init(&Q->read_ready, NULL);
	pthread_cond_init(&Q->write_ready, NULL);

    return 0;
}

/*  * Add Function : adds the given data to the queue.
    * First, it locks the thread containg the data.
    * If the queue is open and Queue does not have space to write the thread waits.
    * If queue is not open its unlocks the thread and exits with failure
    * If none of the conditions apply we add the data to the queue and unlock the thread after.
*/
int enqueue_F(B_Queue *Q, char *data){
    pthread_mutex_lock(&Q->lock);
	
	while (Q->size == Qsize && Q->open) {
		pthread_cond_wait(&Q->write_ready, &Q->lock);
	}

	if (!Q->open) {
		pthread_mutex_unlock(&Q->lock);
		return -1;
	}
	
	unsigned i = Q->head + Q->size; 
	if (i >= Qsize) i -= Qsize;   //sets the index to be added  
	
    //adds the data at an given index.
	Q->data[i] = (char *) malloc((strlen(data) + 1) * sizeof(char)); 

    //If data is not allocated then return with EXIT_FAILURE
    if(Q->data[i] == NULL){
        perror("Couldn't allocate Memory");
        return -1;
    }

    //Switch the ownership of data to queue.
    memset(Q->data[i], 0, (strlen(data) + 1) * sizeof(char));
    strcpy(Q->data[i], data);
	++Q->size;
	
	pthread_cond_signal(&Q->read_ready);    // waits for the signal to be dequeue.
	
	pthread_mutex_unlock(&Q->lock);     // Unlocks the thread.

    return 0;
}

/*  * Dequeue: Removes the data from the queue.
    * If the queue is open and queue size is empty to read the thread waits.
    * If the queue is empty and is not open, unlock the thread and return with EXIT_FAILURE.
    * Free the data from the head as its queue data strucutre which follows FIFO
*/
int dequeue_F(B_Queue *Q, char **data)
{
	pthread_mutex_lock(&Q->lock); // Locks the thread
	
	while (Q->size == 0 && Q->open) {
		pthread_cond_wait(&Q->read_ready, &Q->lock);
	}
	if (Q->size == 0) {
		pthread_mutex_unlock(&Q->lock);
		return -1;
	}
	
	--Q->size; //decerements the size.

    //Switches the ownership from node data to data parameter
    *data = (char *) malloc((strlen(Q -> data[Q -> head]) + 1) * sizeof(char));
    memset(*data, 0, (strlen(Q -> data[Q -> head]) + 1) * sizeof(char));
    strcpy(*data, Q -> data[Q -> head]);

    //Frees the data from the head
    free(Q -> data[Q -> head]);
    Q -> data[Q -> head] = NULL;
	++Q->head;  //increments the thread.
	if (Q->head == Qsize) Q->head = 0; //If the last element is removed, sets head to 0
	
	pthread_cond_signal(&Q->write_ready);
	
	pthread_mutex_unlock(&Q->lock); //Unlocks the thread.
	
	return 0;
}

/*  * Closes the queue. 
    * Signals all the waiting threads.
*/
int close_F(B_Queue *Q)
{
	pthread_mutex_lock(&Q->lock);   //Locks the thread
	Q->open = 0;   
	pthread_cond_broadcast(&Q->read_ready);     //Signals all the waiting thread to read(dequeue)
	pthread_cond_broadcast(&Q->write_ready);    //Signals all the waiting thread to write(enqueue)
	pthread_mutex_unlock(&Q->lock);	    //Unlocks the thread.

	return 0;
}

//  Print : prints all the strings/Data from the queue.
void print_F(B_Queue *Q){
    for(int i = Q -> head, j = 0; j < Q->size; i++, j++){
        if (i >= Qsize) i -= Qsize;
        printf("%s\n", Q -> data[i]); 
    }
}

//  Destroy : Frees the memory of the queue
void destroy_F(B_Queue *Q){

    for(int i = Q -> head, j = 0; j < Q->size; i++, j++){
        if (i >= Qsize) i -= Qsize;
        free(Q -> data[i]); 
    }

    free(Q -> data);    //frees the data
    Q -> head = 0;      //sets head to 0

    //Destroys the lock and threads
    pthread_mutex_destroy(&Q->lock);
	pthread_cond_destroy(&Q->read_ready);
	pthread_cond_destroy(&Q->write_ready);
}

// ************************* Unbounded Queue *************************

//Intializes the queue and intializes the variables
int init_D(UB_Queue *Q){
    Q -> head = NULL;
    Q -> rear = NULL;
    Q -> size = 0;

    pthread_mutex_init(&Q->lock, NULL);
	pthread_cond_init(&Q->read_ready, NULL);

    return 0;
}

/*  * Enqueue : Adds the data to unbounded Queue.
    * Intialise the node and set the memory of the data to node.
    * If head of the queue is NULL, then node is assigned to the head
    * If head of the queue is not NULL, then check for the end of the queue and add the data at that point.
*/
int enqueue_D(UB_Queue *Q, char *data){

//Locks the thread
    pthread_mutex_lock(&Q -> lock);

    q_node *node = (q_node*) malloc(sizeof(q_node));    //Intializes the node

    node -> data = (char*) malloc((strlen(data) + 1) * sizeof(char));

    //If node is NULL or data is NUll then return with EXIT_FAILURE
    if(node == NULL || node -> data == NULL){
        perror("Couldn't Allocate Memory");
        return -1;
    }

    //Switches the ownership of data to queue.
    memset(node -> data, 0, (strlen(data) + 1) * sizeof(char));
    strcpy(node -> data, data);

    node -> next = NULL;

    //Adds the node to the queue.
    if(Q -> head == NULL){
        Q -> head = node;
    } else {

        q_node *current = Q -> head;
        while(current -> next != NULL){
            current = current -> next;
        }

        current -> next = node;
    }

    Q -> rear = node;

    ++Q -> size;    // Increment the size of the queue after adding the data to the queue

    pthread_cond_signal(&Q->read_ready);
    pthread_mutex_unlock(&Q -> lock);   // Unlock the thread.

    return 0;
}

/*  * Dequeue : Removes the data from the queue.
    * Removes the head of the queue by allocating the memory of the data to temp pointer
    * Once removed frees the memory of the temp node and decrements the queue size.
*/
int dequeue_D(UB_Queue *Q, char **data) {
    pthread_mutex_lock(&Q -> lock); //Locks the thread

    //If queue is empty, decrement the active threads
    //If the number of active threads is <= 0, signal all the 
    //threads waiting to dequeue, unlock the threads and exit.
    if(Q -> size == 0){
        Q->active--;
        if(Q->active <= 0){
            pthread_cond_broadcast(&Q->read_ready);
            pthread_mutex_unlock(&Q->lock);
            return -1;
        }
        
        //Let the thread wait until active threads are > 0 and queue is empty
        while(Q -> size == 0 && Q->active > 0){
            pthread_cond_wait(&Q->read_ready, &Q->lock);
        }

        //If queue is empty release all the threads and exit
        if(Q -> size == 0){
            pthread_mutex_unlock(&Q->lock);
            return -1;
        }

        if(Q->active != Q->activeneed){
            ++Q->active;    // Increments the active threads
        }
    }

    q_node *temp = Q -> head;
    Q -> head = temp -> next;

    //Switches the ownership of node data to data parameter.
    *data = (char *) malloc((strlen(temp -> data) + 1) * sizeof(char));
    memset(*data, 0, (strlen(temp -> data) + 1) * sizeof(char));
    strcpy(*data, temp -> data);

    free(temp->data);   //Frees the memory of temp data
    free(temp);         //Frees the memory of temp node

    --Q -> size;        //Decrements the queue size.

    pthread_mutex_unlock(&Q -> lock);   // Unlocks the threads.

    return 0;
}

//Print : Prints all the data in the queue.
void print_D(UB_Queue *Q){

    q_node *current = Q -> head;

    while(current != NULL){
        printf("%s\n", current -> data);
        current = current -> next;
    }

}

//Destory : Frees the memory from the queue
void destroy_D(UB_Queue *Q){

    q_node *current;    // Intialize the node

    //Removes all the elements from the queue.
    while(Q -> head != NULL){
        current = Q -> head;
        Q -> head = Q -> head -> next;
        free(current -> data);
        free(current);
    }

    pthread_mutex_destroy(&Q->lock);    //Destroys the lock
	pthread_cond_destroy(&Q->read_ready);   // Destroys the threads waiting to dequeue

}

// *************************** Linked List ***************************

// Intializes the linked list variables.
void initLL(l_list *list){
    list -> head = NULL;
    list -> size = 0;
    list -> words = 0;
    
}

 //add : Adds the word to the list in aplhabetical order
int add(l_list *list, char *word) {

    //Intilizes the list node
    l_list_node *node = (l_list_node *) malloc(sizeof(l_list_node));
    
    //Switches ownership of the word to node data.
    node -> word = malloc((strlen(word) + 1) * sizeof(char));
    memset(node -> word, 0, (strlen(word) + 1) * sizeof(char));
    strcpy(node -> word, word);
    node -> count = 1;  // Number of times the words appeares in the file
    node -> mean = 0.0; // Frequency of the word
    node -> next = NULL;    //Sets the end of the node as NULL

    //Adds the words in lexilogical order
    //If the list is empty, add the word in alphabetical order
    if(list -> head == NULL || 
        strcmp(node -> word, list -> head -> word) < 0)
    {
        node -> next = list -> head;
        list -> head = node;

    } else {

        //If the list is not empty then, compare the words and add to the list in the lexilogical order.
        l_list_node *current = list -> head;
        
        while(current -> next != NULL && 
                strcmp(node -> word, current -> next -> word) >= 0)
        {
            current = current->next;
        }
        node -> next = current -> next;
        current -> next = node;
    }

    list->size++;   // Increment the list size
    list->words++;  // Increment the words.
    return 0;
}

// Check : checks if the same word came in the list. If the same word comes increment the count.
int check(l_list *list, char *c){
    l_list_node *current = list -> head;    // Intialize the list node current to list's head

    //check for the same word while list if not empty.
    //If encoutered the same word, increment the count of that word 
    while(current != NULL){
        if(strcmp(current -> word, c) == 0) {
            current -> count++;
            list->words++;
            return 0;
        }
        current = current->next;
    }
    
    //if not encountered the same word, add the word to the list used add function
    add(list, c);

    return 0;
}

// Destory : Frees the memory of the linked list
void destroy_list(l_list *list){
    free(list -> file);
    l_list_node *current;

    while(list -> head != NULL){
        current = list -> head;
        list -> head = list -> head -> next;
        free(current -> word);
        free(current);
    }

}

//meanCal : Calulates the mean frequency of the each word in the list
void meanCal(l_list *list){
    l_list_node *current = list -> head;

    while(current != NULL){
        current -> mean = current -> count / (double) list -> words;
        current = current -> next;
    }
}

//Print : prints all the words present in the list with its count and mean frequency.
void printList(l_list *list){
    l_list_node *current = list -> head;

    while(current != NULL){
        printf("%s \t[%d] \t[%.4f]\n", current -> word, current ->count, current -> mean);
        current = current->next;
    }
}

// ************************* Word Repository *************************

//Intializes the word repository variables.
void init_WR(WR *repo){
    repo -> lists = NULL;
    repo -> size = 0;

    pthread_mutex_init(&repo -> lock, NULL);
}

// AddLists : Stores all the lists into the Word Respository
int add_lists(WR *repo, l_list *list){
    //Locks the thread
    pthread_mutex_lock(&repo -> lock);

    //If the respository is NULL then allocate the memory to store the list.
    if(repo -> lists == NULL){
        repo -> lists = (l_list **) malloc(sizeof(l_list*));

        if(repo -> lists == NULL){
            perror("couldn't allocate Memory");
            return -1;
        }
    } else {

        // If the lists is not null then rellocate the memory to store another list
        l_list **p = (l_list **) realloc(repo -> lists, (repo->size + 1) * sizeof(l_list*));
        if(p == NULL){
            perror("couldn't allocate Memory");
            return -1;
        }

        //Add the list to the respository list.
        repo -> lists = p;
    }

    //Switches the ownership of the list parameter to Word Repository.
    repo -> lists[repo -> size] = (l_list*) malloc(sizeof(l_list));

    memcpy(repo -> lists[repo -> size], list, sizeof(l_list));

    free(list);     //Frees the memory of the list parameter

    ++repo -> size; // Increment the repsoitory list size.

    pthread_mutex_unlock(&repo -> lock);    // Unlock the threads

    return 0;
}

//Find : finds the lists with the matching File Name from the Respository Lists
// And returns the corresponding lists.
l_list *find(WR *repo, char *name){
    for(int i = 0; i < repo->size; i++){
        if(strcmp(repo -> lists[i] -> file, name) == 0){
            return repo -> lists[i];
        }
    }

    return NULL;
}

//Print : Prints out the all the lists present in repository list.
void printWR(WR *repo){
    for(int i = 0; i < repo -> size; i++){
        printf("\n List %d\n\n", (i+1));
        printf("%s\t [%d]\n", repo -> lists[i] -> file, repo -> lists[i] -> words);
        printList(repo -> lists[i]);
    }
}

//Destory : Frees the memory from Word Repository
void destroy_WR(WR *repo){
    for(int i = 0; i < repo -> size; i++){
        destroy_list(repo -> lists[i]);
        free(repo->lists[i]);
    }

    free(repo -> lists);        //Frees the respository list array

    pthread_mutex_destroy(&repo -> lock);
}

// ****************************** Word *******************************

//CreateWord : creates the word by character
int createWord(word *w, char c){
    if(strlen(w -> data) + 1 == w -> size){
        char *p = realloc(w -> data, (w -> size + 1) * sizeof(char));
        if(p == NULL){
            perror("Problem Reallocating");
            return -1;
        }
        w -> size++;    // increments the word size.
        w -> data = p;  
        w -> data[w -> size - 1] = 0;
    }
    w -> data[strlen(w -> data)] = c; 

    return 0;
}

/*  * ReadFile : reads the file
    * Reads each and every character from the file and builds the word
    * Also converts the characters into lowercase and ignores punctuation.
    * Calculates the mean frequency of every word in the list created.
    * And adds the list to the word Repository.
*/
int readFile(char *file, WR *wr){

    //Intializes the word.
    word w; 
    w.data = (char *) malloc(2 * sizeof(char));
    memset(w.data, 0, 2);
    w.size = 2;

    int fd = open(file, O_RDONLY),
        isWord = 0;

    // If file cannot be opened, return -1.    
    if(fd == 0){
        perror("Could not open files");
        return -1;
    }

    char c;

    //Allocate the memory for the list
    l_list *list = (l_list*) malloc(sizeof(l_list));

    //Intilize the list
    initLL(list);
    list->file = malloc((strlen(file) + 1) * sizeof(char));
    memset(list->file, 0, (strlen(file) + 1) * sizeof(char));
    strcpy(list->file, file);

    //Reads each and every character from the file until the we reach the end of the file.
    while(read(fd, &c, sizeof(char)) != 0){

        //If there is a space and there was a word before the space then use check() to check the word
        // in the list.
        if(isspace(c) != 0){
            if(isWord){
                check(list, w.data);
                free(w.data);   //free the memory of word for next word.
                w.data = NULL;
                w.data = (char *) malloc(2 * sizeof(char));
                memset(w.data, 0, 2);
                w.size = 2;
                isWord = 0;
            }
        } 

        // If there is a punctuation then it ignores and continue reading the file
        else if(ispunct(c)){
            //If '-' is encounterd and it is between the word, then it's a word.
            if(c == '-' && isWord == 1){
                c = tolower(c);
                isWord = 1;
                createWord(&w, c);
            } else {
                continue;
            }
        }
        //If its just a regualar character, convert into lowercase.
        else {
            c = tolower(c);
            isWord = 1;
            createWord(&w, c);
        }
    }

    //If the word data is not null and there is a word then use check()
    if(w.data != NULL && isWord != 0){
        check(list, w.data);
        free(w.data);
        w.data = NULL;
    }

    //Calculates the mean frequency of every word in the lists.
    meanCal(list);

    //Frees the memory of word.
    free(w.data);

    // Adds the list to the Word Repository
    add_lists(wr, list);

    close(fd);  // Closes the file

    return 0;
}

//cmp_l : Compares two given lists and puts the calculated JSD into a comparison result array(jsd_arr[]).
void cmp_l(l_list* list1, l_list* list2, jsd_arr *jsd_a){

    //Intialize the list1 head to curr1 node
    l_list_node *cur1 = list1->head;

    //Intialize the list2 head to curr2 node
    l_list_node *cur2 = list2->head;

    double list1_kld = 0.0,
           list2_kld = 0.0;

    while(cur1 != NULL || cur2 != NULL){
        double mean_f = 0.0;
        //If list1 is empty then calulate the mean frequency and KLD of list2
        if(cur1 == NULL){
            mean_f = (cur2 -> mean) / 2;

            list2_kld += cur2 -> mean * log2(cur2 -> mean / mean_f);

            cur2 = cur2 -> next;
        }
        //If list2 is empty then calacuate the mean frequency and KLD of list1
        else if(cur2 == NULL){
            mean_f = (cur1 -> mean) / 2;

            list1_kld += cur1 -> mean * log2(cur1 -> mean / mean_f);

            cur1 = cur1 -> next;
        }

        //If list1 and list2 are not empty then compare the words from list1 and list2
        else {

            int cmp = strcmp(cur1 -> word, cur2 -> word);
            
            //If the compare value is same then calculate the mean frequency of both lists and KLD for both lists
            if(cmp == 0){
                mean_f = (cur1 -> mean + cur2 -> mean) / 2;

                list1_kld += cur1 -> mean * log2(cur1 -> mean / mean_f);
                list2_kld += cur2 -> mean * log2(cur2 -> mean / mean_f);

                // Increment both the lists simuntenously. 
                cur1 = cur1 -> next;
                cur2 = cur2 -> next;
            }

            //If the compare value of two lists words is < 0 then calculate the mean frequency and KLD of list1.  
            else if(cmp < 0){
                mean_f = (cur1 -> mean) / 2;
                list1_kld += cur1 -> mean * log2(cur1 -> mean / mean_f);
                cur1 = cur1 -> next;    // Increments list1
            } else {
                // If not then calculate the calulate the mean frequency and KLD of list2
                mean_f = (cur2 -> mean) / 2;
                list2_kld += cur2 -> mean * log2(cur2 -> mean / mean_f);
                cur2 = cur2 -> next;    // Increments list2
            }
        }

    }

    jsd_a -> jsd = sqrt((list1_kld / 2) + (list2_kld / 2));// Calculates the JSD

}