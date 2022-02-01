#ifndef RECEIVING_LOOP_H
#define RECEIVING_LOOP_H

#include <asio.hpp>

template <typename F1, typename F2>
class ReceivingLoop
{
private:
  using udp = asio::ip::udp;
  udp::socket socket;
  udp::endpoint remote;
  F1 receive_buffer_generator;
  F2 on_receive;

public:
  ReceivingLoop(udp::socket &&socket, F1 &&receive_buffer_generator, F2 &&on_receive)
      : socket{std::move(socket)}, receive_buffer_generator{std::move(receive_buffer_generator)}, on_receive{std::move(on_receive)}
  {
    (*this)();
  }
  void operator()()
  {
    socket.async_receive_from(receive_buffer_generator(), remote, [this](asio::error_code ec, std::size_t bytes_received)
                              {
                                on_receive(ec, bytes_received, remote);
                                (*this)();
                              });
  }
};

#endif
