#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define QOI_IMPLEMENTATION
#include "../qoi.h"

int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size)
{
    if (Size > 3) {
        uint8_t* img_data = (uint8_t*) malloc(Size - 3);
        int _size;
        memcpy(img_data, Data+3, Size-3);
        void* res = qoi_encode(Data, 
                        &(qoi_desc){
                            .width = Data[0],
                            .height = Data[1],
                            .channels = Data[2],
                            .colorspace = QOI_SRGB},
                        &_size
            );
        if (res) {
            free(res);
        }
    }
    
    return 0;
}