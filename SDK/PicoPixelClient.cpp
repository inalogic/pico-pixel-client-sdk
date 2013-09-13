#include "PicoPixelClient.h"
#include "PicoPixelClientProtocol.h"
#include <process.h>
#include <iostream>
#include <vector>
#include <sstream>

static const char* PIXEL_PRINTF_CLIENT_FILE_NAME = "Pixel-PrintF-Image";
static const int PIXEL_PRINTF_RECV_TIMEOUT  = 1000;
static const int PIXEL_PRINTF_RECV_TRIALS   = 3;

struct PicoPixelClient::Impl
{
  Impl(PicoPixelClient* parent)
    : parent_(parent)
    , sock_(INVALID_SOCKET)
    , receiver_thread_(0)
    , thread_id_(0)
    , markers_auto_sync_(true)
  {}

  bool Connected() const;

  bool SendRaw(const char* ptr, int size);
  bool SendString(const std::string& str);
  bool SendInteger(int);

  void FlushRecvBuffer();
  int RecvInteger(int* val, int expected_size);
  int RecvString(char* str, int expected_size, char str_terminal_char = 0);
  int RecvRaw(char* dst_buffer, unsigned int buffer_size, unsigned int timeout, bool& connection_close, bool peek = false);

  void HandShake(std::string client_id);
  
  static DWORD WINAPI ReceiverThread(void* ptr);

  SOCKET sock_;
  std::string picopixel_server_ip_;
  std::string client_id_;
  PicoPixelClient* parent_;
  HANDLE receiver_thread_;
  DWORD thread_id_;

  std::vector<Marker> markers_;
  bool markers_auto_sync_;

  static int default_timeout_millisec_;
  static int trials_read_on_socket;
};

int PicoPixelClient::Impl::default_timeout_millisec_ = PIXEL_PRINTF_RECV_TIMEOUT;
int PicoPixelClient::Impl::trials_read_on_socket = PIXEL_PRINTF_RECV_TRIALS;

bool PicoPixelClient::Impl::Connected() const
{
  return sock_ != INVALID_SOCKET;
}

int PicoPixelClient::Impl::RecvRaw(char* dst_buffer,
                                   unsigned int buffer_size,
                                   unsigned int timeout,
                                   bool& connection_closed,
                                   bool peek)
{
  if (!Connected())
  {
    connection_closed = true;
    return 0;
  }

  if (dst_buffer == NULL || buffer_size < 1L)
  {
    return 0;
  }

  TIMEVAL  stTime;
  SOCKET socket = sock_;

  fd_set  fd_read;
  FD_ZERO(&fd_read);
  FD_SET(socket, &fd_read);
  stTime.tv_sec = timeout / 1000;
  stTime.tv_usec = timeout % 1000;

  // Select function set read timeout
  unsigned int bytes_read = 0;

  int res = select((int)(socket + 1), &fd_read, NULL, NULL, &stTime);
  if (res == SOCKET_ERROR)
  {
    printf("[PicoPixelClient::Impl::ReadSocket] 'select' has failed.\n");
    return 0xFFFFFFFF;
  }

  int error_code = 0;
  if (FD_ISSET(socket, &fd_read))
  {
    res = recv(socket, (char*)dst_buffer, buffer_size, peek ? MSG_PEEK : 0);

    if (res == SOCKET_ERROR)
    {
      printf("[PicoPixelClient::Impl::ReadSocket] 'recv' has failed.\n");
      error_code = WSAGetLastError();
      return -1;
    }

    if (res == 0)
    {
      printf("[PicoPixelClient::Impl::ReadSocket] Connection closed.\n");
      connection_closed = true;
      sock_ = INVALID_SOCKET;
      return 0;
    }

    bytes_read = (unsigned int)((res >= 0) ? (res) : (-1L));
  }

  return bytes_read;
}

