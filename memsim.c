#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
        int pageNo;
        int modified;
		int access_time; // added for lru algorithm
		int referenced; // added for clock algorithm
} page;
enum    repl { repl_random, fifo, lru, clock};
int     createMMU( int);
int     checkInMemory( int ) ;
int     allocateFrame( int ) ;
page    selectVictim( int, enum repl) ;
const   int pageoffset = 12;            /* Page size is fixed to 4 KB */
int     numFrames ;
page* pageTable; // global page table
int accessCounter = 0; // global variable for a counter to track access time
int clockHand = 0; // initialise the clock hand to 0

/* Creates the page table structure to record memory allocation */
int     createMMU (int frames)
{
		// check if page table is already created
		if (pageTable != NULL) {
			printf("MMU already created\n");
			return -1;
		}

		// allocate memory for the page table
		pageTable = (page*)malloc(frames * sizeof(page));

		if (pageTable == NULL) {
			printf("Failed to allocate memory for the MMU\n");
			return -1;
		}

		// initialise the page table
		for (int i = 0; i < frames; i++) {
			pageTable[i].pageNo = -1; // initialise page numbers to -1/empty frames
			pageTable[i].modified = 0; // initialise modified flag to 0
			pageTable[i].access_time = 0; // initialise frame access time to 0
			pageTable[i].referenced = 0; // initialise referenced bit to 0
		}

		// set global numFrames variable
		numFrames = frames;

        return 0;
}

/* Checks for residency: returns frame no or -1 if not found */
int     checkInMemory( int page_number)
{
        int     result = -1;

		for (int i = 0; i < numFrames; i++) {
			if (pageTable[i].pageNo == page_number) {
				// page is in memory, update its access time
				pageTable[i].referenced = 1;
				pageTable[i].access_time = accessCounter++;
				result = i;
				break; // no need to continue searching
			}
		}

        return result ;
}

/* allocate page to the next free frame and record where it put it */
int     allocateFrame( int page_number)
{

        // look for an empty frame in the page table
		for (int i = 0; i < numFrames; i++) {
			if(pageTable[i].pageNo == -1) {
				// found an empty frame, allocate the page to it
				pageTable[i].pageNo = page_number;
				pageTable[i].modified = 0; // initialise modified flag to 0;
				pageTable[i].access_time = accessCounter++; // set the access time for the next page
				pageTable[i].referenced = 1;
				return i; // return allocated frame number
			}
		}

    	return -1; // no free frames available, return -1 to indicate failure
}

