#include <stdio.h>
#include <string.h>
#include <errno.h>
#include "time.h"
#include<stdlib.h>

#include "arguments.h"
#include "raw.h"
#include "hexdump.h"
#include "checksums.h"

// minimal frame length is defined by dest. MAC + source MAC + EtherType + CRC
#define MIN_FRAME_LENGTH (2 * ETH_ALEN + 16 + 4)

//struct framesList {
//    size_t frameLength;
//    struct framesList *nextFrame;
//    uint8_t *frameData;
//};

struct framesContainer {
    size_t size;
    uint16_t etherType;
    int broadcastFramesNum;
    int unicastFramesNum;
    int totalByteLength;
    struct framesContainer *nextFrameType;
};

struct framesContainer *framesContainer;

uint8_t mymac[ETH_ALEN];

struct timespec start, end;

void printBuffer(uint8_t buffer[], size_t length);

void addFrameToContainer(uint8_t buffer[], size_t length);

void setTransmissionType(struct framesContainer *frameType, uint8_t *buffer);

void analyseFrames();

void analyse(int fd, int frames) {
    unsigned int timeout = 10000;
    uint8_t recbuffer[1514];
    size_t ret;
    int framesReceived = 0;
    memcpy(&mymac, grnvs_get_hwaddr(fd), ETH_ALEN);

    
    fprintf(stdout, "I am ready!\n");
    clock_gettime(CLOCK_MONOTONIC, &start);

    while (framesReceived < frames) {
        ret = grnvs_read(fd, recbuffer, sizeof(recbuffer), &timeout);
        if (ret == 0) {
            fprintf(stderr, "Timed out, this means there was nothing to receive. Do you have a sender set up?\n");
            break;
        }

        framesReceived++;
        addFrameToContainer(recbuffer, ret);
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    analyseFrames();
}

void addFrameToContainer(uint8_t buffer[], size_t length) {
    // make sure a valid frame was given
    if (length < MIN_FRAME_LENGTH)
        return;

    // dest. MAC + source MAC to ignore
    int etherTypeOffset = ETH_ALEN + ETH_ALEN;
    // 16 bits = 2x8 bits: standard size for EtherType IEEE 802.
    uint16_t etherType = (buffer[etherTypeOffset] << 8) + buffer[etherTypeOffset + 1];

    // debugging
//    printf("EtherType dec: %i \n", etherType);    // print the EtherType of the frame in decimal
//    printf("EtherType hex: %04x \n", etherType);  // print the EtherType of the frame in hex with 4 digits


    struct framesContainer *currFrameType = framesContainer;
    struct framesContainer *prevFrameType = NULL;
    while (currFrameType != NULL) {
        // check if the found EtherType is already in the list of such frames
        if (currFrameType->etherType == etherType) {
            // debugging
//            printf("Case 01!\n");

            currFrameType->size++;
            currFrameType->totalByteLength += length;

            setTransmissionType(currFrameType, buffer);

            break;

        } else if (currFrameType->etherType < etherType) {
            // debugging
//            printf("Case 02!\n");

            // current frame type is smaller than the received one
            // -> check the next frame type, so the list remains sorted
            prevFrameType = currFrameType;
            currFrameType = currFrameType->nextFrameType;

        } else {
            // debugging
//            printf("Case 03!\n");

            // current frame type is greater than the received one and it's not zero (due to while condition)
            // -> initialize the new frame type and connect it to existing so that the list remains sorted
            struct framesContainer *newFrameType = malloc(sizeof(struct framesContainer));
            newFrameType->size = 1;
            newFrameType->etherType = etherType;
            newFrameType->totalByteLength = length;
            newFrameType->broadcastFramesNum = 0;
            newFrameType->unicastFramesNum = 0;

            setTransmissionType(newFrameType, buffer);

            prevFrameType->nextFrameType = newFrameType;
            newFrameType->nextFrameType = currFrameType;

            break;
        }
    }

    if (currFrameType == NULL) {
        // current frame type is greater than all other types found in the list
        // -> initialize the new frame type and connect it at to the prev. last one (if exists)
        currFrameType = malloc(sizeof(struct framesContainer));
        currFrameType->size = 1;
        currFrameType->etherType = etherType;
        currFrameType->totalByteLength = length;
        currFrameType->broadcastFramesNum = 0;
        currFrameType->unicastFramesNum = 0;
        currFrameType->nextFrameType = NULL;

        setTransmissionType(currFrameType, buffer);

//        printf("Starting the container!\n");
        if (prevFrameType != NULL)
            prevFrameType->nextFrameType = currFrameType;
        else
            framesContainer = currFrameType;
    }

}

void setTransmissionType(struct framesContainer *frameType, uint8_t *buffer) {
    // the transmission type is found in the first byte of dest. MAC
    // least significant bit of this byte corresponds to the type
    // 0 -> unicast | 1 -> broadcast
    uint8_t lastByteDestMAC = buffer[0];
    if ((lastByteDestMAC & 0b00000001) == 1)
        frameType->broadcastFramesNum++;
    else {
        // if dest. address is not broadcast, the unicast frame can still be not for me
        for (int i = 0; i < ETH_ALEN; i++)
            if (buffer[i] != mymac[i])
                return;
        frameType->unicastFramesNum++;
    }
}

void analyseFrames() {
    struct framesContainer *currFrameType = framesContainer;
    long totalBroadCastFrames = 0, totalUnicastFrames = 0, totalBytesReceived = 0;
    while (currFrameType != NULL) {
        printf("0x%04x: %zu frames, %i bytes\n", currFrameType->etherType, currFrameType->size,
               currFrameType->totalByteLength);
        totalBroadCastFrames += currFrameType->broadcastFramesNum;
        totalUnicastFrames += currFrameType->unicastFramesNum;
        totalBytesReceived += currFrameType->totalByteLength;
        currFrameType = currFrameType->nextFrameType;
    }
    printf("%li of them were for me\n", totalUnicastFrames);
    printf("%li of them were multicast\n", totalBroadCastFrames);

    // timing in seconds (converted from nanoseconds)
    double totalTime;
    totalTime = (end.tv_sec - start.tv_sec) * 1e9;
    totalTime = (totalTime + (end.tv_nsec - start.tv_nsec)) * 1e-9;

//    printf("Total Bytes received in %f seconds: %li\n", totalTime, totalBytesReceived);
    printf("Total rate %f Mbit/s | ", (totalBytesReceived * 8 / totalTime) / 1e6);
    printf("%f MiB/s | ", (totalBytesReceived / totalTime) / (1024 * 1024));
    printf("%f kB/s | ", (totalBytesReceived / totalTime) / 1e3);
    printf("%f Kibit/s\n", (totalBytesReceived * 8 / totalTime) / 1024);
}

// debugging purposes
void printBuffer(uint8_t recbuffer[], size_t length) {
    for (size_t i = 0; i < length; i++)
        printf("%02X ", recbuffer[i]);
    printf("\n");
}

int main(int argc, char **argv) {
    struct arguments args;
    int sock;

    setvbuf(stdout, NULL, _IOLBF, 0);

    if (parse_args(&args, argc, argv) < 0) {
        fprintf(stderr, "Failed to parse arguments, call with "
                        "--help for more information\n");
        return -1;
    }

    if ((sock = grnvs_open(args.interface, SOCK_RAW)) < 0) {
        fprintf(stderr, "grnvs_open() failed: %s\n", strerror(errno));
        return -1;
    }

    analyse(sock, args.frames);

    grnvs_close(sock);

    return 0;
}
