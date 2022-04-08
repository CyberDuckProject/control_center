#ifndef ADDRESS_H
#define ADDRESS_H

#include <asio.hpp>
#include <ostream>

struct Address {
  explicit Address(const asio::ip::tcp::endpoint &endpoint) {
    const auto address = endpoint.address().to_v4().to_bytes();
    ip[0] = address[0];
    ip[1] = address[1];
    ip[2] = address[2];
    ip[3] = address[3];
    port = endpoint.port();
  }

  int ip[4];
  int port;
};

constexpr const char *const MOTOR_TCP_PORT = "1333";
constexpr int VIDEO_UDP_PORT = 1512;
constexpr int SENSOR_UDP_PORT = 1666;

inline std::ostream &operator<<(std::ostream &out, const Address &adr) {
  out << adr.ip[0] << '.';
  out << adr.ip[1] << '.';
  out << adr.ip[2] << '.';
  out << adr.ip[3] << ':';
  out << adr.port;
  return out;
}

#endif
