#include "input.h"

static void HandleInputBuffer();

static const int NUM_BUFFERS = 3;                            // 1

struct AQRecorderState {
    AudioStreamBasicDescription  mDataFormat;                   // 2
    AudioQueueRef                mQueue;                        // 3
    AudioQueueBufferRef          mBuffers[NUM_BUFFERS];      // 4
    AudioFileID                  mAudioFile;                    // 5
    UInt32                       bufferByteSize;                // 6
    SInt64                       mCurrentPacket;                // 7
    bool                         mIsRunning;                    // 8
};

static struct AQRecorderState S;
#define BUFFER_SIZE 64

bool init_input(void) {

#define PRINT_R do{\
	printf("%d: r = %d\n",__LINE__, r);\
}while(0)  

	AudioStreamBasicDescription *fmt = &S.mDataFormat;  

	fmt->mFormatID         = kAudioFormatLinearPCM;  
	fmt->mSampleRate       = 44100.0;  
	fmt->mChannelsPerFrame = 1;  
	fmt->mBitsPerChannel   = 32;  
	fmt->mBytesPerFrame    = fmt->mChannelsPerFrame * sizeof (float);  
	fmt->mFramesPerPacket  = 1;  
	fmt->mBytesPerPacket   = fmt->mBytesPerFrame * fmt->mFramesPerPacket;  
	fmt->mFormatFlags      = kAudioFormatFlagIsFloat | kAudioFormatFlagIsNonInterleaved;  

	OSStatus r = 0;  

	r = AudioQueueNewInput(&S.mDataFormat, HandleInputBuffer, &S, NULL, kCFRunLoopCommonModes, 0, &S.mQueue);  

	PRINT_R;  

	UInt32 dataFormatSize = sizeof (S.mDataFormat);  

	r = AudioQueueGetProperty (  
			S.mQueue,  
			kAudioConverterCurrentInputStreamDescription,  
			&S.mDataFormat,  
			&dataFormatSize  
			);  

	S.bufferByteSize = BUFFER_SIZE;      

	for (int i = 0; i < NUM_BUFFERS; ++i) {        
		r = AudioQueueAllocateBuffer(S.mQueue, S.bufferByteSize, &S.mBuffers[i]);  
		PRINT_R;  

		r = AudioQueueEnqueueBuffer(S.mQueue, S.mBuffers[i], 0, NULL);  
		PRINT_R;  
	}  

	S.mCurrentPacket = 0;        

	return true;

}

static float in_buffer[BUFFER_SIZE];

float *get_input_data() { return in_buffer; }

static void HandleInputBuffer (
                           void                                *aqData,
                           AudioQueueRef                       inAQ,
                           AudioQueueBufferRef                 inBuffer,
                           const AudioTimeStamp                *inStartTime,
                           UInt32                              inNumPackets,
                           const AudioStreamPacketDescription  *inPacketDesc
) {

    struct AQRecorderState *pAqData = (struct AQRecorderState *) aqData;

    if (inNumPackets == 0 && pAqData->mDataFormat.mBytesPerPacket != 0) {
        inNumPackets =
        inBuffer->mAudioDataByteSize / pAqData->mDataFormat.mBytesPerPacket;
    }
//    printf("%f\n", *(float*)inBuffer->mAudioData);
    if (pAqData->mIsRunning == 0)
        return;

    memcpy(in_buffer, inBuffer->mAudioData, BUFFER_SIZE*sizeof(float));

    AudioQueueEnqueueBuffer(pAqData->mQueue, inBuffer, 0, NULL);
}

void start_input() {
	OSStatus r;
	S.mIsRunning = true;        

	r = AudioQueueStart(S.mQueue, NULL);  
	PRINT_R;  

}

void stop_input() {

	OSStatus r;
	r = AudioQueueStop(S.mQueue, true);  
	S.mIsRunning = false;  

	PRINT_R;  

	r = AudioQueueDispose(S.mQueue, true);  

}

