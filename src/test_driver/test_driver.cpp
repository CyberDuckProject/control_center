#include <asio.hpp> // network
#include <iomanip>
#include <iostream>
#include <jpge.h> // jpeg compression
#include <sstream>
#include <filesystem>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> // loading images from disk

#include <nfd.hpp> // for folder selection dialog

namespace ip = asio::ip;
using tcp = ip::tcp;
using udp = ip::udp;

struct Pixel
{
  unsigned char r, g, b;
};
struct ImageStorage
{
  const size_t width, height;
  std::unique_ptr<Pixel[]> data;

  ImageStorage(size_t width, size_t height) : width{width},
                                              height{height},
                                              data{std::make_unique<Pixel[]>(width * height)}
  {
  }
};
std::string show_pick_folder_dialog()
{
  NFD::UniquePathN path;
  NFD::PickFolder(path);
  std::string s = path.get();
  return s;
}
class ImageLoader
{
  size_t current_frame;
  size_t frame_count;
  std::string base_path;

public:
  ImageLoader(size_t current_frame, size_t frame_count, const std::string &base_path) : current_frame{current_frame},
                                                                                        frame_count{frame_count},
                                                                                        base_path{base_path}
  {
  }

  size_t load_next_frame(ImageStorage &out)
  {
    std::stringstream ss;
    ss << std::setw(4) << std::setfill('0') << current_frame;
    std::string s = ss.str();

    auto path = (base_path + '/' + s + ".png");

    int read_width, read_height, read_components;
    auto read_image = stbi_load(path.c_str(), &read_width, &read_height, &read_components, 3);
    if (!read_image)
      throw std::runtime_error(strerror(errno));
    if (read_width != out.width || read_height != out.height || read_components != 3)
      throw std::runtime_error("Read image not compatible with the given image storage");

    memcpy(static_cast<void *>(out.data.get()), read_image, out.width * out.height * 3);
    stbi_image_free(read_image);

    ++current_frame;
    current_frame %= frame_count;
    return current_frame;
  }
};
constexpr size_t MAX_COMPRESSED_IMG_SZ = 65000;
struct ImageCompressedStorage
{
  const size_t capacity;
  int stored_size;
  std::unique_ptr<char[]> data;

  ImageCompressedStorage(const ImageStorage &image) : capacity{std::min(image.width * image.height * 3, MAX_COMPRESSED_IMG_SZ)},
                                                      stored_size{0},
                                                      data{std::make_unique<char[]>(capacity)}
  {
  }
};
bool compress_image(ImageCompressedStorage &out, const ImageStorage &image, size_t quality)
{
  out.stored_size = out.capacity;

  auto params = jpge::params{};
  params.m_quality = quality;
  return jpge::compress_image_to_jpeg_file_in_memory(
      out.data.get(), out.stored_size, image.width, image.height, 3, reinterpret_cast<jpge::uint8 *>(image.data.get()), params);
}
constexpr int TCP_PORT = 1333;
constexpr int UDP_PORT = 1512;
class TCPConnector
{
  tcp::acceptor acceptor;
  ip::address &receiver;

  void accept()
  {
    acceptor.async_accept(std::ref(*this));
  }
  void on_connect()
  {
    // TODO: for example flash eyes
  }
  void on_receive(float left, float right)
  {
    // TODO: change motor speed
    // std::cout << "left motor: " << left << std::endl;
    // std::cout << "right motor: " << right << std::endl;
  }
  void on_disconnect()
  {
    receiver = ip::address{};
  }

public:
  TCPConnector(asio::io_context &ctx, ip::address &receiver) : acceptor{ctx},
                                                               receiver{receiver}
  {
    accept();
  }

  void operator()(asio::error_code ec, tcp::socket socket)
  {
    if (!ec)
    {
      on_connect();

      receiver = socket.remote_endpoint().address();

      while (true)
      {
        float data[2];
        asio::error_code ec;
        socket.read_some(asio::buffer(data), ec);

        if (!ec)
        {
          on_receive(data[0], data[1]);
        }
        else
        {
          on_disconnect();
          accept();
          break;
        }
      }
    }
  }
};
template <typename TransmissionGenerator>
class UDPTransmitter
{
  ip::address &receiver;
  udp::socket socket;
  TransmissionGenerator generator;

  void transmit()
  {
    socket.async_send_to(generator(), {receiver, UDP_PORT}, [&](asio::error_code ec, std::size_t b)
                         {
                           if (ec)
                             std::cerr << " ec: " << ec.message() << std::endl;
                           transmit();
                         });
  }

public:
  UDPTransmitter(asio::io_context &ctx, ip::address &receiver, TransmissionGenerator &&generator) : receiver{receiver},
                                                                                                    socket{ctx},
                                                                                                    generator{generator}
  {
    socket.open(udp::v4());
    socket.set_option(asio::socket_base::broadcast(true));
    transmit();
  }
};

struct MessageHeader
{
  uint64_t type;
  int64_t size;
};
MessageHeader generate_message_header(uint8_t type)
{
  return {
      type, time(nullptr)};
}

int main()
{
  asio::io_context ctx;
  ImageStorage image{1385, 1080};
  NFD::Init();
  ImageLoader loader{1, 4200, show_pick_folder_dialog()};

  ImageCompressedStorage compressed{image};

  ip::address receiver;
  TCPConnector connector{ctx, receiver};

  MessageHeader header;
  int frame_idx;
  float rand_val;
  int type = 0;
  UDPTransmitter transmitter{ctx, receiver, [&]()
                             {
                               header = generate_message_header(type);
                               if (type == 0)
                               {
                                 frame_idx = loader.load_next_frame(image);
                                 for (int quality = 50; !compress_image(compressed, image, quality);)
                                   quality /= 2;
                                 return std::vector{
                                     asio::buffer(&header, sizeof(header)),
                                     asio::buffer(&frame_idx, sizeof(frame_idx)),
                                     asio::buffer(compressed.data.get(), compressed.stored_size)};
                               }
                               else
                               {
                                 rand_val = rand() / static_cast<float>(RAND_MAX);
                                 return std::vector{
                                     asio::buffer(&header, sizeof(header)),
                                     asio::buffer(&rand_val, sizeof(rand_val))};
                               }

                               ++type;
                             }};

  std::thread worker1{[&]
                      { ctx.run(); }};
  ctx.run();

  return 0;
}
