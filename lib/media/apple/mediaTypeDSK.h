#ifndef _MEDIATYPE_DSK_
#define _MEDIATYPE_DSK_

#include <stdio.h>

#include "mediaTypeWOZ.h"

// #define MAX_TRACKS 160

// struct TRK_t
// {
//     uint16_t start_block;
//     uint16_t block_count;
//     uint32_t bit_count;
// };


class MediaTypeDSK  : public MediaTypeWOZ
{
private:
    void dsk2woz_info();
    void dsk2woz_tmap();
    bool dsk2woz_tracks(uint8_t *dsk); 

public:

    virtual mediatype_t mount(FILE *f, uint32_t disksize) override;
    mediatype_t mount(FILE *f) {return mount(f, 0);};
    // virtual void unmount() override;

    // static bool create(FILE *f, uint32_t numBlock);
};


#endif // _MEDIATYPE_DSK_
