/*
// Copyright (c) 2018 Intel Corporation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
*/

/*
// brief The entry point for the buffer queue management
*/

typedef enum _BuffStatus
{
    BUFFER_IVD =0, // INVALID
    BUFFER_IDLE=1,
    BUFFER_DEQ =2,
    BUFFER_ENQ =3,
    BUFFER_ACQ =4
}BufferStatus;


struct BufferItem
{
public:
    BufferItem(){};

    mfxFrameSurface1* mfxSurface;
    BufferStatus      mStatus;
    int               mEnqNo;
} ;


class BufferQueue
{
public:
    BufferQueue(int numOfMaxBuffer)
    {
        pthread_mutex_init(&m_mutex,NULL); 
        pBufferItem =  (BufferItem *)malloc(sizeof(BufferItem)*numOfMaxBuffer);
        memset(pBufferItem, 0, sizeof(BufferItem)*numOfMaxBuffer);
        sem_init(&newQueueBufferSem, 0, 1);
        m_nMaxNumOfSlots = numOfMaxBuffer;
        m_nSlots         =  0;
        lastEnqNo        = -1;
        latestEnqNo      = -1;
        m_nModelId       = 0; //by default it is decoder
    };

    int registerBuffer(mfxFrameSurface1* mfxSurface)
    {
        int i=0;  

        pthread_mutex_lock(&m_mutex);  
        //Check whether existing queue has such surfae or not.
        for(i=0;i<m_nMaxNumOfSlots;i++)
        { 
            if(pBufferItem[i].mfxSurface == mfxSurface)
                break;
        }  
        if(i == m_nMaxNumOfSlots)
        {
            //Not exist
            //printf("it is a new buffer =%d\r\n",m_nMaxNumOfSlots);
            for(i=0;i<m_nMaxNumOfSlots;i++)
            { 
                if(pBufferItem[i].mStatus == BUFFER_IVD){
                    pBufferItem[i].mStatus    = BUFFER_IDLE;
                    pBufferItem[i].mfxSurface = mfxSurface;
                    pBufferItem[i].mEnqNo     = -1;
                    m_nSlots++;
                    break;
                }
            }
        }
        else{
            //printf("already exist an existing buffer\r\n");
        }
        pthread_mutex_unlock(&m_mutex);
        return i;
    };
    
    int dequeueBuffer(int modelID)//bufferID
    {
        int i=0;  
        pthread_mutex_lock(&m_mutex);  
        //Check whether existing queue has such surfae or not.
        for(i=0;i<m_nSlots;i++)
        { 
            if(pBufferItem[i].mStatus == BUFFER_IDLE || (pBufferItem[i].mStatus == BUFFER_DEQ && (pBufferItem[i].mfxSurface)->Data.Locked ==0 )){
                pBufferItem[i].mStatus = BUFFER_DEQ;
                break;
            }
        }  
        pthread_mutex_unlock(&m_mutex);
        if(i == m_nSlots)
        { // NO avaiable idel buffer, try to waiting for it
            while(1)
            {
                pthread_mutex_lock(&m_mutex);  
                for(i=0;i<m_nSlots;i++)
                { 
                    if(pBufferItem[i].mStatus == BUFFER_IDLE || (pBufferItem[i].mStatus == BUFFER_DEQ && (pBufferItem[i].mfxSurface)->Data.Locked ==0 ) ){
                        pBufferItem[i].mStatus = BUFFER_DEQ;
                        break;

                    }
                }
                pthread_mutex_unlock(&m_mutex);
                if(i != m_nSlots){
                     break;
                }
                usleep(1000);
             }
        }
        // Wait a new frame ready
        return i;

    }
    int dequeueBuffer(mfxFrameSurface1* pmfxFrameSurface)//bufferID
    {
        int i=0;  
        pthread_mutex_lock(&m_mutex);  
        //Check whether existing queue has such surfae or not.
        for(i=0;i<m_nSlots;i++)
        {
            if( pBufferItem[i].mfxSurface ==pmfxFrameSurface   ){
                pBufferItem[i].mStatus = BUFFER_DEQ;
                 break;
            }
        }  
        pthread_mutex_unlock(&m_mutex);

        // Wait a new frame ready
        return i;

    };
    int enqueueBuffer(int bufferID)
    {
        int errcode = 0;
        pthread_mutex_lock(&m_mutex);  
        //Check whether existing queue has such surfae or not.
      
        if(pBufferItem[bufferID].mStatus == BUFFER_DEQ){
            pBufferItem[bufferID].mStatus = BUFFER_ENQ;
            latestEnqNo++;
            pBufferItem[bufferID].mEnqNo = latestEnqNo;
        }
        else{
             printf("INVALID status\r\n");
        }
        pthread_mutex_unlock(&m_mutex);
        sem_post(&newQueueBufferSem);
        return 0;//success

     };

