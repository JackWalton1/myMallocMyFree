#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

#define PAGESIZE 4096
int heapsize = 0;
void *startofheap = NULL;

typedef struct chunkhead { 
    unsigned int size; 
    unsigned int info; 
    unsigned char *next,*prev; 
}chunkhead; 

typedef unsigned char *byte;
void initializeFreeChunkhead();
unsigned char *mymalloc(unsigned int size); 
void myfree(unsigned char *address); 
void analyze();
void split();
void merge();
chunkhead* find_best_fit(unsigned int size);
chunkhead* get_last_chunk();
int heap_has_freespace();

chunkhead *freeChunkhead;

//need to implement: splitting chunk size when mallocing small and there is a big free chunk
int main(){
    
//PROF TESTS
    byte*b[100]; 
    clock_t start_t, end_t;
    start_t = clock();  
    for(int i=0;i<100;i++){b[i]= mymalloc(1000); }
    for(int i=0;i<90;i++) {myfree(b[i]); }
    myfree(b[95]); 
    b[95] = mymalloc(1000); 
    for(int i=90;i<100;i++){myfree(b[i]);}
    end_t = clock(); 
    fprintf(stderr, "\nduration(seconds): %f\n", (double)(end_t - start_t)/CLOCKS_PER_SEC); 
    
    
    byte* a[100]; 
    analyze();//50% points 
    for(int i=0;i<100;i++){a[i]= mymalloc(1000); } 
    for(int i=0;i<90;i++){myfree(a[i]);}
    analyze(); //50% of points if this is correct 
    myfree(a[95]); 
    a[95] = mymalloc(1000); 
    analyze();//25% points, this new chunk should fill the smaller free one 
    //(best fit) 
    for(int i=90;i<100;i++){
        myfree(a[i]); }
    analyze();// 25% should be an empty heap now with the start address from the program start 
    /*
MY TESTS
    unsigned char *a, *b, *c;
    int i;
    a = mymalloc(4073);
    for(i=0 ; i<4071; i++){
        a[i] = 'a';
    }
    a[4071] = '\0';
    fprintf(stderr, "%s\n", a);
    b = mymalloc(1000);
    c = mymalloc(1000);
    analyze();
    myfree(b);
    myfree(c);
    myfree(a);
    c = mymalloc(1000);
    a = mymalloc(1000); // test for split?
    analyze();
    */
    
    return 0;
}

void initialize(){
    //find program break
    freeChunkhead = (chunkhead *)sbrk(0);
    startofheap = (void*)sbrk(0);
    if(startofheap==(void*)-1){
        fprintf(stderr, "Heap failed to initialize\n");
        return;
    }
    //allocate enough space for chunkhead at last program break
    void* new_pb = (void*)sbrk(sizeof(chunkhead));
    freeChunkhead->info = 0; //means its free space 
    freeChunkhead->next = freeChunkhead->prev = 0; 
    freeChunkhead->size = 0;
    return;
}

int heap_has_freespace(){
    int has_freespace = 0;
    chunkhead *prev = (chunkhead*)freeChunkhead->prev;
    while(prev !=NULL && !has_freespace){
        if(prev->info == 0){
            has_freespace = 1;
        }else{
            prev = (chunkhead*)prev->prev;
        }
    }
    return has_freespace;

}

chunkhead* find_best_fit(unsigned int size){
    chunkhead* f_ch = (chunkhead*)startofheap;
    chunkhead* chosen = NULL;
    while(f_ch != freeChunkhead){
        if(f_ch->info ==0 && f_ch->size >= size){
            if(chosen == NULL){
                chosen = f_ch;
            }
            else if ((f_ch->size >= size) && f_ch->size <chosen->size){
                chosen = f_ch;
            }
        }
        f_ch = (chunkhead*)f_ch->next;
    }
    return chosen;
}

