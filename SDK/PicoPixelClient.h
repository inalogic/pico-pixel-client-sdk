#ifndef PICO_PIXEL_CLIENT_H
#define PICO_PIXEL_CLIENT_H

# pragma comment(lib, "Ws2_32.lib")
# include <windows.h>
# include <winsock2.h>
# include <Ws2tcpip.h>
# include <string>

class PicoPixelClient
{
public:
  enum PixelFormat
  {
    PIXEL_FORMAT_UNKNOWN,
    PIXEL_FORMAT_RGBA8,
    PIXEL_FORMAT_BGRA8,
    PIXEL_FORMAT_ARGB8,
    PIXEL_FORMAT_ABGR8,
    PIXEL_FORMAT_RGB8,
    PIXEL_FORMAT_BGR8,
    PIXEL_FORMAT_R5G6B5,

    PIXEL_FORMAT_DEPTH,
    
    PIXEL_FORMAT_FORCE32 = 0x7fffffff
  };

  struct ImageInfo
  {
    PixelFormat       pixel_format;
    unsigned int      width;
    unsigned int      height;
    unsigned int      pitch;
    unsigned int      size;
    BOOL              srgb;
    BOOL              upside_down;
    std::string       image_name;
  };

  PicoPixelClient(std::string client_id);
  ~PicoPixelClient();

  bool StartConnection(bool start_receiver_thread = true);
  bool StartConnectionToHost(std::string host_ip, int port, bool start_receiver_thread = true);
  void EndConnection();
  bool Connected();

  int AddMarker(std::string name, int use_count);
  int AddMarker(std::string name, int use_count, unsigned int color);
  int MarkerUseCount(int marker_index);

  void DeleteAllAddMarkers();
  void DeleteMarker(int index);
  void DeleteMarker(std::string name);
  void SynchronizeMarkers();
  void AutoSynchronizeMarkers();
  void DisableAutoSynchronizeMarkers();

  void SendMarkersToPicoPixel();
  void UpdateMarkersFromPicoPixel();

  /*!
      Sends an image raw data to PicoPixel.

      @param image_info     Structure holding the information of the image to send.
      @param data           The image raw data.

      @return Returns true is the pixel data was sent successfully.
  */
  bool PixelPrintf(const ImageInfo& image_info, char* data);

  /*!
      Sends an image raw data to PicoPixel.

      @param image_name     The name of the image. It will be displayed in PicoPixel title bar.
      @param pixel_format   Pixel format of the image.
      @param width          Image width.
      @param height         Image height.
      @param data_size      Data size in bytes.
      @param pitch          Image pitch. This is the number of bytes from one pixel in the image to the pixel just below.
      @param data           The image raw data.

      @return Returns true is the pixel data was sent successfully.
  */
  bool PixelPrintf(std::string image_name,
    PixelFormat pixel_format,
    int width,
    int height,
    int pitch,
    BOOL srgb,
    BOOL upside_down,
    char* data);

  /*!
      Sends an image raw data to PicoPixel.

      @param image_info     Structure holding the information of the image to send.
      @param data           The image raw data.

      @return Returns true is the pixel data was sent successfully.
  */
  bool PixelPrintf(int marker_index, const ImageInfo& image_info, char* data);

  /*!
      Sends an image raw data to PicoPixel.

      @param image_name     The name of the image. It will be displayed in PicoPixel title bar.
      @param pixel_format   Pixel format of the image.
      @param width          Image width.
      @param height         Image height.
      @param data_size      Data size in bytes.
      @param pitch          Image pitch. This is the number of bytes from one pixel in the image to the pixel just below.
      @param data           The image raw data.

      @return Returns true is the pixel data was sent successfully.
  */
  bool PixelPrintf(int marker_index,
    std::string image_name,
    PixelFormat pixel_format,
    int width,
    int height,
    int pitch,
    BOOL srgb,
    BOOL upside_down,
    char* data);

#ifdef PICO_PIXEL_CLIENT_OPENGL
  // Experimental
  bool PixelPrintfGLColorBuffer(int marker_index, std::string image_name, BOOL upside_down);
  bool PixelPrintfGLDepthBuffer(int marker_index, std::string image_name, BOOL upside_down);
#endif

private:
  struct Impl;
  Impl* impl_;
};

#endif PICO_PIXEL_CLIENT_H
