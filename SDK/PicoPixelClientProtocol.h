#ifndef PICO_PIXEL_COMMON_H
#define PICO_PIXEL_COMMON_H

static const int PICO_PIXEL_NET_SIGNATURE = 0x5049434F; // 'P','I','C','O'
static const int PICO_PIXEL_SERVER_PORT   = 2001;
static const int PICO_PIXEL_MARKER_COLOR  = 0xFF66FF00;

enum PackageType
{
  PACKAGE_TYPE_UNKNOWN,
  PACKAGE_TYPE_CLIENT_HANDSHAKE,
  PACKAGE_TYPE_IMAGE,
  PACKAGE_TYPE_MARKER,
};

#pragma pack(push, 4)
struct PixelPrintfProtocol
{
  int      picomagic;
  int      payload_type;
  int      picoversion;
  int      isbigendian;
  PixelPrintfProtocol()
  {
    picomagic = PICO_PIXEL_NET_SIGNATURE;
    payload_type = PackageType::PACKAGE_TYPE_UNKNOWN;
    picoversion = 1;
    unsigned int i32 = 0x01000000;
    isbigendian = *((char*)&i32);
  }
};

struct HandShakeHeader: PixelPrintfProtocol
{
  int      size; // Size of a null terminated string. Not including the null character.
  HandShakeHeader()
  {
    payload_type = PackageType::PACKAGE_TYPE_CLIENT_HANDSHAKE;
    size = 0;
  }
};

struct PixelInfoHeader: PixelPrintfProtocol
{
  int     width;
  int     height;
  int     pitch;
  int     pixel_format;
  BOOL    srgb;
  BOOL    upside_down;

  PixelInfoHeader()
  {
    payload_type = PackageType::PACKAGE_TYPE_IMAGE;
    width = 0;
    height = 0;
    pitch = 0;
    pixel_format = 0;
    srgb = FALSE;
    upside_down = FALSE;
  }
  // [image 0 name size]    (4 bytes)
  // [image name]           (size bytes)
  // [image raw data]       (size bytes)
};

struct MarkerDataHeader: PixelPrintfProtocol
{
  int marker_count;
  MarkerDataHeader()
  {
    payload_type = PackageType::PACKAGE_TYPE_MARKER;
    marker_count = 0;
  }
  // [marker 0 index]                   (4 bytes)
  // [marker 0 use_count]               (4 bytes)
  // [marker 0 hex_color]               (4 bytes)
  // [marker 0 name size]               (4 bytes)
  // [marker 0 name string + null char] (name size + 1 byte)
  // [marker 1 index]                   (4 bytes)
  // [marker 1 use_count]               (4 bytes)
  // [marker 1 hex_color]               (4 bytes)
  // [marker 1 name size]               (4 bytes)
  // [marker 1 name string + null char] (name size + 1 byte)
  // [marker 2 index]                   (4 bytes)
  // [marker 2 use_count]               (4 bytes)
  // [marker 2 hex_color]               (4 bytes)
  // [marker 2 name size]               (4 bytes)
  // [marker 2 name string + null char] (name size + 1 byte)
  // .
  // .
  // .
  // .
};

// Markers are objects used by PixelPrintF to decide whether to send a pixel data to Pico Pixel or not.
// A marker has an associated 'use_count' (positive value). As long as a marker's use_count is greater than 0
// any call to send data with that marker will be carried through. Each time the marker is used, its use_count
// is decremented by 1.
// When a marker use_count reach zero, the marker can no longer be use to send data to Pico Pixel. Its use_count has
// to be reloaded before it can be used again.

struct Marker
{
  Marker(int index, std::string name, int use_count, unsigned int hex_color)
  {
    index_ = index;
    name_ = name;
    use_count_ = use_count;
    hex_color_ = hex_color;
    use_count_pico_pixel_update_ = -1;
  }

  Marker(int index, std::string name, int use_count)
  {
    index_ = index;
    name_ = name;
    use_count_ = use_count;
    hex_color_ = PICO_PIXEL_MARKER_COLOR;
    use_count_pico_pixel_update_ = -1;
  }

  Marker()
  {
    index_ = -1;
    use_count_ = 0;
    use_count_pico_pixel_update_ = -1;
  }

  void SetTriggerCount(int count)
  {
    use_count_ = count;
  }

  int TriggerCount() const
  {
    return use_count_;
  }

  int DecrementTriggerCount()
  {
    if (use_count_ == 0)
      return 0;

    return --use_count_;
  }

  int index_;
  int use_count_;
  int use_count_pico_pixel_update_;
  unsigned int hex_color_; //!< Color to be display in Pico Pixel interface
  std::string name_;
};
#pragma pack(pop)

#endif // PICO_PIXEL_COMMON_H
