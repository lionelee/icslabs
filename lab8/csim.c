/* Name: Li Xinyu
 * Student ID: 515030910292 
 */

#include "cachelab.h"
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>

#define ADDRSZ 64
#define GET_POW(exp) (((lld)1)<<exp)
#define GET_TAG(addr) (addr>>(b+s))
#define GET_SET(addr) ((addr>>b)&(0x7fffffffffffffff>>(ADDRSZ-1-s)))

typedef unsigned long long int mem_addr;
typedef unsigned long long int lld;

struct LINE{
	int valid;
	lld tag;
	long time;
};

static int v=0, s=0, E=0, b=0;
static int hits=0, misses=0, evicts=0;
static struct LINE** cache;

static void printUsage() //print usage info
{
printf(
"./csim: Missing required command line argument\n\
Usage: ./csim [-hv] -s <num> -E <num> -b <num> -t <file>\n\
Options:\n\
  -h         Print this help message.\n\
  -v         Optional verbose flag.\n\
  -s <num>   Number of set index bits.\n\
  -E <num>   Number of lines per set.\n\
  -b <num>   Number of block offset bits.\n\
  -t <file>  Trace file.\n\n\
Examples:\n\
  linux> ./csim -s 4 -E 1 -b 4 -t traces/yi.trace\n\
  linux> ./csim -v -s 8 -E 2 -b 4 -t traces/yi.trace\n");
}

static void cache_init() //initialize cache space
{
	lld S = GET_POW(s);
	cache = malloc(sizeof(struct LINE*)*S); //LINE* cache[S]
	for(int i=0; i<S; ++i){
		cache[i] = malloc(sizeof(struct LINE)*E);//LINE cache[S][E]
		for(int j=0; j<E; ++j){ //initialize each line
			cache[i][j].valid=0;
			cache[i][j].tag=0;
			cache[i][j].time=0;
		}	
	}
}

static void cache_clear() //clear cache space alloced
{
	lld S = GET_POW(s);
	for(int i=0; i<S; ++i){
		free(cache[i]);
	}	
	free(cache);
}

static void update_time(int set, int line) //update line's time in set
{
	long max_time=cache[set][0].time;
	for(int j=1; j<E; ++j){ //find max time in set
		if(cache[set][j].time>max_time)
			max_time = cache[set][j].time;	
	}
	cache[set][line].time = max_time + 1; //make line's time be the max(latest)
}

static char* access(char op, mem_addr addr)
{
	lld tag = GET_TAG(addr);
	int set = GET_SET(addr);
	int flag = 0; // flag to record empty line in set
	int line; //record the line index
	for(int j=0; j<E; ++j){
		if(cache[set][j].valid==1){
			if(cache[set][j].tag==tag){//hit
				update_time(set,j);
				++hits;		
				if(op=='M'){ ++hits; return "hit hit";}
				else return "hit";
			}
		}
		else{
			if(!flag){ flag=1; line=j; }
		};
	}
	++misses; //miss and 
	if(flag){//either don't need to evict
		cache[set][line].valid = 1;
		cache[set][line].tag = tag;
		update_time(set, line);
		if(op=='M'){ ++hits; return "miss hit";}
		else return "miss";
	}
	else{ //or need to evict
		long min_time = cache[set][0].time;
		line = 0;
		for(int j = 1; j < E; ++j){ //find the line with mininum time(least-recent) 
			if(cache[set][j].time < min_time){
				min_time = 	cache[set][j].time;
				line = j;
			}
		}
		++evicts;
		cache[set][line].tag = GET_TAG(addr);
		update_time(set, line);
		if(op=='M'){++hits; return "miss eviction hit";}
		else return "miss eviction";
	}
}

int main(int argc, char** argv)
{
	char *file_name = NULL;
	int opt;
	while((opt=getopt(argc,argv,"hvs:E:b:t:")) != -1){
        switch(opt){
		case 'v':
            v = 1;
            break;
        case 's':
            s = atoi(optarg);
            break;
        case 'E':
            E = atoi(optarg);
            break;
        case 'b':
            b = atoi(optarg);
            break;
        case 't':
            file_name = optarg;
            break;
        default:
            printUsage();
            exit(0);
        }
    }
	if(s==0 || E==0 || b==0 || file_name==NULL){
		printUsage();
        exit(1);
	}
	FILE* file = NULL;
	if((file=fopen(file_name, "r"))==NULL){
		printf("ERROR: failed to open file.\n");
		exit(1);	
	}
	cache_init();
	char op, *status; //operation to access and status to output
	mem_addr addr;
	int size;
	while(fscanf(file, " %c %llx,%d\n", &op, &addr, &size)!=EOF){
		if(op!='I'){
			status = access(op, addr);
			if(v)printf("%c %llx,%d %s\n",op,addr,size,status);
		}
	}
	cache_clear();
    printSummary(hits, misses, evicts);
	fclose(file);
    return 0;
}