/* Selects a victim for eviction/discard according to the replacement algorithm,  returns chosen frame_no  */
page    selectVictim(int page_number, enum repl  mode )
{
        page    victim;
		

		switch (mode) {
			case repl_random:
				// select random frame as victim
				int randomFrame = rand() % numFrames;

				// retrieve the page number and modified flag of randomly selected frame
				victim.pageNo = pageTable[randomFrame].pageNo;
				victim.modified = pageTable[randomFrame].modified;

				// update selected frame with the new page number
				pageTable[randomFrame].pageNo = page_number;
				pageTable[randomFrame].modified = 0; // reset modified flag for the new page
				break;
			case lru:
				// find the frame with the highest access time (least recently used)
				int lruFrame = 0;
				for (int i = 1; i < numFrames; i++) {
					if (pageTable[i].access_time < pageTable[lruFrame].access_time) {
						lruFrame = i;
					}
				}

				pageTable[lruFrame].access_time = accessCounter++; // increment the access time for the chosen frame

				// retrieve the page number and modified flag of the lru frame
				victim.pageNo = pageTable[lruFrame].pageNo;
				victim.modified = pageTable[lruFrame].modified;

				// update selected frame with the new page number
				pageTable[lruFrame].pageNo = page_number;
				pageTable[lruFrame].access_time = accessCounter++; // set the access time for the next page
				pageTable[lruFrame].modified = 0; // reset modified flag for the new page
				
				break;
			case clock:
				while (1) {
					// check if the current page at the clock hand is marked as "not referenced"
					if (pageTable[clockHand].referenced == 0) {
						// this page is the victim for replacement
						victim.pageNo = pageTable[clockHand].pageNo;
						victim.modified = pageTable[clockHand].modified;

						// update selected frame with the new page number
						pageTable[clockHand].pageNo = page_number;
						pageTable[clockHand].modified = 0; // reset modified flag for the new page 
						pageTable[clockHand].referenced = 1; // mark the page as referenced
						
						// move the clock hand to the next page in the circular list
						clockHand = (clockHand + 1) % numFrames;
						
						break; // exit while loop
					} else {
						// mark page as not referenced for the next round
						pageTable[clockHand].referenced = 0;
					}
					// move the clock hand to the next page in the circular list
					clockHand = (clockHand + 1) % numFrames;
				}
				break;
			default:
				victim.pageNo = 0;
				victim.modified = 0;
		}	

		return victim;
}

		
int main(int argc, char *argv[])
{
  
	char	*tracename;
	int	page_number,frame_no, done ;
	int	do_line, i;
	int	no_events, disk_writes, disk_reads;
	int     debugmode;
 	enum	repl  replace;
	int	allocated=0; 
	int	victim_page;
        unsigned address;
    	char 	rw;
	page	Pvictim;
	FILE	*trace;


        if (argc < 5) {
             printf( "Usage: ./memsim inputfile numberframes replacementmode debugmode \n");
             exit ( -1);
	}
	else {
        tracename = argv[1];	
	trace = fopen( tracename, "r");
	if (trace == NULL ) {
             printf( "Cannot open trace file %s \n", tracename);
             exit ( -1);
	}
	numFrames = atoi(argv[2]);
        if (numFrames < 1) {
            printf( "Frame number must be at least 1\n");
            exit ( -1);
        }
        if (strcmp(argv[3], "lru\0") == 0)
            replace = lru;
	    else if (strcmp(argv[3], "rand\0") == 0)
	     replace = repl_random;
	          else if (strcmp(argv[3], "clock\0") == 0)
                       replace = clock;		 
	               else if (strcmp(argv[3], "fifo\0") == 0)
                             replace = fifo;		 
        else 
	  {
             printf( "Replacement algorithm must be rand/fifo/lru/clock  \n");
             exit ( -1);
	  }

        if (strcmp(argv[4], "quiet\0") == 0)
            debugmode = 0;
	else if (strcmp(argv[4], "debug\0") == 0)
            debugmode = 1;
        else 
	  {
             printf( "Replacement algorithm must be quiet/debug  \n");
             exit ( -1);
	  }
	}
	
	done = createMMU (numFrames);
	if ( done == -1 ) {
		 printf( "Cannot create MMU" ) ;
		 exit(-1);
        }
	no_events = 0 ;
	disk_writes = 0 ;
	disk_reads = 0 ;

        do_line = fscanf(trace,"%x %c",&address,&rw);
	while ( do_line == 2)
	{
		page_number =  address >> pageoffset;
		frame_no = checkInMemory( page_number) ;    /* ask for physical address */

		if ( frame_no == -1 )
		{
		  disk_reads++ ;			/* Page fault, need to load it into memory */
		  if (debugmode) 
		      printf( "Page fault %8d \n", page_number) ;
		  if (allocated < numFrames)  			/* allocate it to an empty frame */
		   {
                     frame_no = allocateFrame(page_number);
		     allocated++;
                   }
                   else{
		      Pvictim = selectVictim(page_number, replace) ;   /* returns page number of the victim  */
		      frame_no = checkInMemory( page_number) ;    /* find out the frame the new page is in */
		   if (Pvictim.modified)           /* need to know victim page and modified  */
	 	      {
                      disk_writes++;			    
                      if (debugmode) printf( "Disk write %8d \n", Pvictim.pageNo) ;
		      }
		   else
                      if (debugmode) printf( "Discard    %8d \n", Pvictim.pageNo) ;
		   }
		}
		if ( rw == 'R'){
		    if (debugmode) printf( "reading    %8d \n", page_number) ;
		}
		else if ( rw == 'W'){
		    // mark page in page table as written - modified
			pageTable[frame_no].modified = 1;
		    if (debugmode) printf( "writting   %8d \n", page_number) ;
		}
		 else {
		      printf( "Badly formatted file. Error on line %d\n", no_events+1); 
		      exit (-1);
		}

		no_events++;
        	do_line = fscanf(trace,"%x %c",&address,&rw);
	}

	printf( "total memory frames:  %d\n", numFrames);
	printf( "events in trace:      %d\n", no_events);
	printf( "total disk reads:     %d\n", disk_reads);
	printf( "total disk writes:    %d\n", disk_writes);
	printf( "page fault rate:      %.4f\n", (float) disk_reads/no_events);
	
	// free allocated memory for page table
	free(pageTable);
	
	return 0;
}