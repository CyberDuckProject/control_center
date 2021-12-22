#ifndef TRANSMITTER_H
#define TRANSMITTER_H

#include <asio.hpp>

#include "address.h"

class Transmitter {
  using tcp = asio::ip::tcp;
  tcp::resolver resolver;
  tcp::socket socket;

public:
  Transmitter(asio::io_context &ctx) : resolver{ctx}, socket{ctx} {}

  template <typename F>
  void async_connect(std::string_view host, std::string_view service,
                     F &&handler) {
    resolver.cancel();
    resolver.async_resolve(
        host, service,
        [&](const asio::error_code &ec, tcp::resolver::results_type results) {
          if (!ec) {
            asio::async_connect(socket, results, handler);
          } else {
            handler(ec, tcp::endpoint{});
          }
        });
  }
  Address remote_address() const { return Address{socket.remote_endpoint()}; }
  template <typename T, typename F> void async_send(const T &pod, F &&handler) {
    asio::async_write(socket, asio::buffer(&pod, sizeof(pod)), handler);
  }
};

#endif