DWORD PicoPixelClient::Impl::ReceiverThread(void* ptr)
{
  PicoPixelClient* pixel_printf = static_cast<PicoPixelClient*>(ptr);

  unsigned int  bytes_read = 0;
  unsigned int  timeout = 1000;
  bool connection_closed = false;
  bool connection_dropped = false;
  const int receive_buffer_size = 1024;
  char receive_buffer[receive_buffer_size];

  while ((!connection_closed) && (!connection_dropped))
  {
    // Blocking mode: Wait for event
    bytes_read = 0;
    int res = pixel_printf->impl_->RecvRaw(receive_buffer, receive_buffer_size, timeout, connection_closed, true);

    if (res == SOCKET_ERROR)
    {
      connection_dropped = true;
    }

    if (connection_closed || connection_dropped)
    {
      // This thread is bout to be terminated.
      if (connection_closed)
      {
        printf("[PicoPixelClient::Impl::ReceiverThread] Connection closed.\n");
      }
      else
      {
        printf("[PicoPixelClient::Impl::ReceiverThread] Connection droped.\n");
      }
    }

    if (res > 0)
    {
      PixelPrintFProtocol* pixel_printf_header = (PixelPrintFProtocol*)receive_buffer;

      if ((pixel_printf_header->picomagic == PICO_PIXEL_NET_SIGNATURE) && (pixel_printf_header->payload_type == PackageType::PACKAGE_TYPE_MARKER))
      {
        MarkerDataHeader payload_markers;
        pixel_printf->impl_->RecvRaw((char*)&payload_markers, sizeof(payload_markers), PIXEL_PRINTF_RECV_TIMEOUT, connection_closed);

        for (int i = 0; i < payload_markers.marker_count; ++i)
        {
          int index = 0;
          int use_count = 0;
          int name_size = 0;
          std::string name;
          
          if (pixel_printf->impl_->RecvInteger(&index, 1) <= 0)
          {
            pixel_printf->impl_->FlushRecvBuffer();
            break;
          }

          if (pixel_printf->impl_->RecvInteger(&use_count, 1) <= 0)
          {
            pixel_printf->impl_->FlushRecvBuffer();
            break;
          }

          if (pixel_printf->impl_->RecvInteger(&name_size, 1) <= 0)
          {
            pixel_printf->impl_->FlushRecvBuffer();
            break;
          }

          char* str = new char[name_size];
          if (pixel_printf->impl_->RecvString(str, name_size) <= 0)
          {
            delete [] str;
            pixel_printf->impl_->FlushRecvBuffer();
            break;
          }
          delete [] str;

          if (index < (int)pixel_printf->impl_->markers_.size())
          {
            printf("value: %d\n", use_count);
            if (pixel_printf->impl_->markers_auto_sync_)
            {
              pixel_printf->impl_->markers_[index].use_count_ = use_count;
              pixel_printf->impl_->markers_[index].use_count_pico_pixel_update_ = -1;
            }
            else
            {
              pixel_printf->impl_->markers_[index].use_count_pico_pixel_update_ = use_count;
            }
          }
        }
      }
      else
      {
        pixel_printf->impl_->FlushRecvBuffer();
      }
      bytes_read = res;
    }
  }

  return 0;
}

bool PicoPixelClient::Impl::SendRaw(const char* ptr, int size)
{
  if (ptr == NULL)
    return false;
  
  if (size <= 0)
    return false;

  if (size > INT_MAX)
    return false;

  int error_code = 0;
  int total_sent = 0;
  int ret = 0;

  while(total_sent < size)
  {
    ret = send(sock_, (const char*)ptr + total_sent, size - total_sent, 0);
    if (ret == -1)
    {
      break;
    }
    total_sent += ret;
  }
  
  if( ret == -1 )
  {
    error_code = WSAGetLastError();
    printf("[PixelPrintF] Failed to send data to Pico Pixel server.\n");
    return false;
  }
  return true;
}

bool PicoPixelClient::Impl::SendString(const std::string& str)
{
  int size = (int)str.size() + 1;           // +1 for null terminated string
  bool ret = SendRaw((char*)&size, sizeof(int));   // send string size
  if (ret == false) return false;
  ret = SendRaw(str.c_str(), size);           // send null terminated string
  if (ret == false) return false;
  return true;
}

bool PicoPixelClient::Impl::SendInteger(int integer)
{
  return SendRaw((char*)&integer, sizeof(int));
}

void PicoPixelClient::Impl::FlushRecvBuffer()
{
  char buffer_flush[256];
  bool end = false;
  bool connection_closed = false;
  int count = 0;
  while (!end)
  {
    count = RecvRaw((char*)buffer_flush, 256, default_timeout_millisec_, connection_closed);
    if (count <= 0 || connection_closed)
    {
      end = true;
    }
  }
}

