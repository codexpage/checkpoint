#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h> //mmap
#include <ucontext.h>
#include <string.h>


ucontext_t context;

struct MemoryRegion
{
  unsigned long int startAddr; //or void *?
  unsigned long int endAddr;
  int isReadable;
  int isWriteable;
  int isExecutable;
} mem[100];

void sig_handler(int);
void readMaps();
void readContext();

__attribute__ ((constructor))
void myconstructor(){
	// printf("hooked\n");
	signal(12,sig_handler);
}

// int main(){
// 	signal(12,sig_handler);

// 	while(1)
// 	{
// 		printf("sleep for 1s... %d\n",getpid());
// 		sleep(1);
// 	}

// 	return 0;
// }

// int main(int argc, char const *argv[])
// {
// 	readContext();
// 	readMaps();
// 	return 0;
// }

void sig_handler(int signum){
	printf("catch signal: %d\n", signum );
	
	int pidnum=getpid(); 

	if(getcontext(&context) != 0){
		printf("get context error.\n");
		return;
	}
	printf("checkpoint point \n");
	if(pidnum!=getpid()){
		printf("this is a new proc \n");
		return;
	}

	readMaps();
	return;
}

void readContext(){
	if(getcontext(&context) != 0){
		printf("get context error.\n");
		return;
	}
}

void readMaps(){
	//struct MemoryRegion mem={(int*)0xd1,(int*)0xd2,1,1,1};

	int pid = getpid();
	int c;
	FILE *file;
	char str[20];
	sprintf(str,"/proc/%d/maps",pid);

	//print map
	// char str2[20];
	// sprintf(str2,"cat %s",str);
	// system(str2);

	file = fopen(str, "r");
	// if (file) {
	//     while ((c = getc(file)) != EOF)
	//         putchar(c);
	//     fclose(file);	
	// }

	unsigned long int start;
	unsigned long int end;
	char perms[200];
	int num = 0;
	if(file){
		//while(fscanf(file,"%lx-%lx %s%*[^\n]",&start,&end,perms)!=EOF){
		char line[500];
		while(fgets(line,sizeof(line),file)){
			//skip vvar
			// printf("line:%s\n",line);
			if(strstr(line,"vvar")||strstr(line,"vdso")||strstr(line,"vsyscall")){
				// printf("skipped\n");
				continue;
			}
			sscanf(line,"%lx-%lx %s%*[^\n]",&start,&end,perms);
			// printf("%lx,%lx,%s\n",start,end,perms);
			if(perms[0]=='-'){
				// printf("upper one cannot read.\n");//seg fault
				continue;
			}
			mem[num].startAddr = start;
			mem[num].endAddr = end;
			mem[num].isReadable = (perms[0]=='r')? 1 : 0;
			mem[num].isWriteable = (perms[1]=='w')? 1 : 0;
			mem[num].isExecutable = (perms[2]=='x')? 1 : 0;

			// printf("%d: %lx-%lx,%d,%d,%d\n",num+1, mem[num].startAddr,mem[num].endAddr,mem[num].isReadable,mem[num].isWriteable,mem[num].isExecutable);
			num++;
		}

	}

	//num = num -3;
	FILE *checkpoint;
	checkpoint = fopen("myckpt", "w");
	//fprintf(checkpoint, "%d\n", num);//1. total number of sections

	fwrite(&num,sizeof(int),1,checkpoint);//1. total number of sections
	int i=0;
	for(i=0; i<num; i++){
		if(fwrite(&mem[i],sizeof(struct MemoryRegion),1,checkpoint)!=1)//2. the MemoryRegion header
		{
			printf("%d block\n",i+1);
			perror("write header error");
		}
		size_t length = mem[i].endAddr - mem[i].startAddr;
		if(fwrite((void *)mem[i].startAddr,length,1,checkpoint)!=1)//3. peice of memory data
		{
			printf("%d block\n",i+1);
			perror("write mem block error");
		}
	}



	if(fwrite(&context, sizeof(ucontext_t),1,checkpoint)!=1)//4. context
	{
		perror("write context error");
	}
	fclose(checkpoint);

	return;
		
}