unsigned char* mymalloc(unsigned int size){
    if(size<=0){return NULL;}
    chunkhead *currentCh;
    int pages_req = (int)(((size+sizeof(chunkhead)-1)/(PAGESIZE)) +1);
    if(!(heapsize)){
        initialize();
    }
    int byte_size = PAGESIZE*pages_req;
    int has_freespace = heap_has_freespace();
    if(has_freespace){
        if(find_best_fit(size+sizeof(chunkhead))!=NULL){
            currentCh = find_best_fit(size+sizeof(chunkhead));
            currentCh->info =1;
            unsigned char* result = (unsigned char*)currentCh+sizeof(chunkhead);
            return result;
        }
        // find free space, determine if space is big enough (or too big!)
    }

    heapsize+=byte_size;
    currentCh = freeChunkhead;
    currentCh->size = byte_size;
    currentCh->prev = freeChunkhead->prev;
    if(currentCh->size >= size){
        void* new_pb;
        currentCh->info = 1;
        unsigned char* result = (unsigned char*)currentCh+sizeof(chunkhead);
        (void *)sbrk(byte_size-sizeof(chunkhead));
        new_pb = sbrk(0);
        if(new_pb == (void*)-1){
            fprintf(stderr, "Failed to move program break\n");
            return NULL;
        }
        currentCh->next = (unsigned char *)new_pb;
        freeChunkhead = (chunkhead*)new_pb;
        (void*)sbrk(sizeof(chunkhead));
        void* whereWeAt = (void*)sbrk(0);
        freeChunkhead->size, freeChunkhead->next, freeChunkhead->info = 0;
        freeChunkhead->prev = (unsigned char *)currentCh;
    
        return result;
    }
}
void merge_free_chunks(){
    chunkhead* f_ch = (chunkhead*)startofheap;
    //chunkhead* l_ch = get_last_chunk();
    while((chunkhead*)f_ch->next != freeChunkhead){
        if(f_ch->info ==0 && ((chunkhead*)(f_ch->next))->info ==0){
            f_ch->size = f_ch->size + ((chunkhead*)(f_ch->next))->size;
            f_ch->next = ((chunkhead*)(f_ch->next))->next;
            ((chunkhead*)f_ch->next)->prev = (unsigned char*)f_ch;
            //remove next chunkhead, increase size of free chunkhead
        }else{
            f_ch = (chunkhead*)f_ch->next;
        }
    }
    return;
}

int is_free_heap(){
    int free_heap = 1;
    chunkhead* f_ch = (chunkhead*)startofheap;
    while(f_ch != freeChunkhead){
        if(f_ch->info == 1){
            free_heap = 0;
        }
        f_ch = (chunkhead*)f_ch->next;
    }
    return free_heap;

}

void myfree(unsigned char *address){
    if(!heapsize){ //I have a global void *startofheap = NULL; 
        fprintf(stderr, "nothing to free.\n");
        return;   
    }
    chunkhead* ch = (chunkhead*)startofheap; 
    while(address-sizeof(chunkhead)!=(unsigned char*)ch){
        ch = (chunkhead*)ch->next;
    }
    ch->info = 0;
    merge_free_chunks();
    if(is_free_heap()){
        brk(startofheap);
        void* new_pb = (void*)sbrk(0);
        freeChunkhead = (chunkhead*)new_pb;
        sbrk(sizeof(chunkhead));
        new_pb = sbrk(0);
        heapsize = sizeof(chunkhead);
    }
    return;
}

chunkhead* get_last_chunk() //you can change it when you aim for performance 
    {   
    if(!heapsize) //I have a global void *startofheap = NULL; 
        return NULL;   
    chunkhead* ch = (chunkhead*)startofheap; 
    for (; ch->next; ch = (chunkhead*)ch->next); 
    return (chunkhead*)ch->prev; 
 } 

void analyze() { 
    fprintf(stderr, "\n--------------------------------------------------------------\n"); 
    if(!startofheap) 
        { 
        fprintf(stderr, "no heap"); 
        return; 
    } 
    chunkhead* ch = (chunkhead*)startofheap; 
    for (int no=0; ch; ch = (chunkhead*)ch->next,no++) { 
        fprintf(stderr, "%d | current addr: %x |", no, ch); 
        fprintf(stderr, "size: %d | ", ch->size); 
        fprintf(stderr, "info: %d | ", ch->info); 
        fprintf(stderr, "next: %x | ", ch->next); 
        fprintf(stderr, "prev: %x", ch->prev); 
        fprintf(stderr, "      \n"); 
    } 
    fprintf(stderr, "program break on address: %x\n",sbrk(0)); 
    return;
}