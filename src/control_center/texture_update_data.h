#ifndef TEXTURE_UPDATE_DATA_H
#define TEXTURE_UPDATE_DATA_H

#include <asio.hpp>
#include <jpgd.h>

#include "tripplebuffer.h"

struct ReceivedPixel
{
  char r, g, b;
};
class TextureUpdateData
{
private:
  const size_t width, height;
  std::unique_ptr<Pixel[]> decompressed;
  const size_t compressed_data_cap;
  struct CompressedImage
  {
    std::unique_ptr<jpgd::uint8[]> data;
    size_t size;
  };
  TrippleBuffer<CompressedImage>::Storage compressed_data_storage;
  TrippleBuffer<CompressedImage> compressed_data;

  void decompress(const CompressedImage &compressed)
  {
    int w, h, comps;
    jpgd::uint8 *pDecompressed = jpgd::decompress_jpeg_image_from_memory(
        compressed.data.get(), compressed.size, &w, &h, &comps, 3);
    if (!pDecompressed || w != width || h != height || comps != 3)
    {
      free(pDecompressed);
      std::cout << "Received incompatible texture";
      return;
    }
    auto rcvd = reinterpret_cast<ReceivedPixel *>(pDecompressed);

    for (int i = 0; i < w * h; ++i)
    {
      decompressed[i].r = rcvd[i].r;
      decompressed[i].g = rcvd[i].g;
      decompressed[i].b = rcvd[i].b;
    }

    free(pDecompressed); // TODO: avoid copy
  }

public:
  TextureUpdateData(const Texture &tex)
      : width{tex.width()}, height{tex.height()},
        decompressed{std::make_unique<Pixel[]>(width * height)},
        compressed_data_cap{width * height * 3},
        compressed_data_storage{{std::make_unique<jpgd::uint8[]>(compressed_data_cap), 0}, {std::make_unique<jpgd::uint8[]>(compressed_data_cap), 0}, {std::make_unique<jpgd::uint8[]>(compressed_data_cap), 0}},
        compressed_data{compressed_data_storage}
  {
    for (int i = 0; i < width * height; ++i)
    {
      decompressed[i] = {255, 0, 255, 255};
    }
  }
  Pixel *data()
  {
    compressed_data.swap_front();
    if (compressed_data.get_front_buffer().size)
      decompress(compressed_data.get_front_buffer());
    compressed_data.get_front_buffer().size = 0;
    return decompressed.get();
  }
  auto begin_receiving_data()
  {
    return asio::buffer(compressed_data.get_back_buffer().data.get(), compressed_data_cap);
  }
  void end_receiving_data(size_t compressed_bytes)
  {
    compressed_data.get_back_buffer().size = compressed_bytes;
    compressed_data.swap_back();
  }
};

#endif
