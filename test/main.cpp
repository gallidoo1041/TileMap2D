#include "C:\Users\Admin\Documents\usb\C++\modules\TileMap2D.h"
#include "C:\Users\Admin\Documents\usb\C++\modules\png\lodepng.h"

int main()
{
	std::vector<uint8_t> data;
	uint32_t w, h;
	lodepng::decode(data, w, h, "img.png");
	tm2D::TileMap2DView<uint32_t> tmap(data.data(), w, h);
	tmap.fillArea({ 0, 0 }, [](uint32_t color) { return (color & 0xff000000) == 0x00; }, 0xff000000);
	lodepng::encode("save.png", data, w, h);
}