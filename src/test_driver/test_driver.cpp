#include <algorithm>
#include <asio.hpp> // network
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <ios>
#include <iostream>
#include <istream>
#include <jpge.h> // jpeg compression
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h> // loading images from disk

namespace ip = asio::ip;
using tcp = ip::tcp;
using udp = ip::udp;

struct Pixel {
  unsigned char r, g, b;
};
struct ImageStorage {
  size_t width, height;
  std::unique_ptr<Pixel[]> data;

  ImageStorage(size_t width = 0, size_t height = 0)
      : width{width}, height{height}, data{std::make_unique<Pixel[]>(width *
                                                                     height)} {}
  ImageStorage(const ImageStorage &other)
      : ImageStorage{other.width, other.height} {}

  ImageStorage& operator=(ImageStorage &&) = default;
};
class ImageLoader {
  int current_frame = 0;
  std::istream &in;
  std::vector<unsigned char> current_contents;
  static constexpr unsigned char delim[] = {
      0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4a, 0x46, 0x49, 0x46,
      0x00, 0x01, 0x02, 0x00, 0x00, 0x01, 0x00, 0x01, 0x00};
  
  static constexpr size_t delim_sz = sizeof(delim);

  static constexpr size_t bufsz = 4096;
  std::unique_ptr<unsigned char[]> intermediate =
      std::make_unique<unsigned char[]>(bufsz);
  std::unique_ptr<unsigned char[]> intermediate_next =
      std::make_unique<unsigned char[]>(bufsz);
  int i = 0;

  void process_contents(ImageStorage &out) {
    int read_width, read_height, read_components;
    auto read_image =
        stbi_load_from_memory(current_contents.data(), current_contents.size(),
                              &read_width, &read_height, &read_components, 3);
    if (!read_image || read_components != 3) {
      std::cerr << std::string("Could not read image. ") + stbi_failure_reason()
                << '\n';
      return;
    }
    if (read_width != out.width || read_height != out.height)
      out = ImageStorage(read_width, read_height);

    memcpy(static_cast<void *>(out.data.get()), read_image,
           out.width * out.height * 3);
    stbi_image_free(read_image);
  }

public:
  ImageLoader(std::istream& in) : in{in} {
    in.read((char *)intermediate.get(), bufsz);
    in.read((char *)intermediate_next.get(), bufsz);
  };
  size_t load_next_frame(ImageStorage &out) {
    for (; true;) {
      if (i == bufsz) {
        swap(intermediate, intermediate_next);
        in.read((char *)intermediate_next.get(), bufsz);
        i = 0;
        continue;
      }
      if (intermediate[i] == delim[0]) {
        bool complete_match = true;
        bool incomplete_match = false;
        for (int j = i + 1; j < i + delim_sz; ++j) {
          if (j == bufsz) {
            complete_match = false;
            incomplete_match = true;
            break;
          }
          if (intermediate[j] != delim[j - i]) {
            complete_match = false;
            break;
          }
        }
        if (complete_match) {
          process_contents(out);
          current_contents.clear();
          current_contents.insert(current_contents.end(),
                                  intermediate.get() + i,
                                  intermediate.get() + i + delim_sz);
          i = i + delim_sz;
          return ++current_frame;
        }
        if (incomplete_match) {
          size_t match_continue_pos = bufsz - i;
          bool match = true;
          for (int j = 0; j < delim_sz - match_continue_pos; ++j) {
            if (intermediate_next[j] != delim[match_continue_pos + j])
              match = false;
            break;
          }
          if (match) {
            process_contents(out);
            current_contents.clear();
            current_contents.insert(current_contents.end(),
                                    intermediate.get() + i,
                                    intermediate.get() + i + delim_sz);
            i = delim_sz - match_continue_pos;
            swap(intermediate, intermediate_next);
            in.read((char *)intermediate_next.get(), bufsz);
            return ++current_frame;
          }
        }
      }
      current_contents.push_back(intermediate[i]);
      ++i;
    }
  }
};
constexpr size_t MAX_COMPRESSED_IMG_SZ = 65001;
struct ImageCompressedStorage {
  size_t capacity;
  int stored_size;
  std::unique_ptr<char[]> data;
  int width, height;

  ImageCompressedStorage(size_t width = 0, size_t height = 0)
      : capacity{std::min(width * height * 3, MAX_COMPRESSED_IMG_SZ)},
        stored_size{0}, data{std::make_unique<char[]>(capacity)}, width(width), height(height) {}
  ImageCompressedStorage(const ImageCompressedStorage &other)
      : ImageCompressedStorage{1, other.capacity / 3} {}

  ImageCompressedStorage& operator=(ImageCompressedStorage&&) = default;

};
bool compress_image(ImageCompressedStorage &out, const ImageStorage &image,
                    size_t quality) {
  if (out.width != image.width || out.height != image.height)
    out = ImageCompressedStorage(image.width, image.height);
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
    std::cout << "Connected to remote: " << receiver << '\n';
  }
  void on_receive(float left, float right) {
    // TODO: change motor speed
    // std::cout << "left motor: " << left << std::endl;
    // std::cout << "right motor: " << right << std::endl;
  }
  void on_disconnect() {
    receiver = ip::address{};
    std::cout << "Disconnected from remote\n";
  }

public:
  TCPConnector(asio::io_context &ctx, ip::address &receiver)
      : acceptor{ctx, tcp::endpoint{tcp::v4(), MOTOR_TCP_PORT}}, receiver{
                                                                     receiver} {
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
    socket.async_send_to(
        generator(), {receiver, port}, [&](asio::error_code ec, std::size_t b) {
          if (ec)
            std::cerr << " ec: " << ec << " " << ec.message() << std::endl;
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

// use with
// ffmpeg -y -f avfoundation -framerate 30 -i "0" -preset ultrafast -r 20 -f
// image2pipe - |
// ./test_driver
int main(int argc, char **argv) {
  try {
    asio::io_context ctx;

    auto in = (argc == 1 ? std::nullopt : std::optional<std::ifstream>{std::in_place, argv[1], std::ios_base::binary});
    if (in.has_value() && !*in)
      throw std::runtime_error("Could not open file");
    ImageLoader loader{[&]()->std::istream&{ if(in.has_value()) return *in; else return std::cin;  }()};

    ip::address receiver;
    TCPConnector connector{ctx, receiver};

    UDPTransmitter video_transmitter{
        ctx, receiver, VIDEO_UDP_PORT,
        [frame_idx = 0, &loader, image = ImageStorage{},
         compressed = ImageCompressedStorage{}]() mutable {
          frame_idx = loader.load_next_frame(image);
          for (int quality = 50; !compress_image(compressed, image, quality);)
            if (quality == 0) {
                std::cerr << "Could not compress frame! Skipping!\n";
                break;            
              }
            else
              quality /= 2;
          return std::array{
              asio::buffer(&frame_idx, sizeof(frame_idx)),
              asio::buffer(compressed.data.get(), compressed.stored_size)};
        }};
    UDPTransmitter sensor_transmitter{
        ctx, receiver, SENSOR_UDP_PORT,
        [rand_val = 0.0f, type = 0,
         now = std::chrono::steady_clock::time_point{},
         message = Message{}]() mutable {
          std::this_thread::sleep_for(std::chrono::seconds(1));
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
    std::thread worker2{[&] { ctx.run(); }};
    std::thread worker3{[&] { ctx.run(); }};
    ctx.run();
    worker1.join();
    worker2.join();
    worker3.join();
  } catch (const std::exception &e) {
    std::cerr << "STREAMER ERROR: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
