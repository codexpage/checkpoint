#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h> //mmap
#include <ucontext.h>
#include <string.h>

//global name for myckpt
char checkpoint[90];

ucontext_t context;



struct MemoryRegion
{
  unsigned long int startAddr; //or void *?
  unsigned long int endAddr;
  int isReadable;
  int isWriteable;
  int isExecutable;
} mem[100];


void restore_memory();
void unmapStack();
void restoreMemBlock();
//void restoreContext();
void restoreContext(FILE* file);

int main(int argc,char *argv[])
{
	if(argc!=2){
		printf("Please pass ONE image name.\n");
		return 0;
	}

	strcpy(checkpoint,argv[1]);
	size_t stacksize = 0x1000;
	void * stack_start = (void *)0x5300000;
	void * stack_pointer;
	//map anon block to the memory as new stack
	stack_pointer = mmap(stack_start,stacksize,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_GROWSDOWN | MAP_ANONYMOUS | MAP_FIXED,
		-1,
		0);
	if(stack_pointer == MAP_FAILED){
		perror("map stack failed");
		return 1;
	}
	stack_pointer = memset(stack_pointer,'\0',stacksize);//wrap data
	stack_pointer = stack_pointer + stacksize;
	// printf("sp:%p\n",stack_pointer);

	asm volatile ("mov %0,%%rsp;" : : "g" (stack_pointer) : "memory");//rsp

	restore_memory();


	return 0;





}


void restore_memory(){
	// printf("done\n");
	// register int sp asm ("rsp");
	// printf("%llx\n", sp);
	// void* p = NULL;
  // 	printf("addr: %p\n", (void*)&p);
  //   void* p2 = NULL;
  //   	printf("addr: %p\n", (void*)&p2);
  //     int p3 = 0;
  //       printf("addr: %p\n", (void*)&p3);

	unmapStack();
  restoreMemBlock();

	return;
}


void unmapStack(){
	FILE* file;
	file = fopen("/proc/self/maps","r");
	char line[500];
	long unsigned int stackstart,stackend;
	char perms[100];

	while(fgets(line,sizeof(line),file)){
		if(strstr(line,"stack")){
			// printf("%s\n",line);
			sscanf(line,"%lx-%lx %s%*[^\n]",&stackstart,&stackend,perms);
		}
	}
	size_t len = stackend -stackstart;
	// printf("%lx\n", (long unsigned int)len);
	if(munmap((void *)stackstart,(len-(3 * sysconf(_SC_PAGE_SIZE))))==-1){
		perror("stack unmapped error.");
	}
	fclose(file);
  // printf("done\n");

	return;
}

void restoreMemBlock() {
  	struct MemoryRegion mem;
    FILE* file;
    file = fopen("myckpt","r");
    int num=0;
    fread(&num,sizeof(int),1,file);
    // printf("num:%d\n",num);
    int i = 0;
    for (i = 0; i < num; i++) {
        if(fread(&mem,sizeof(struct MemoryRegion),1,file)!=1){
        	perror("block header read error.");
        }
        size_t len = mem.endAddr - mem.startAddr;
        //unmap
        if(munmap((void*)mem.startAddr,len)==-1){
        	// printf("start:%lx-len:%lx-end:%lx\n",mem.startAddr,len,mem.endAddr );
          perror("memory block unmapped error.");
        }

        //map mem block
        int prot = PROT_WRITE;
        if(mem.isReadable){
          prot = prot|PROT_READ;
        }
        if(mem.isExecutable){
          prot = prot|PROT_EXEC;
        }
        int flag = MAP_PRIVATE|MAP_ANONYMOUS | MAP_FIXED;

        if(mmap((void*)mem.startAddr,len,prot,flag,-1,0)==MAP_FAILED){
            perror("mmap empty mem block error");
        }

        if(fread((void*)mem.startAddr,len,1,file)!=1){//one elemment
          perror("read block to mem error");
        }
    
        // printf("%d:%lx-%lx\n",i+1,mem.startAddr,mem.endAddr);
    }
    printf("mem block all restored\n");

    // restoreContext(file);

    //restore context
    if(fread(&context,sizeof(ucontext_t),1,file)!=1){
		perror("read context error");
	}
	fclose(file);

	// printf("done1\n");
	if(setcontext(&context)==-1){
		// printf("done3\n");
		perror("setcontext error");
		return ;
	}
	// printf("done2\n");


    return;
}

void restoreContext(FILE* file){
	
	if(fread(&context,sizeof(ucontext_t),1,file)!=1){
		perror("read context error");
	}
	fclose(file);

	// printf("done1\n");
	if(setcontext(&context)==-1){
		// printf("done3\n");
		perror("setcontext error");
		return ;
	}
	// printf("done2\n");


  return;
}