int PicoPixelClient::Impl::RecvInteger(int* val, int expected_size)
{
  int count = 0;
  int total_bytes = 0;
  bool end = false;
  bool connection_closed = false;
  int trial = 0;
  while (!end)
  {
    count = RecvRaw((char*)val + count, expected_size * sizeof(int) - count, default_timeout_millisec_, connection_closed);
    if ((count == 0) && (trial++ == Impl::trials_read_on_socket))
    {
      return total_bytes / sizeof(int);
    }

    total_bytes += count;
    if (total_bytes == expected_size * sizeof(int))
      end = true;
  }

  return total_bytes / sizeof(int);
}

int PicoPixelClient::Impl::RecvString(char* str, int expected_size, char str_terminal_char)
{
  int count = 0;
  int total_bytes = 0;
  bool end = false;
  bool connection_closed = false;
  int trial = 0;
  while (!end)
  {
    count = RecvRaw(str + count, expected_size * sizeof(char) - count, default_timeout_millisec_, connection_closed);
    if ((count == 0) && (trial++ == Impl::trials_read_on_socket))
      return total_bytes / sizeof(char);

    total_bytes += count;
    if (total_bytes == expected_size)
      end = true;
  }

  return total_bytes / sizeof(char);
}

void PicoPixelClient::Impl::HandShake(std::string client_id)
{
  HandShakeHeader hand_shake;
  hand_shake.size = (unsigned int) client_id.size() + 1;

  if (hand_shake.size > UINT_MAX)
    return;

  SendRaw(reinterpret_cast<const char*>(&hand_shake), sizeof(HandShakeHeader));
  SendRaw(client_id.c_str(), (unsigned int)client_id.size() + 1);
}

PicoPixelClient::PicoPixelClient(std::string client_id)
  : impl_(new Impl(this))
{
  impl_->client_id_ = client_id;
}

PicoPixelClient::~PicoPixelClient()
{
  if (impl_->receiver_thread_ != NULL)
  {
    ::CloseHandle(impl_->receiver_thread_);
    impl_->receiver_thread_ = NULL;

  }
  if (Connected())
  {
    EndConnection();
  }
}

bool PicoPixelClient::StartConnection(bool start_receiver_thread)
{
  // Will attempt to connect to local port.
  return StartConnectionToHost(std::string(""), PICO_PIXEL_SERVER_PORT, start_receiver_thread);
}

bool PicoPixelClient::StartConnectionToHost(std::string host_ip, int port, bool start_receiver_thread)
{
  if (Connected())
  {
    return true;
  }

  if (port <= 1024)
  {
    // Reserved ports on Windows
    return false;
  }

  int err = 0;
  int ret = 0;
  int const host_name_size = 512;
  int const host_port_size = 32;
  char host_name[host_name_size] = {0};
  char host_port[host_port_size] = {0};
  WSADATA wsaData;

  std::memset(host_name, 0, host_name_size);
  std::memset(host_port, 0, host_port_size);

  if (!host_ip.empty() && host_ip.size() < host_name_size - 1)
  {
    std::memcpy(host_name, host_ip.c_str(), host_ip.size());
  }

#if WIN32
  err = sprintf_s(host_port, host_port_size-1, "%d", port);
  if (err < 0)
    return false;
#else
  err = snprintf(host_port, host_port_size-1, "%d", port);
  if (err < 0)
    return false;
#endif

#if WIN32
  err = WSAStartup( MAKEWORD( 2, 2 ), &wsaData );
  if (err != 0)
  {
    printf("[PicoPixelClient::PicoPixelClient] WSAStartup has failed: %d\n", err);
    return false;
  }
#endif

  ////
  struct addrinfo ai_hints;
  struct addrinfo* ai_list = NULL;

  ::memset(&ai_hints, 0, sizeof(ai_hints));
  ai_hints.ai_family = PF_INET;
  ai_hints.ai_socktype = SOCK_STREAM;
  ai_hints.ai_protocol = IPPROTO_TCP;

  ret = getaddrinfo(host_name, host_port, &ai_hints, &ai_list);
  if (ret)
  {
    printf("[LocalIPString] 'getaddrinfo' error: %d.\n", ret);
    return false;
  }

  struct sockaddr_in* sa = (struct sockaddr_in*) ai_list->ai_addr;
  host_name[0] = 0;
  ret = sprintf_s(host_name, host_name_size, "%u.%u.%u.%u",
    (UINT)(sa->sin_addr.S_un.S_un_b.s_b1),
    (UINT)(sa->sin_addr.S_un.S_un_b.s_b2),
    (UINT)(sa->sin_addr.S_un.S_un_b.s_b3),
    (UINT)(sa->sin_addr.S_un.S_un_b.s_b4));

  impl_->picopixel_server_ip_ = host_name;

  struct addrinfo* ai = NULL;
  for (ai = ai_list; ai; ai = ai->ai_next)
  {
    impl_->sock_ = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (impl_->sock_ == INVALID_SOCKET)
    {
      int error_code = WSAGetLastError();
      printf("[PixelPrintF] Failed to create network socket. Error code: %d.\n", error_code);

      continue;
    }

    ret = connect(impl_->sock_, ai->ai_addr, (int)ai->ai_addrlen);
    if( ret == -1 )
    {
      closesocket(impl_->sock_);
      impl_->sock_ = INVALID_SOCKET;
      continue;
    }

    // we have a good connection.
    break;
  }

  freeaddrinfo(ai_list);

  if (ai == NULL)
  {
    printf("[PicoPixelClient::PicoPixelClient] Connection failed.\n");
    printf("[PicoPixelClient::PicoPixelClient] You need to launch Pico Pixel desktop application before running this program.\n");
    return false;
  }

  if (impl_->sock_ != INVALID_SOCKET)
  {
    impl_->HandShake(impl_->client_id_);
  }

  if (start_receiver_thread)
  {
    impl_->receiver_thread_ = ::CreateThread(NULL,
      0,
      PicoPixelClient::Impl::ReceiverThread,
      this,
      CREATE_SUSPENDED,
      &impl_->thread_id_);

    if (impl_->receiver_thread_ != NULL)
    {
      ResumeThread(impl_->receiver_thread_);
      return true;
    }
  }
  return true;
}

