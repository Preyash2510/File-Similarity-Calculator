/*  * Header file for the Bounded Queue, Unbounded Queue,LinkedList, Word Repositary, word
     and a function used to compare the results(JSD, KLD)

    * Contains all the method definations and variables used for the excetution of each Data Structures.

*/

#ifndef LISTS_H
#define LISTS_H
#endif

// ************* Bounded Queue *************

typedef struct queue_f {
    char **data;
    size_t size;
    unsigned int head;
    int open;
    pthread_mutex_t lock;
    pthread_cond_t read_ready;
	pthread_cond_t write_ready;
} B_Queue;

int init_F(B_Queue *Q, size_t size);
int enqueue_F(B_Queue *Q, char *data);
int dequeue_F(B_Queue *Q, char **data);
int close_F(B_Queue *Q);
void print_F(B_Queue *Q);
void destroy_F(B_Queue *Q);

// ************ Unbounded Queue ************

typedef struct Q_node {
    char *data;
    struct Q_node *next;
} q_node;

typedef struct queue_d {
    q_node *head;
    size_t size;
    q_node *rear;
    unsigned int active;
    unsigned int activeneed;
    pthread_mutex_t lock;
    pthread_cond_t read_ready;
} UB_Queue;

int init_D(UB_Queue *Q);
int enqueue_D(UB_Queue *Q, char *data);
int dequeue_D(UB_Queue *Q, char **data);
void print_D(UB_Queue *Q);
void destroy_D(UB_Queue *Q);

// ************** Linked List **************

typedef struct L_node {
    char* word;
    int count;
    double mean;
    struct L_node *next;
} l_list_node;

typedef struct l{
    char *file;
    l_list_node *head;
    size_t size;
    int words;
} l_list;

void initLL(l_list *list);
int add(l_list *list, char *word);
int check(l_list *list, char *c);
void meanCal(l_list *list);
void destroy_list(l_list *list);
void printList(l_list *list);

// ************ Word Repository ************

typedef struct wordRepo {
    l_list **lists;
    size_t size;
    pthread_mutex_t lock;
} WR;

void init_WR(WR *repo);
int add_lists(WR *repo, l_list *list);
l_list* find(WR *repo, char *name);
void printWR(WR *repo);
void destroy_WR(WR *repo);

// ***************** Word ******************

typedef struct w {
    char *data;
    size_t size;
} word;

int createWord(word *w, char c);

// ************ Useful Function ************

// Comparison results
typedef struct jsd_struct{
    char *file1;
    char *file2;
    double jsd;
    size_t combineWord;
} jsd_arr;

int readFile(char *file, WR *wr);
void cmp_l(l_list *list1, l_list *list2, jsd_arr *jsd_a);