#include "cache.h"
#include<QtMath>
#include <QDebug>

int power(int a, int b)//a^b
{
    if(b==0)
        return 1;
    return a*power(a, b-1);
}

Block::Block()
{
    lru=-1;
    dirtyBit=false;
    this->validBit=false;
    this->tag=-1;
}
int Block::getTag()
{
    return tag;
}
bool Block::isModified()
{
    return dirtyBit;
}
Set::Set(int noOfBlocks)  : noOfMisses(0), noOfHits(0)
{
    setClock=0;
    noOfHits=0;
    noOfMisses=0;
    blocks=new Block*[noOfBlocks];
    for(int i=0;i<noOfBlocks;i++)
    {
        blocks[i]=new Block();
    }
    this->noOfBlocks=noOfBlocks;
}
int Set::getHits()
{
    return noOfHits;
}
int Set::getMisses()
{
    return noOfMisses;
}
bool Set::checkHit(int tag)
{
    for(int i=0;i<noOfBlocks;i++){
        if(tag==blocks[i]->getTag()){
            noOfHits++;
            return true;
        }
    }
    noOfMisses++;
    return false;
}
bool Set::setBlock(int tag)
{
    int kickOutIndex=0;
    for(int i=0;i<noOfBlocks;i++)
    {
        if(!blocks[i]->getValidBit()){
            kickOutIndex=i;
            break;
        }
        if(blocks[i]->getLru()<blocks[kickOutIndex]->getLru()){
            kickOutIndex=i;
        }
    }
    if(blocks[kickOutIndex]->isModified())
    {
        //WB policy
        //Maybe Take No Of Writes
    }
    blocks[kickOutIndex]->setTag(tag);
    blocks[kickOutIndex]->setDirtyBit(false);
    blocks[kickOutIndex]->setLru(setClock);
    setClock++;
    return true;
}

Cache::Cache() : noOfHits(0), noOfMisses(0), missPenalty(100), storeLatency(0), loadLatency(0), valid(false)    //32 Bit ~ 4 Byte addressable machine - Assumption
{

}
Set* Cache::getSet(int i)
{
    return sets[i];
}
int Cache::getNoOfSets()
{
    return noOfSets;
}
int Cache::getHits()
{
    return noOfHits;
}
int Cache::getMisses()
{
    return noOfMisses;
}
int Cache::getMissPenalty()
{
    return missPenalty;
}
int Cache::getStoreLatency()
{
    return storeLatency;
}
int Cache::getLoadLatency()
{
    return loadLatency;
}
void Cache::setCache(int cacheSize, int blockSize, int associativity, int loadLatency, int storeLatency)
{
    int totalBlocks=(cacheSize*1024)/blockSize;     // cacheSize in KB and blockSize in Bytes
    noOfSets=totalBlocks/associativity;
    if(noOfSets>=1)
        valid=true;
    else                                            // if associativity is more than total number of blocks
    {
        valid=false;
        return;
    }

    bits_offset=(int)log2(blockSize);
    bits_index=(int)log2(noOfSets);
    bits_tag=32-bits_index-bits_offset;

    sets=new Set*[noOfSets];
    for(int i=0;i<noOfSets;i++)
    {
        sets[i]=new Set(associativity);
    }
    this->storeLatency=storeLatency;
    this->loadLatency=loadLatency;
}

bool Cache::storeInCache(int address)
{
    int andValue;
//    andValue=power(2, bits_offset)-1;//Not Required
//    int offset=address & andValue;//Not Required

    andValue=power(2, bits_index)-1;
    int index=(address>>bits_offset) & andValue;

    andValue=power(2, bits_tag)-1;
    int tag=(address>>(bits_index+bits_offset)) & andValue;

    if(index>=0 && index<noOfSets)
        return sets[index]->setBlock(tag);
    return false;
}
bool Cache::checkHit(int address)
{
    int andValue;
//    andValue=power(2, bits_offset)-1;//Not Required
//    int offset=address & andValue;//Not Required

    andValue=power(2, bits_index)-1;
    int index=(address>>bits_offset) & andValue;

    andValue=power(2, bits_tag)-1;
    int tag=(address>>(bits_index+bits_offset)) & andValue;

    if(index>=0 && index<noOfSets)
    {
        if(sets[index]->checkHit(tag))
        {
            noOfHits++;
            return true;
        }
    }
    noOfMisses++;
        return false;
}
