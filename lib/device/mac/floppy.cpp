#ifdef BUILD_MAC
#include "floppy.h"
#include "../bus/mac/mac_ll.h"

#define NS_PER_BIT_TIME 125
#define BLANK_TRACK_LEN 6400

mediatype_t macFloppy::mount(FILE *f, mediatype_t disk_type) //, const char *filename), uint32_t disksize, mediatype_t disk_type)
{

  mediatype_t mt = MEDIATYPE_UNKNOWN;
  // mediatype_t disk_type = MEDIATYPE_WOZ;

  // Debug_printf("disk MOUNT %s\n", filename);

  // Destroy any existing MediaType
  if (_disk != nullptr)
  {
    delete _disk;
    _disk = nullptr;
  }

  switch (disk_type)
  {
  case MEDIATYPE_MOOF:
    Debug_printf("\nMounting Media Type MOOF");
    // init();
    device_active = true;
    _disk = new MediaTypeMOOF();
    mt = ((MediaTypeMOOF *)_disk)->mount(f);
    track_pos = 0;
    old_pos = 2; // makde different to force change_track buffer copy
    change_track(0); // initialize rmt buffer
    change_track(1); // initialize rmt buffer
    switch (_disk->num_sides)
    {
    case 1:
      fnUartBUS.write('s');
      break;
    case 2:
      fnUartBUS.write('d');
    default:
      break;
    }
    break;
  // case MEDIATYPE_DSK:
  //   Debug_printf("\nMounting Media Type DSK");
  //   device_active = true;
  //   _disk = new MediaTypeDSK();
  //   mt = ((MediaTypeDSK *)_disk)->mount(f);
  //   change_track(0); // initialize spi buffer
  //   break;
  default:
    Debug_printf("\nMedia Type UNKNOWN - no mount in floppy.cpp");
    device_active = false;
    break;
  }

  return mt;
}

// void macFloppy::init()
// {
//   track_pos = 80;
//   old_pos = 0;
//   device_active = false;
// }

/* MCI/DCD signals

 * 800 KB GCR Drive
CA2	    CA1	    CA0	    SEL	    RD Output       PIO
Low	    Low	    Low	    Low	    !DIRTN          latch
Low	    Low	    Low	    High	  !CSTIN          latch
Low	    Low	    High	  Low	    !STEP           latch
Low	    Low	    High	  High	  !WRPROT         latch
Low	    High	  Low	    Low	    !MOTORON        latch
Low	    High    Low     High    !TK0            latch
Low	    High	  High	  Low	    SWITCHED        latch
Low	    High	  High	  High	  !TACH           tach
High	  Low	    Low	    Low	    RDDATA0         echo
High	  Low	    Low	    High	  RDDATA1         echo
High	  Low	    High	  Low	    SUPERDRIVE      latch
High	  Low	    High	  High	  +               latch
High	  High	  Low	    Low	    SIDES           latch
High	  High	  Low	    High	  !READY          latch
High	  High	  High	  Low	    !DRVIN          latch
High	  High	  High	  High	  REVISED         latch
+ TODO

Signal Descriptions
Signal Name 	Description
!DIRTN	      Step direction; low=toward center (+), high=toward rim (-)
!CSTIN	      Low when disk is present
!STEP	        Low when track step has been requested
!WRPROT	      Low when disk is write protected or not inserted
!MOTORON	    Low when drive motor is on
!TK0	        Low when head is over track 0 (outermost track)
SWITCHED	    High when disk has been changed since signal was last cleared
!TACH	        Tachometer; frequency reflects drive speed in RPM
INDEX	        Pulses high for ~2 ms once per rotation
RDDATA0	      Signal from bottom head; falling edge indicates flux transition
RDDATA1	      Signal from top head; falling edge indicates flux transition
SUPERDRIVE	  High when a Superdrive (FDHD) is present
MFMMODE	      High when drive is in MFM mode
SIDES	        High when drive has a top head in addition to a bottom head
!READY	      Low when motor is at proper speed and head is ready to step
!DRVIN	      Low when drive is installed
REVISED	      High for double-sided double-density drives, low for single-sided double-density drives
PRESENT/!HD	  High when a double-density (not high-density) disk is present on a high-density drive
DCDDATA	      Communication channel from DCD device to Macintosh
!HSHK	        Low when DCD device is ready to receive or wishes to send

*/

void macFloppy::unmount()
{
}

int IRAM_ATTR macFloppy::step()
{
  // done - todo: move head by 2 steps (keep on even track numbers) for side 0 of disk
  // done - todo: change_track() should copy both even and odd tracks from SPRAM to DRAM
  // todo: the next_bit() should pick from even or odd track buffer based on HDSEL

  if (!device_active)
    return -1;

  old_pos = track_pos;
  track_pos += 2 * head_dir;
  if (track_pos < 0)
  {
    track_pos = 0;
  }
  else if (track_pos > MAX_TRACKS - 2)
  {
    track_pos = MAX_TRACKS - 2;
  }
  // change_track(0);
  // change_track(1);
  return (track_pos / 2);
}

void macFloppy::update_track_buffers()
{
  change_track(0);
  change_track(1);
}

