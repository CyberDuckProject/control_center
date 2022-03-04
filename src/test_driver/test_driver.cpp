#include <asio.hpp> // network
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <jpge.h> // jpeg compression
#include <sstream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> // loading images from disk

#include <nfd.hpp> // for folder selection dialog

namespace ip = asio::ip;
using tcp = ip::tcp;
using udp = ip::udp;

struct Pixel {
  unsigned char r, g, b;
};
struct ImageStorage {
  const size_t width, height;
  std::unique_ptr<Pixel[]> data;

  ImageStorage(size_t width, size_t height)
      : width{width}, height{height}, data{std::make_unique<Pixel[]>(width *
                                                                     height)} {}
  ImageStorage(const ImageStorage &other)
      : ImageStorage{other.width, other.height} {}
};
std::string show_pick_folder_dialog() {
  NFD::UniquePathN path;
  NFD::PickFolder(path);
  std::string s = path.get();
  return s;
}
class ImageLoader {
  inline static std::atomic<size_t> current_frame = 1;
  const size_t frame_count;
  const std::string base_path;

public:
  ImageLoader(size_t frame_count, const std::string &base_path)
      : frame_count{frame_count}, base_path{base_path} {}

  size_t load_next_frame(ImageStorage &out) {
    const size_t current_frame_val = current_frame.fetch_add(1) % frame_count;

    std::stringstream ss;
    ss << std::setw(4) << std::setfill('0') << current_frame_val;
    std::string s = ss.str();

    auto path = (base_path + '/' + s + ".png");

    int read_width, read_height, read_components;
    auto read_image =
        stbi_load(path.c_str(), &read_width, &read_height, &read_components, 3);
    if (!read_image)
      throw std::runtime_error(strerror(errno));
    if (read_width != out.width || read_height != out.height ||
        read_components != 3)
      throw std::runtime_error(
          "Read image not compatible with the given image storage");

    memcpy(static_cast<void *>(out.data.get()), read_image,
           out.width * out.height * 3);
    stbi_image_free(read_image);

    return current_frame_val;
  }
};
constexpr size_t MAX_COMPRESSED_IMG_SZ = 65001;
struct ImageCompressedStorage {
  const size_t capacity;
  int stored_size;
  std::unique_ptr<char[]> data;

  ImageCompressedStorage(size_t width, size_t height)
      : capacity{std::min(width * height * 3, MAX_COMPRESSED_IMG_SZ)},
        stored_size{0}, data{std::make_unique<char[]>(capacity)} {}
  ImageCompressedStorage(const ImageCompressedStorage &other)
      : ImageCompressedStorage{1, other.capacity / 3} {}
};
bool compress_image(ImageCompressedStorage &out, const ImageStorage &image,
                    size_t quality) {
  out.stored_size = out.capacity;

  auto params = jpge::params{};
  params.m_quality = quality;
  return jpge::compress_image_to_jpeg_file_in_memory(
      out.data.get(), out.stored_size, image.width, image.height, 3,
      reinterpret_cast<jpge::uint8 *>(image.data.get()), params);
}
constexpr int MOTOR_TCP_PORT = 1333;
constexpr int VIDEO_UDP_PORT = 1512;
constexpr int SENSOR_UDP_PORT = 1666;
class TCPConnector {
  tcp::acceptor acceptor;
  ip::address &receiver;

  void accept() { acceptor.async_accept(std::ref(*this)); }
  void on_connect() {
    // TODO: for example flash eyes
  }
  void on_receive(float left, float right) {
    // TODO: change motor speed
    // std::cout << "left motor: " << left << std::endl;
    // std::cout << "right motor: " << right << std::endl;
  }
  void on_disconnect() { receiver = ip::address{}; }

public:
  TCPConnector(asio::io_context &ctx, ip::address &receiver)
      : acceptor{ctx}, receiver{receiver} {
    accept();
  }

  void operator()(asio::error_code ec, tcp::socket socket) {
    if (!ec) {
      on_connect();

      receiver = socket.remote_endpoint().address();

      while (true) {
        float data[2];
        asio::error_code ec;
        socket.read_some(asio::buffer(data), ec);

        if (!ec) {
          on_receive(data[0], data[1]);
        } else {
          on_disconnect();
          accept();
          break;
        }
      }
    }
  }
};
template <typename TransmissionGenerator> class UDPTransmitter {
  ip::address &receiver;
  udp::socket socket;
  const uint16_t port;
  TransmissionGenerator generator;

  void transmit() {
    socket.async_send_to(generator(), {receiver, port},
                         [&](asio::error_code ec, std::size_t b) {
                           if (ec)
                             std::cerr << " ec: " << ec.message() << std::endl;
                           transmit();
                         });
  }

public:
  UDPTransmitter(asio::io_context &ctx, ip::address &receiver, uint16_t port,
                 TransmissionGenerator &&generator)
      : receiver{receiver}, socket{ctx}, port{port}, generator{
                                                         std::move(generator)} {
    socket.open(udp::v4());
    socket.set_option(asio::socket_base::broadcast(true));
    transmit();
  }
};

struct Message {
  float water_temperature;
  float turbidity;
  float dust;
  float battery_voltage;
  float pressure;
  float temperature;
  float humidity;
};

int main() {
  asio::io_context ctx;
  NFD::Init();
  ImageLoader loader{4200, show_pick_folder_dialog()};

  ip::address receiver;
  TCPConnector connector{ctx, receiver};

  UDPTransmitter video_transmitter{
      ctx, receiver, VIDEO_UDP_PORT,
      [frame_idx = 0, &loader, image = ImageStorage{1385, 1080},
       compressed = ImageCompressedStorage{1385, 1080}]() mutable {
        frame_idx = loader.load_next_frame(image);
        for (int quality = 50; !compress_image(compressed, image, quality);)
          quality /= 2;
        return std::array{
            asio::buffer(&frame_idx, sizeof(frame_idx)),
            asio::buffer(compressed.data.get(), compressed.stored_size)};
      }};
  UDPTransmitter sensor_transmitter{
      ctx, receiver, SENSOR_UDP_PORT,
      [rand_val = 0.0f, type = 0, now = std::chrono::steady_clock::time_point{},
       message = Message{}]() mutable {
        const float val = rand() / static_cast<float>(RAND_MAX);
        rand_val = val;
        message.water_temperature = val;
        message.turbidity = val;
        message.dust = val;
        message.battery_voltage = val;
        message.pressure = val;
        message.temperature = val;
        message.humidity = val;
        return asio::buffer(&message, sizeof(message));
      }};

  std::thread worker1{[&] { ctx.run(); }};
  ctx.run();

  return 0;
}
