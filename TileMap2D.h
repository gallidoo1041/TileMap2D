#pragma once

#include <vector>
#include <functional>
#include <deque>
#include <array>

// 2-dimensional Tilemap structure (for 2-dimensional tilemaps in games and image processing) implementations and functions.

// All 2-dimensional tilemap types here have a template tile type 'T' that can be default-constructed, has contiguous and direct memory access, with 1-dimensional data starting from the leftmost and first element of the top and first row, going from left to right.

namespace tm2D
{

// Coordinates of a 2-dimensional tilemap.
struct Point
{
	size_t x = 0;
	size_t y = 0;

	constexpr bool operator==(const Point&) const = default;
};

// An area of a 2-dimensional tilemap, starting from the top left.
struct Rect
{
	size_t x = 0;
	size_t y = 0;
	size_t width = 0;
	size_t height = 0;

	constexpr bool operator==(const Rect& r) const = default;

	// Check if 2 rectangles intersect. 
	constexpr bool intersects(const Rect& r) const
	{
		return
			(x + width > r.x) && (x < r.x + r.width) &&
			(y + height > r.y) && (y < r.y + r.height);
	}

	// Get the intersection area of 2 rectangles.
	// Returns:
	// Empty rectangle (Rect()) if the 2 rectangles don't intersect.
	Rect intersection(const Rect& r) const
	{
		Rect res;
		if (intersects(r)) {
			res.x = std::max<>(x, r.x);
			res.y = std::max<>(y, r.y);
			res.width = std::min<>(x + width, r.x + r.width) - res.x;
			res.height = std::min<>(y + height, r.y + r.height) - res.y;
		}
		return res;
	}
};

// Implementation class for TileMap2D types.
template<typename T>
struct TileMap2DImpl
{
	// Get width of tilemap.
	virtual size_t width() const = 0;
	// Get height of tilemap.
	virtual size_t height() const = 0;

	// Get tile directly (does not check for bounds).
	virtual T& operator()(size_t x, size_t y) = 0;
	// Get const reference to tile directly (does not check for bounds).
	virtual const T& operator()(size_t x, size_t y) const = 0;

	// Get tile, with bounds checking. If out of bounds, return default-constructed value.
	virtual T get(size_t x, size_t y) const
	{
		return (x < width() && y < height()) ? operator()(x, y) : T{};
	}
	// Set tile, with bounds checking. If out of bounds, do nothing.
	virtual void set(size_t x, size_t y, const T& t)
	{
		if (x < width() && y < height()) operator()(x, y) = t;
	}

	// Filp the tilemap.
	// Note: calling this function with both parameters set to 'true' is equal to calling rot90() twice in the same direction.
	void flip(bool horizontal, bool vertical)
	{
		if (horizontal) {
			for (size_t x = 0; x < width() / 2; x++)
				for (size_t y = 0; y < height(); y++)
					std::swap(operator()(x, y), operator()(width() - 1 - x, y));
		}
		if (vertical) {
			for (size_t y = 0; y < height() / 2; y++)
				for (size_t x = 0; x < width(); x++)
					std::swap(operator()(x, y), operator()(x, height() - 1 - y));
		}
	}

	// Draw a line from [p1] to [p2] on a tilemap.
	// Params:
	//   [drawfunc] Draw function that takes a tilemap, x and y coordinates as parameters.
	void drawLine(
		const Point& p1,
		const Point& p2,
		const std::function<void(TileMap2DImpl*, size_t, size_t)>& drawfunc
	) {
		// Get the longer distance to use in the draw loop.
		Point delta(
			(p1.x < p2.x) ? (p2.x - p1.x) : (p1.x - p2.x),
			(p1.y < p2.y) ? (p2.y - p1.y) : (p1.y - p2.y)
		);

		// Starting point.
		double current_x = p1.x, current_y = p1.y;
		double step_x, step_y;

		if (delta.x > delta.y) {
			step_x = (p2.x - p1.x) / (double)delta.x;
			step_y = (p2.y - p1.y) / (double)delta.x;

			for (size_t x = 0; x <= delta.x; x++) {
				drawfunc(this, round(current_x), round(current_y));
				current_x += step_x;
				current_y += step_y;
			}
		}
		else {
			step_x = (p2.x - p1.x) / (double)delta.y;
			step_y = (p2.y - p1.y) / (double)delta.y;

			for (size_t y = 0; y <= delta.y; y++) {
				drawfunc(this, round(current_x), round(current_y));
				current_x += step_x;
				current_y += step_y;
			}
		}
	}

	// Fill a polygonal area of elements sastifying [rule] with [elem].
	void fillArea(
		const Point& center,
		const std::function<bool(const T&)>& rule,
		const T& elem
	) {
		std::deque<Point> positions;

		if (center.x < width() && center.y < height()) {
			positions.push_back(center);
		}
		
		while (!positions.empty()) {
			// Get the current iteration's size to remove this iteration and get the next iteration in tandem.
			size_t iteration_elem_size = positions.size();

			for (size_t i = 0; i < iteration_elem_size; i++) {
				Point current = *positions.begin();

				operator()(current.x, current.y) = elem;

				std::array<Point, 4> adjs({
					{ current.x - 1, current.y },
					{ current.x + 1, current.y },
					{ current.x, current.y - 1 },
					{ current.x, current.y + 1 }
				});

				for (const Point& adj : adjs) {
					// Ignore out-of-bounds positions and elements that satisfy [rule].
					if ((adj.x < width() && adj.y < height()) && rule(operator()(adj.x, adj.y))) {
						positions.push_back(adj);

						// Set the adjacent pixels also to avoid duplicates.
						Point current_nextitr = *positions.rbegin();

						operator()(current_nextitr.x, current_nextitr.y) = elem;
					}
				}

				positions.pop_front();
			}
		}
	}
};

// Implementation class for resizable TileMap2D types.
template<typename T>
struct ResizableTileMap2DImpl: public TileMap2DImpl<T>
{
	// Initialize the tilemap with a new buffer and fill the new space with [padding].
	virtual void reset(size_t new_width, size_t new_height, const T& padding = {}) = 0;
};

// A 2-dimensional tilemap view of a contiguous memory buffer that can be used for providing 2-dimensional access to large 1-dimensional image data without having to copy it into a 2-dimensional container.
template<typename T>
struct TileMap2DView: public TileMap2DImpl<T>
{
	TileMap2DView() {}

