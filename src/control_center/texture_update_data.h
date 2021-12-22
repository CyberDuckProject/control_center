#ifndef TEXTURE_UPDATE_DATA_H
#define TEXTURE_UPDATE_DATA_H

#include <asio.hpp>
#include <jpgd.h>
#include <mutex>

struct ReceivedPixel {
  char r, g, b;
};
class TextureUpdateData {
private:
  const size_t width, height;
  std::unique_ptr<Pixel[]> pixels;
  const size_t compressed_data_cap;
  std::unique_ptr<jpgd::uint8[]> compressed_data;
  size_t compressed_data_sz;

  mutable std::mutex m; // TODO: make this lockless
  // TODO: dedicated decoding thread

  void decompress() {
    int w, h, comps;
    jpgd::uint8 *decompressed = jpgd::decompress_jpeg_image_from_memory(
        compressed_data.get(), compressed_data_sz, &w, &h, &comps, 3);
    if (!decompressed || w != width || h != height || comps != 3) {
      free(decompressed);
      std::cout << "Received incompatible texture";
    }
    auto rcvd = reinterpret_cast<ReceivedPixel *>(decompressed);

    for (int i = 0; i < w * h; ++i) {
      pixels[i].r = rcvd[i].r;
      pixels[i].g = rcvd[i].g;
      pixels[i].b = rcvd[i].b;
    }

    free(decompressed);
  }

public:
  TextureUpdateData(const Texture &tex)
      : width{tex.width()}, height{tex.height()},
        pixels{std::make_unique<Pixel[]>(width * height)},
        compressed_data_cap{width * height * 3},
        compressed_data{std::make_unique<jpgd::uint8[]>(compressed_data_cap)},
        compressed_data_sz{0} {
    for (int i = 0; i < width * height; ++i) {
      pixels[i] = {255, 0, 255, 255};
    }
  }
  Pixel *data() {
    std::scoped_lock lk{m};
    if (compressed_data_sz > 0) {
      decompress();
      compressed_data_sz = 0;
    }
    return pixels.get();
  }
  auto get_compressed_buffer() {
    std::scoped_lock lk{m};
    return asio::buffer(compressed_data.get(), compressed_data_cap);
  }
  void set_recieved_data(size_t uncompressed_bytes) {
    compressed_data_sz = uncompressed_bytes;
  }
};

#endif
