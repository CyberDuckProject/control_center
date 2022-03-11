#include <algorithm>
#include <asio.hpp> // network
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <jpge.h> // jpeg compression
#include <memory>
#include <sstream>
#include <stdexcept>
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
  const size_t width, height;
  std::unique_ptr<Pixel[]> data;

  ImageStorage(size_t width, size_t height)
      : width{width}, height{height}, data{std::make_unique<Pixel[]>(width *
                                                                     height)} {}
  ImageStorage(const ImageStorage &other)
      : ImageStorage{other.width, other.height} {}
};
class ImageLoader {
  int current_frame = 0;
  std::istream &in = std::cin;
  std::vector<unsigned char> current_contents;
  static constexpr unsigned char delim[] = {
      0xff, 0xd8, 0xff, 0xfe, 0x00, 0x10, 0x4c, 0x61, 0x76, 0x63,
      0x35, 0x39, 0x2e, 0x31, 0x38, 0x2e, 0x31, 0x30, 0x30};
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
    if (!read_image) {
      std::cerr << std::string("Could not read image. ") + stbi_failure_reason()
                << '\n';
      return;
    }
    if (read_width != out.width || read_height != out.height ||
        read_components != 3)
      throw std::runtime_error(
          "Read image not compatible with the given image storage");

    memcpy(static_cast<void *>(out.data.get()), read_image,
           out.width * out.height * 3);
    stbi_image_free(read_image);
  }

public:
  ImageLoader() {
    in.read((char *)intermediate.get(), bufsz);
    in.read((char *)intermediate_next.get(), bufsz);
  }
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
      : acceptor{ctx, tcp::endpoint{tcp::v4(), MOTOR_TCP_PORT}}, receiver{receiver} {
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

std::pair<size_t, size_t> proccess_cmdline_args(int argc, char **argv) {
  if (argc != 3)
    throw std::runtime_error("Usage: test_driver image_width image_height");
  return {std::stoi(argv[1], nullptr, 10), std::stoi(argv[2], nullptr, 10)};
}

// use with
// ffmpeg -y -f avfoundation -framerate 30 -i "0" -preset ultrafast -r 20 -f
// image2pipe - |
// ./test_driver
int main(int argc, char **argv) {
  try {
    auto [img_width, img_height] = proccess_cmdline_args(argc, argv);

    asio::io_context ctx;
    ImageLoader loader;

    ip::address receiver;
    TCPConnector connector{ctx, receiver};

    UDPTransmitter video_transmitter{
        ctx, receiver, VIDEO_UDP_PORT,
        [frame_idx = 0, &loader, image = ImageStorage{img_width, img_height},
         compressed = ImageCompressedStorage{img_width, img_height}]() mutable {
          frame_idx = loader.load_next_frame(image);
          for (int quality = 50; !compress_image(compressed, image, quality);)
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

  } catch (const std::exception &e) {
    std::cerr << "STREAMER ERROR: " << e.what() << '\n';
    return 1;
  }

  return 0;
}