void IRAM_ATTR macFloppy::change_track(int side)
{
  int tp = track_pos + side;
  int op = old_pos + side;

  if (!device_active)
    return;

  if (op == tp)
    return;

  // should only copy track data over if it's changed
  if (((MediaTypeMOOF *)_disk)->trackmap(op) == ((MediaTypeMOOF *)_disk)->trackmap(tp))
    return;

  // need to tell diskii_xface the number of bits in the track
  // and where the track data is located so it can convert it
  if (((MediaTypeMOOF *)_disk)->trackmap(tp) != 255)
    floppy_ll.copy_track(
        ((MediaTypeMOOF *)_disk)->get_track(tp), 
        side,
        ((MediaTypeMOOF *)_disk)->track_len(tp),
        ((MediaTypeMOOF *)_disk)->num_bits(tp),
        NS_PER_BIT_TIME * ((MediaTypeMOOF *)_disk)->optimal_bit_timing);
  else
    floppy_ll.copy_track(
        nullptr,
        side,
        BLANK_TRACK_LEN,
        BLANK_TRACK_LEN * 8,
        NS_PER_BIT_TIME * ((MediaTypeMOOF *)_disk)->optimal_bit_timing);
  // Since the empty track has no data, and therefore no length, using a fake length of 51,200 bits (6400 bytes) works very well.
}


#endif // BUILD_MAC

#if 0
#include "disk2.h"

#include "fnSystem.h"
#include "fuji.h"
#include "fnHardwareTimer.h"

#define NS_PER_BIT_TIME 125
#define BLANK_TRACK_LEN 6400

const int8_t phase2seq[16] = {-1, 0, 2, 1, 4, -1, 3, -1, 6, 7, -1, -1, 5, -1, -1, -1};
const int8_t seq2steps[8] = {0, 1, 2, 3, 0, -3, -2, -1};

iwmDisk2::~iwmDisk2()
{
}

void iwmDisk2::shutdown()
{
}

iwmDisk2::iwmDisk2()
{
  track_pos = 80;
  old_pos = 0;
  oldphases = 0;
  Debug_printf("\nNew Disk ][ object");
  device_active = false;
}

void iwmDisk2::init()
{
  track_pos = 80;
  old_pos = 0;
  oldphases = 0;
  device_active = false;
}

mediatype_t iwmDisk2::mount(FILE *f, mediatype_t disk_type) //, const char *filename), uint32_t disksize, mediatype_t disk_type)
{

  mediatype_t mt = MEDIATYPE_UNKNOWN;
  // mediatype_t disk_type = MEDIATYPE_WOZ;

  // Debug_printf("disk MOUNT %s\n", filename);

  // Destroy any existing MediaType
  if (_disk != nullptr)
  {
    delete _disk;
    _disk = nullptr;
  }

  switch (disk_type)
  {
  case MEDIATYPE_WOZ:
    Debug_printf("\nMounting Media Type WOZ");
    device_active = true;
    _disk = new MediaTypeWOZ();
    mt = ((MediaTypeWOZ *)_disk)->mount(f);
    change_track(0); // initialize spi buffer
    break;
  case MEDIATYPE_DSK:
    Debug_printf("\nMounting Media Type DSK");
    device_active = true;
    _disk = new MediaTypeDSK();
    mt = ((MediaTypeDSK *)_disk)->mount(f);
    change_track(0); // initialize spi buffer
    break;
  default:
    Debug_printf("\nMedia Type UNKNOWN - no mount in disk2.cpp");
    device_active = false;
    break;
  }

  return mt;
}

void iwmDisk2::unmount()
{
}

bool iwmDisk2::write_blank(FILE *f, uint16_t sectorSize, uint16_t numSectors)
{
  return false;
}

bool IRAM_ATTR iwmDisk2::phases_valid(uint8_t phases)
{
  return (phase2seq[phases] != -1);
}

bool IRAM_ATTR iwmDisk2::move_head()
{
  int delta = 0;
  uint8_t newphases = smartport.iwm_phase_vector(); // could access through IWM instead
  if (phases_valid(newphases))
  {
    int idx = (phase2seq[newphases] - phase2seq[oldphases] + 8) % 8;
    delta = seq2steps[idx];

    // phases_lut[oldphases][newphases];
    old_pos = track_pos;
    track_pos += delta;
    if (track_pos < 0)
    {
      track_pos = 0;
    }
    else if (track_pos > MAX_TRACKS - 1)
    {
      track_pos = MAX_TRACKS - 1;
    }
    oldphases = newphases;
  }
  return (delta != 0);
}

void IRAM_ATTR iwmDisk2::change_track(int indicator)
{
  if (!device_active)
    return;

  if (old_pos == track_pos)
    return;

  // should only copy track data over if it's changed
  if (((MediaTypeWOZ *)_disk)->trackmap(old_pos) == ((MediaTypeWOZ *)_disk)->trackmap(track_pos))
    return;

  // need to tell diskii_xface the number of bits in the track
  // and where the track data is located so it can convert it
  if (((MediaTypeWOZ *)_disk)->trackmap(track_pos) != 255)
    diskii_xface.copy_track(
        ((MediaTypeWOZ *)_disk)->get_track(track_pos),
        ((MediaTypeWOZ *)_disk)->track_len(track_pos),
        ((MediaTypeWOZ *)_disk)->num_bits(track_pos),
        NS_PER_BIT_TIME * ((MediaTypeWOZ *)_disk)->optimal_bit_timing);
  else
    diskii_xface.copy_track(
        nullptr,
        BLANK_TRACK_LEN,
        BLANK_TRACK_LEN * 8,
        NS_PER_BIT_TIME * ((MediaTypeWOZ *)_disk)->optimal_bit_timing);
  // Since the empty track has no data, and therefore no length, using a fake length of 51,200 bits (6400 bytes) works very well.
}

#endif /* BUILD_APPLE */