void PicoPixelClient::EndConnection()
{
  if (impl_->sock_ == INVALID_SOCKET)
    return;

  int res = shutdown(impl_->sock_, SD_BOTH);

  if (res == SOCKET_ERROR)
  {
    printf("[PicoPixelClient::EndConnection] shutdown failed: %d\n", WSAGetLastError());
  }

  closesocket(impl_->sock_);
  impl_->sock_ = INVALID_SOCKET;
  WSACleanup();
}

bool PicoPixelClient::Connected()
{
  return impl_->sock_ != INVALID_SOCKET;
}

int PicoPixelClient::AddMarker(std::string name, int use_count)
{
  return AddMarker(name, use_count, PICO_PIXEL_MARKER_COLOR);
}

int PicoPixelClient::AddMarker(std::string name, int use_count, unsigned int color)
{
  std::vector<Marker>::iterator it;
  for (it = impl_->markers_.begin(); it != impl_->markers_.end(); ++it)
  {
    if ((*it).name_ == name)
    {
      std::cout << "[PicoPixelClient::AddMarker] There is already a marker with name " << name << std::endl;
      return -1;
    }
  }

  int index = (int)impl_->markers_.size();
  Marker m(index, name, use_count, color);

  impl_->markers_.push_back(m);
  return index;
}

int PicoPixelClient::MarkerUseCount(int marker_index)
{
  int marker_count = (int) impl_->markers_.size();
  if (marker_index >= marker_count)
  {
    printf("[PicoPixelClient::MarkerUseCount] Invalid marker index.\n");
    return -1;
  }

  return impl_->markers_[marker_index].use_count_;
}

void PicoPixelClient::DeleteAllAddMarkers()
{
  impl_->markers_.clear();
  SendMarkersToPicoPixel();
}

void PicoPixelClient::DeleteMarker(int index)
{
  if (index < 0)
    return;

  if (index >= (int)impl_->markers_.size())
    return;

  std::vector<Marker>::iterator it;
  for (it = impl_->markers_.begin(); it != impl_->markers_.end(); ++it)
  {
    if ((*it).index_ == index && (*it).index_ != -1)
    {
      (*it).index_ = -1;
      return;
    }
  }
}

void PicoPixelClient::DeleteMarker(std::string name)
{
  std::vector<Marker>::iterator it;
  for (it = impl_->markers_.begin(); it != impl_->markers_.end(); ++it)
  {
    if ((*it).name_ == name && (*it).index_ != -1)
    {
      (*it).index_ = -1;
      return;
    }
  }
}

