#include "io.h"
#include <asio.hpp>
#include <iostream>

using tcp = asio::ip::tcp;
using udp = asio::ip::udp;

asio::io_context ctx;

struct Pixel {
  char r, g, b;
};
const int w = 1280;
const int h = 720;
int row = 0;
auto pixels = std::make_unique<Pixel[]>(w * h);
static int t = 0;

bool in(int i) {
  int c = i % w;
  int r = i / w;

  return (c + t) % 50 < 25 && (r + t) % 50 < 25;
}

void update_texture() {
  for (int i = 0; i < w * h; ++i) {
    if (in(i)) {
      pixels[i].r = 255;
      pixels[i].g = 255;
      pixels[i].b = 255;
    } else {
      pixels[i].r = 0;
      pixels[i].g = 0;
      pixels[i].b = 0;
    }
  }
  ++t;
}

udp::endpoint receiver;
void send_texture() {
  static udp::socket socket = []() {
    udp::socket socket{ctx};
    socket.open(udp::v6());
    return socket;
  }();

  if (row == 0)
    update_texture();

  socket.async_send_to(std::array{asio::buffer(&row, sizeof(row)),
                                  asio::buffer(pixels.get() + w * row, w * 3)},
                       receiver, [](asio::error_code ec, std::size_t b) {
                         // std::cout << "sent " << b << ", ec: " <<
                         // ec.message()
                         //           << '\n';
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