    int enqueueBuffer(mfxFrameSurface1* pmfxFrameSurface, int modelID)
    {
        int errcode = 0;
        // Try to make sure buffer is registered.
        //std::cout<<getModelName(modelID)<<"enqueueBuffer"<<std::endl;
        registerBuffer(pmfxFrameSurface);
        pthread_mutex_lock(&m_mutex); 

        for(int i=0;i<m_nSlots;i++)
        { 
            if(pBufferItem[i].mfxSurface == pmfxFrameSurface){

                if(pBufferItem[i].mStatus == BUFFER_DEQ || 
                   pBufferItem[i].mStatus == BUFFER_IDLE){
                   pBufferItem[i].mStatus = BUFFER_ENQ;
                   latestEnqNo++;
                   pBufferItem[i].mEnqNo = latestEnqNo;
                }
                else{ 
                   break;
                }

             }
         }
        //Check whether existing queue has such surfae or not.
        pthread_mutex_unlock(&m_mutex);
        sem_post(&newQueueBufferSem);
        return 0;//success

    };

    // return the surfaceID
    int acquireBuffer(int modelID)
    {
        int i=0;  
        pthread_mutex_lock(&m_mutex);  
        //Check whether existing queue has such surfae or not.
        //std::cout<<getModelName(modelID)<<": acquireBuffer"<<std::endl;
        for(i=0;i<m_nSlots;i++)
        { 
            if(pBufferItem[i].mStatus == BUFFER_ENQ && pBufferItem[i].mEnqNo == (lastEnqNo+1))
            {
                pBufferItem[i].mStatus = BUFFER_ACQ;
                lastEnqNo ++;
                 
                break;
            }
        }  
        pthread_mutex_unlock(&m_mutex);


        if(i == m_nSlots )
        {
            sem_wait(&newQueueBufferSem);
           
            while(1){
            pthread_mutex_lock(&m_mutex);  
            //Check whether existing queue has such surfae or not.
            for(i=0;i<m_nSlots;i++)
            { 
                if(pBufferItem[i].mStatus == BUFFER_ENQ && pBufferItem[i].mEnqNo == (lastEnqNo+1))
                {
                    pBufferItem[i].mStatus = BUFFER_ACQ;
                    lastEnqNo ++;
                     break;
                }
            }  
            pthread_mutex_unlock(&m_mutex);
            if(i != m_nSlots)
                break;

            usleep(1000);
            }

        }

        return i;
    };
    // 
    int returnBuffer(int bufferID, int modelID)
    {

        int errcode = 0;
        pthread_mutex_lock(&m_mutex);  
        //Check whether existing queue has such surfae or not.
        if(pBufferItem[bufferID].mStatus == BUFFER_ACQ){
           pBufferItem[bufferID].mStatus  = BUFFER_IDLE;
           pBufferItem[bufferID].mEnqNo = -1;
          // pBufferItem[bufferID].mfxSurface->Data.Locked = 0;
        }
         else
          printf("Vpp: INVALID status\r\n");
        /*
         for(int i=0;i<m_nSlots;i++)
         { 
        printf("pBufferItem[%d].mStatus =%d\r\n", i, pBufferItem[i].mStatus);
         }  */
        pthread_mutex_unlock(&m_mutex);
        return 0;//success
    };

    BufferStatus  getBufferStatus(mfxFrameSurface1* mfxSurface)
    {

        BufferStatus bufferStatus = BUFFER_IVD;
        registerBuffer(mfxSurface);
        pthread_mutex_lock(&m_mutex);  
        //Check whether existing queue has such surfae or not.

        for(int i=0;i<m_nSlots;i++)
        { 
            if(pBufferItem[i].mfxSurface == mfxSurface)
            {
                bufferStatus =  pBufferItem[i].mStatus;
            }
        }  
        pthread_mutex_unlock(&m_mutex);
        //std::cout<<": getBufferStatus:"<<bufferStatus<<std::endl;

        return bufferStatus;
    };

    mfxFrameSurface1* getBuffer(int bufferID)
    {

        int errcode = 0;
        if(bufferID >= m_nSlots)
           return NULL;

        return pBufferItem[bufferID].mfxSurface;
    };

    int m_nSlots;
    int m_nMaxNumOfSlots;
    int lastEnqNo;
    int latestEnqNo;
    BufferItem *pBufferItem;
    int m_nModelId;
    sem_t newQueueBufferSem;
    pthread_mutex_t m_mutex ;

};

