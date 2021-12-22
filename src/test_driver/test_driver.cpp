#include "io.h"
#include <asio.hpp>
#include <iostream>
#include <jpge.h>

using tcp = asio::ip::tcp;
using udp = asio::ip::udp;

asio::io_context ctx;

struct Pixel {
  char r, g, b;
};
constexpr int w = 480;
constexpr int h = 240;
int row = 0;
auto pixels = std::make_unique<Pixel[]>(w * h);
static int t = 0;

bool in(int i) {
  int c = i % w;
  int r = i / w;

  return (c + t) % 50 < 25 && (r + t) % 50 < 25;
  // return 1;
}

char crand() {
  static char last = 134;
  last = (last + 219) * 13;
  return last;
}

void update_texture() {
  for (int i = 0; i < w * h; ++i) {
    if (in(i)) {
      pixels[i].r = crand();
      pixels[i].g = crand();
      pixels[i].b = crand();
    } else {
      pixels[i].r = 0;
      pixels[i].g = 0;
      pixels[i].b = 0;
    }
  }
  ++t;
}

constexpr size_t UDP_MAX_PACKET_SZ = 65507;
constexpr size_t BUFSZ = w * h * 3;
char compressed_buf[BUFSZ];
size_t compress_texture() {
  int compressed_sz = BUFSZ;
  jpge::compress_image_to_jpeg_file_in_memory(
      compressed_buf, compressed_sz, w, h, 3,
      reinterpret_cast<jpge::uint8 *>(pixels.get()));
  if (!compressed_sz)
    std::cout << "failed to compress texture";
  return compressed_sz;
}

udp::endpoint receiver;
void send_texture() {
  static udp::socket socket = []() {
    udp::socket socket{ctx};
    socket.open(udp::v6());
    return socket;
  }();

  update_texture();

  size_t compressed_sz = compress_texture();

  socket.async_send_to(std::array{asio::buffer(&row, sizeof(row)),
                                  asio::buffer(compressed_buf, compressed_sz)},
                       receiver, [](asio::error_code ec, std::size_t b) {
                         if (ec) {
                           std::cout << "sending error ec: " << ec.message()
                                     << '\n';
                         }
                         send_texture();
                       });
  ++row;
  row %= h;
}

void _accept();

void handle_connection(asio::error_code ec, tcp::socket socket) {
  if (!ec) {
    // TODO: set eyes to steady
    std::cout << "connected to " << socket.remote_endpoint() << '\n';
    while (true) {
      // TODO: this should be done on the main thread
      float data[2];
      asio::error_code ec;
      socket.read_some(asio::buffer(data), ec);

      if (!ec) {
        std::cout << "left motor: " << data[0] << '\n';
        std::cout << "right motor: " << data[1] << '\n';
      } else {
        _accept();
        break;
      }
    }
  }
}

tcp::acceptor *pAcceptor;
void _accept() {
  // TODO: set eyes to flash
  std::cout << "awaiting connection... ";
  pAcceptor->async_accept(handle_connection);
}

int main() {
  tcp::acceptor acceptor{ctx, {tcp::v4(), 13}};
  pAcceptor = &acceptor;
  _accept();

  udp::resolver resolver{ctx};

  resolver.async_resolve(
      "localhost", "1512",
      [](asio::error_code ec, udp::resolver::results_type results) {
        if (!ec) {
          std::cout << "udp resolved... ";
          receiver = results.begin()->endpoint();
          std::cout << receiver << '\n';
          send_texture();
        }
      });

  std::thread worker1{[&] { ctx.run(); }};
  std::thread worker2{[&] { ctx.run(); }};
  ctx.run();

  return 0;
}