	TileMap2DView(void* data, size_t width, size_t height)
		: _data((T*)data), _width(width), _height(height) {}

	constexpr size_t width() const { return _width; }
	constexpr size_t height() const { return _height; }

	constexpr T& operator()(size_t x, size_t y) { return _data[x + _width * y]; }

	constexpr const T& operator()(size_t x, size_t y) const { return _data[x + _width * y]; }

	constexpr T* data() const { return _data; }

private:
	T* _data = NULL;
	size_t _width = 0;
	size_t _height = 0;
};

// A 2-dimensional tilemap with a contiguous 1-dimensional memory buffer.
template<typename T>
struct TileMap2D_1D: public ResizableTileMap2DImpl<T>
{
	TileMap2D_1D() {}

	// Initialize by row-major initializer list as 2D array.
	TileMap2D_1D(std::initializer_list<T> arr, size_t _width, size_t _height)
		: _width(_width), _height(_height)
	{
		_data.resize(_width * _height);
		for (size_t i = 0; i < std::min<>(arr.size(), _data.size()); i++)
			_data[i] = *(arr.begin() + i);
	}

	// Copies the underlying content of a TileMap2DView.
	TileMap2D_1D(const TileMap2DView<T>& view)
		: _width(view.width()), _height(view.height())
	{
		_data.resize(_width * _height);
		std::copy(view.data(), view.data() + _data.size(), _data.begin());
	}

	TileMap2D_1D(size_t _width, size_t _height, const T& elem = {})
		: _width(_width), _height(_height)
	{
		_data.resize(_width * _height, elem);
	}

	constexpr size_t width() const { return _width; }
	constexpr size_t height() const { return _height; }

	constexpr T& operator()(size_t x, size_t y) { return _data[x + _width * y]; }

	constexpr const T& operator()(size_t x, size_t y) const { return _data[x + _width * y]; }

	void reset(size_t new_width, size_t new_height, const T& padding = {})
	{
		_width = new_width;
		_height = new_height;
		_data.clear();
		_data.resize(new_width * new_height, padding);
	}

	// Get the pointer to the underlying data.
	constexpr T* data() { return _data.data(); }

private:
	std::vector<T> _data = {};
	size_t _width = 0;
	size_t _height = 0;
};

// Get a chunk of a 2D tilemap with the size of [src_area] and return in [output_map].
// This can also be used to 'resize' tilemap, by setting [src_area]'s position to (0; 0).
template<typename T>
void getChunk(
	ResizableTileMap2DImpl<T>* output,
	const TileMap2DImpl<T>* input,
	const Rect& src_area
) {
	output->reset(src_area.width, src_area.height);
	const Rect src_cliprect = src_area.intersection({ 0, 0, input->width(), input->height() });

	for (size_t _x = 0; _x < src_cliprect.width; _x++)
		for (size_t _y = 0; _y < src_cliprect.height; _y++)
			(*output)(_x, _y) = (*input)(_x + src_cliprect.x, _y + src_cliprect.y);
}

// Set a chunk of a 2D tilemap.
// Parameters:
//   [src_area]: Source chunk area to get from. Default value is the whole [src_chunk] area.
template<typename T>
void setChunk(
	TileMap2DImpl<T>* output,
	const TileMap2DImpl<T>* input,
	size_t x,
	size_t y,
	Rect src_area = {}
) {
	if (src_area == Rect(0, 0, 0, 0))
		src_area = { 0, 0, input->width(), input->height() };

	const Rect
		src_cliprect = src_area.intersection({ 0, 0, input->width(), input->height() }),
		dst_cliprect = Rect(x, y, src_cliprect.width, src_cliprect.height).intersection({ 0, 0, output->width(), output->height() });

	for (size_t _x = 0; _x < dst_cliprect.width; _x++)
		for (size_t _y = 0; _y < dst_cliprect.height; _y++)
			(*output)(x + dst_cliprect.x, y + dst_cliprect.y) = (*input)(_x + src_cliprect.x, _y + src_cliprect.y);
}

// Rotate the tilemap 90 degrees in the top left corner.
// Params:
//   [rotate_left] If true, rotate to the left, otherwise rotate to the right.
template<typename T>
void rot90(
	ResizableTileMap2DImpl<T>* output,
	const TileMap2DImpl<T>* input,
	bool rotate_left
) {
	output->reset(input->height(), input->width());

	if (rotate_left) {
		for (size_t x = 0; x < output->width(); x++) {
			for (size_t y = 0; y < output->height(); y++) {
				(*output)(x, y) = (*input)(output->height() - y - 1, x);
			}
		}
	}
	else {
		for (size_t x = 0; x < output->width(); x++) {
			for (size_t y = 0; y < output->height(); y++) {
				(*output)(x, y) = (*input)(y, output->width() - x - 1);
			}
		}
	}
}

}; // |===|   END namespace tm2D   |===|