void PicoPixelClient::SynchronizeMarkers()
{
  std::vector<Marker>::iterator it;
  for (it = impl_->markers_.begin(); it != impl_->markers_.end(); ++it)
  {
    if ((*it).use_count_pico_pixel_update_ >= 0)
    {
      (*it).use_count_pico_pixel_update_ = (*it).use_count_;
    }
  }
}

void PicoPixelClient::AutoSynchronizeMarkers()
{
  impl_->markers_auto_sync_ = true;
}

void PicoPixelClient::DisableAutoSynchronizeMarkers()
{
  impl_->markers_auto_sync_ = false;
}

void PicoPixelClient::SendMarkersToPicoPixel()
{
  if (!Connected())
    return;

  MarkerDataHeader payload;
  payload.marker_count = (int)impl_->markers_.size();
  impl_->SendRaw((const char*)&payload, sizeof(payload));
  std::vector<Marker>::iterator it;
  for (it = impl_->markers_.begin(); it != impl_->markers_.end(); ++it)
  {
    int str_size = (int)(*it).name_.size() + 1;
    impl_->SendRaw((const char*)&((*it).index_),      sizeof(int));
    impl_->SendRaw((const char*)&((*it).use_count_),  sizeof(int));
    impl_->SendRaw((const char*)&((*it).hex_color_),  sizeof(int));
    impl_->SendRaw((const char*)&str_size,            sizeof(int));
    impl_->SendRaw((const char*)(*it).name_.c_str(),  str_size);
  }
}

bool PicoPixelClient::PixelPrintF(int marker_index, const ImageInfo& image_info, char* data)
{
  int marker_count = (int) impl_->markers_.size();
  if (marker_index >= marker_count)
    return false;
  
  if (impl_->markers_[marker_index].use_count_ <= 0)
    return false;

  --impl_->markers_[marker_index].use_count_;

  return PixelPrintF(image_info, data);

}

bool PicoPixelClient::PixelPrintF(const ImageInfo& image_info, char* data)
{
  return PixelPrintF(
    image_info.image_name,
    image_info.pixel_format,
    image_info.width,
    image_info.height,
    image_info.pitch,
    image_info.srgb,
    image_info.upside_down,
    data);
}

bool PicoPixelClient::PixelPrintF(std::string image_name,
                                  PicoPixelClient::PixelFormat pixel_format,
                                  int width,
                                  int height,
                                  int pitch,
                                  BOOL srgb,
                                  BOOL upside_down,
                                  char* data)
{
  if (!Connected())
    return false;

  if (width <= 0 ||
    height <= 0 ||
    pitch <= 0)
    return false;

  if (data == NULL)
    return false;

  PixelInfoHeader pixel_info;
  pixel_info.width = width;
  pixel_info.height = height;
  pixel_info.pixel_format = pixel_format;
  pixel_info.pitch = pitch;
  pixel_info.srgb = srgb;
  pixel_info.upside_down = upside_down;

  static int image_name_index = 0;

  //Send the header over to the server

  std::string network_image_name = image_name;
  if (network_image_name.empty())
  {
    std::ostringstream stream;
    stream << image_name_index++;
    network_image_name = std::string(PIXEL_PRINTF_CLIENT_FILE_NAME) + stream.str();
  }

  bool success = impl_->SendRaw((const char*)&pixel_info, sizeof(PixelInfoHeader));
  if (success == false)
  {
    printf("[PixelPrintF] Failed to send data to Pico Pixel server.");
    return false;
  }

  success = impl_->SendString(network_image_name);
  if (success == false)
  {
    printf("[PixelPrintF] Failed to send data to Pico Pixel server.");
    return false;
  }

  int size = pitch * height;
  success = impl_->SendRaw(data, size);
  if (success == false)
  {
    printf("[PixelPrintF] Failed to send data to Pico Pixel server.");
    return false;
  }
  return true;
}

bool PicoPixelClient::PixelPrintF(int marker_index,
                                  std::string image_name,
                                  PixelFormat pixel_format,
                                  int width,
                                  int height,
                                  int pitch,
                                  BOOL srgb,
                                  BOOL upside_down,
                                  char* data)
{
  int marker_count = (int) impl_->markers_.size();
  if (marker_index >= marker_count)
    return false;

  if (impl_->markers_[marker_index].use_count_ <= 0)
    return false;

  --impl_->markers_[marker_index].use_count_;

  return PixelPrintF(
    image_name,
    pixel_format,
    width,
    height,
    pitch,
    srgb,
    upside_down,
    data);
}

