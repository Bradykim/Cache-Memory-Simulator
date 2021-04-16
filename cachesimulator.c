#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
//Total memory
char mem[(int) pow(2,16)];

//Struct with elements of each cacheblock
struct cacheblock
{
    int validbit =0;
    int dirtybit=0;
    int tag;
    int LRUcounter=0;
    char blockdata[64];
};

FILE *fi;
int main(int argc, char* argv[])
{
    char filecontent[80];
    fi = fopen(argv[1],"r");
    int totalLRU = 0;

    //cache configuration
    int kb = atoi(argv[2]);
    int ways = atoi(argv[3]);
    char write[3];
    strcpy(write,argv[4]);
    int blocksize = atoi(argv[5]);

    //Creating cache which is a 2d array of cacheblocks
    int numsets = ((kb* 1024)/blocksize)/ways;
    struct cacheblock cache[numsets][ways];

    //Address information parsed from file
    char los[80];
    int address;
    int size;
    char data[blocksize];

    //Address bit length information (Address is always 16 bits)
    int index = (int)(log(numsets)/log(2));
    int offsetbits = (int)(log(blocksize)/log(2));
    int tagbits = 16-index-offsetbits;

    //Strings that will be compared to
    char store[]= "store";
    char load[]= "load";
    char writethrough[] = "wt";
    char writeback[] ="wb";

    //Scan the file and do methods
    while(fscanf(fi, "%s", los)!=EOF)
    {









        //If the command is a store
        if(strcmp(los,store)==0)
        {
            //Scan the address and the size of the access in bytes
            fscanf(fi, "%x %i", &address,&size);

            //Address numeric values information
            int blockoffset = address%blocksize;
            int blockindex = (address/blocksize)%numsets;
            int blocktag = address/(numsets*blocksize);

            //Scan the data information which is in hex.
            for (int i = 0; i < size; i++) 
            {
	            fscanf(fi, "%02hhx", data + i);
            }

            //Print load or store and the address.
            printf("%s %04x ",los, address);

            
            bool miss = true;
            bool fullset = true;

            //Iterate through the number of ways at the given index.
            for(int i =0; i<ways; i++)
            {
                //Check if it is a hit
                if(cache[blockindex][i].tag == blocktag && cache[blockindex][i].validbit == 1)
                {
                    //Update the LRU counter of that cacheblock
                    cache[blockindex][i].LRUcounter= totalLRU;

                   

                    printf("%s ","hit");
                    miss= false;
                    //If it is a write through
                    if(strcmp(write,writethrough)==0)
                    {
                        for(int y= 0; y< size; y++)
                        {
                            mem[y+address] = data[y];
                            cache[blockindex][i].blockdata[y+blockoffset] = data[y];
                        }
                    }

                    
                    //If it is a write back.
                    else if(strcmp(write,writeback)==0)
                    {
                        //Write data into cache, update the lru bit and the dirty bit
                        cache[blockindex][i].LRUcounter= totalLRU;
                        cache[blockindex][i].dirtybit = 1;
                        for(int y= 0; y< size; y++)
                        {
                            cache[blockindex][i].blockdata[y+blockoffset] = data[y];
                        }
                    }
                    

                    printf("\n");
                    break;
                }
                else if(cache[blockindex][i].validbit != 1)
                {
                    cache[blockindex][i].LRUcounter= totalLRU;
                    fullset = false;
                    break;
                }
                
            }

            //It is a miss
            if(miss)
            {
                //Print miss
                printf("%s ", "miss");
                //Copy the information from the store data into memory
                if(strcmp(write,writethrough)==0)
                {

                    for(int y= 0; y< size; y++)
                    {
                        mem[y+address] = data[y];
                        
                    }
                }
                

                else if(strcmp(write,writeback)==0)
                {
                    if(!fullset)
                    {
                        
                        //Loop through the values at the set
                        for(int i =0; i<ways; i++)
                        {
                            //Find which cacheblock is empty
                            if(cache[blockindex][i].validbit != 1)
                            {
                                //Update the block index data, valid bit and LRU counter
                                cache[blockindex][i].validbit = 1;
                                cache[blockindex][i].LRUcounter= totalLRU;
                                cache[blockindex][i].tag = blocktag;
                                cache[blockindex][i].dirtybit=1;

                                //move block from memory into cache
                                for(int y =0; y<blocksize;y++)
                                {
                                    cache[blockindex][i].blockdata[y] = mem[address-blockoffset+y];
                                }

                                //Write information of data into the cache
                                for(int y= 0; y< size; y++)
                                {
                                    cache[blockindex][i].blockdata[y+blockoffset] = data[y];
                                    
                                }

                                
                            }
                        }
                    }
                    else if(fullset)
                    {
                       
                        //Integer that keeps track of lowest LRU
                        int leastlru = cache[blockindex][0].LRUcounter;

                        //Integer that keeps track of which cache block has the lowest LRU
                        int indexevict=0;

                        for(int i =1; i<ways; i++)
                        {
                            //If the cache block has a lower LRU that the least lru
                            if(cache[blockindex][i].LRUcounter < leastlru)
                            {
                                //Set the least lru to the new lru
                                leastlru = cache[blockindex][i].LRUcounter;

                                //Set the cacheblock with the lowest LRU to the new cacheblock.
                                indexevict = i;
                            }

                        }

                        //If the dirty bit equals 1
                        if(cache[blockindex][indexevict].dirtybit==1)
                        {
                            //printf("dirtybit");
                            //Put into address in memory found by bit shifter.
                            int tagmem = cache[blockindex][indexevict].tag << (index+offsetbits);
                            int indexmem = blockindex << offsetbits;

                            int memaddress = tagmem+indexmem;
                            for(int z =0; z<blocksize;z++)
                            {
                                mem[memaddress+z] = cache[blockindex][indexevict].blockdata[z];
                            }

                            
                        }
                        //move block from memory into cache
                        for(int y =0; y<blocksize;y++)
                        {
                            cache[blockindex][indexevict].blockdata[y] = mem[y+ (address - address % blocksize)];
                        }
                        //Update the block index data, valid bit and LRU counter
                        cache[blockindex][indexevict].validbit = 1;
                        cache[blockindex][indexevict].LRUcounter= totalLRU;
                        cache[blockindex][indexevict].tag = blocktag;
                        cache[blockindex][indexevict].dirtybit=1;
                        //Copy the information from the memory into the cache block data
                        for(int y= 0; y< size; y++)
                        {
                            cache[blockindex][indexevict].blockdata[y+blockoffset] = data[y];
                            
                        }
                    }
                }
                

                printf("\n");
            }
            
            

        }







        //If the command is a load
        else if(strcmp(los,load)==0)
        {
            //Scan the address and the size of the access in bytes
            fscanf(fi, "%x %i", &address,&size);

            //Address numeric values information
            int blockoffset = address%blocksize;
            int blockindex = (address/blocksize)%numsets;
            int blocktag = address/(numsets*blocksize);


            //Print the information
            printf("%s %04x ",los, address);
            
            //Variable for whether it is a hit.
            bool miss = true;
            bool fullset = true;

            //Iterate through the number of ways at the given index.
            for(int i =0; i<ways; i++)
            {
                //Check if it is a hit
            
                if(cache[blockindex][i].tag == blocktag && cache[blockindex][i].validbit == 1)
                {
                    //Update the LRU counter of that cacheblock
                    cache[blockindex][i].LRUcounter= totalLRU;

                    

                    miss = false;

                    //Print hit
                    printf("%s ", "hit");

                    //Print data
                    for(int j = blockoffset; j < blockoffset+size; j++)
                    {
                
                        printf("%02hhx", cache[blockindex][i].blockdata[j]);
                    }
                    printf("\n");
                    break;
                }

                //Checks to see if there is an empty cach block.
                if(cache[blockindex][i].validbit != 1)
                {
                    fullset = false;
                }
                
            }

            //It is a miss
            if(miss == true)
            {
                //Print miss
                printf("%s ", "miss");


                //If it is not a fullset.
                if(!fullset)
                {
                    for(int i =0; i<ways; i++)
                    {
                        //Find which cacheblock is empty
                        if(cache[blockindex][i].validbit != 1)
                        {
    
                            //Update the block index data, valid bit and LRU counter
                            cache[blockindex][i].validbit = 1;
                            cache[blockindex][i].LRUcounter= totalLRU;
                            cache[blockindex][i].tag = blocktag;

                            //Copy the information from the memory into the cache block data
                            //Getting value of data from the memory and storing in info.

                            for(int y= 0; y< blocksize; y++)
                            {
                                cache[blockindex][i].blockdata[y] = mem[y+ (address - address % blocksize)];
                            }
                            for(int j = blockoffset; j < blockoffset+size; j++)
                            {
                                printf("%02hhx", cache[blockindex][i].blockdata[j]);
                            }
                            printf("\n");
                            break;
                        }
                    }
                    
                    
            
                }

                //If it is a fullset
                else if (fullset)
                {
                    //Integer that keeps track of lowest LRU
                    int leastlru = cache[blockindex][0].LRUcounter;

                    //Integer that keeps track of which cache block has the lowest LRU
                    int indexevict=0;

                    for(int i =1; i<ways; i++)
                    {
                        //If the cache block has a lower LRU that the least lru
                        if(cache[blockindex][i].LRUcounter < leastlru)
                        {
                            //Set the least lru to the new lru
                            leastlru = cache[blockindex][i].LRUcounter;

                            //Set the cacheblock with the lowest LRU to the new cacheblock.
                            indexevict = i;
                        }

                        

                    }
                    
                    
                    if(strcmp(write,writeback)==0)
                    {
                        //If the dirty bit equals 1
                        if(cache[blockindex][indexevict].dirtybit==1)
                        {
                            //Put into address in memory found by bit shifter.
                            int tagmem = cache[blockindex][indexevict].tag << (index+offsetbits);
                            int indexmem = blockindex << offsetbits;

                            int memaddress = tagmem+indexmem;
                            for(int z =0; z<blocksize;z++)
                            {
                                mem[memaddress+z] = cache[blockindex][indexevict].blockdata[z];
                            }


                            //Set the dirty bit to 0
                            cache[blockindex][indexevict].dirtybit=0;
                            
                        }
                    }
                    
                    //Update the block index data, valid bit and LRU counter
                    cache[blockindex][indexevict].validbit = 1;
                    cache[blockindex][indexevict].LRUcounter= totalLRU;
                    cache[blockindex][indexevict].tag = blocktag;
                    //Copy the information from the memory into the cache block data
                    for(int y= 0; y< blocksize; y++)
                    {
                        cache[blockindex][indexevict].blockdata[y] = mem[y+(address - address % blocksize)];
                    }
                
                    for(int j = blockoffset; j < blockoffset+size; j++)
                    {
                        printf("%02hhx", cache[blockindex][indexevict].blockdata[j]);
                    }
                    printf("\n");
                    
                }
              
            }
            
        }
         //Increment the totalLRU
        totalLRU++;
        
    }

    fclose(fi);
    return(0);
}