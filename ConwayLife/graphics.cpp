
#include "graphics.hpp"


ksn::color_bgr_t alpha_mix(ksn::color_bgr_t c1, ksn::color_bgra_t c2)
{
	ksn::vec3f v1{ c1.b, c1.g, c1.r };
	ksn::vec3f v2{ c2.b, c2.g, c2.r };
	ksn::vec3f result;

	float t = c2.a / 255.f;
	result = v2 * t + v1 * (1 - t);

	return ksn::color_bgr_t{ uint8_t(result[2] + 0.5f), uint8_t(result[1] + 0.5f), uint8_t(result[0] + 0.5f) };
}


void framebuffer_t::fill(ksn::color_bgr_t filler)
{
	if (filler.b == filler.g && filler.b == filler.r && false)
		memset(this->m_screen_data.data(), filler.b, this->m_screen_data.size() * sizeof(this->m_screen_data[0]));
	else
		std::ranges::fill(this->m_screen_data, filler);
}

void framebuffer_t::clear()
{
	memset(
		this->m_screen_data.data(),
		0,
		this->m_screen_data.size() * sizeof(this->m_screen_data[0])
	);
}

void framebuffer_t::release()
{
	this->m_semaphore.release();
}

void framebuffer_t::draw_image(ksn::vec2f downleft, const ksn::image_bgra_t& image, const view_t* view)
{
	const auto& screen_size = this->m_parent->get_screen_size();

	auto transform = [&]
	(ksn::vec2f v, bool inclusive) -> ksn::vec2f
	{
		if (view)
			v = view->map_w2s(v);

		ksn::vec2f top;
		if (inclusive)
			top = ksn::vec2f(screen_size - ksn::vec2i{ 1, 1 });
		else
			top = ksn::vec2f(screen_size);

		v = ksn::clamp(v, { 0, 0 }, top);
		v += ksn::vec2f{ 0.5f, 0.5f };
		return v;
	};

	ksn::vec2f topright = transform(downleft + ksn::vec2f{ image.width, image.height }, false);
	downleft = transform(downleft, true);

	auto* buffer_data = this->m_screen_data.data();
	auto buffer_size = this->m_parent->get_actual_size();

	int x0 = int(downleft[0]);
	int y0 = int(downleft[1]);

	for (int y = y0; y < int(topright[1]); ++y)
	{
		for (int x = x0; x < int(topright[0]); ++x)
		{
			ksn::color_bgr_t& dst = buffer_data[(buffer_size[1] - 1 - y) * buffer_size[0] + x];

			int image_x = x - x0;
			int image_y = image.height - 1 - (y - y0);

			dst = alpha_mix(dst, image.m_data[image_y * image.width + image_x]);
		}
	}
}
void framebuffer_t::draw_rect(ksn::vec2f downleft, ksn::vec2f topright, ksn::color_bgra_t color, const view_t* view)
{
	const auto& screen_size = this->m_parent->get_screen_size();

	auto transform = [&]
	(ksn::vec2f v, bool inclusive) -> ksn::vec2f
	{
		if (view)
			v = view->map_w2s(v);

		ksn::vec2f top;
		if (inclusive)
			top = ksn::vec2f(screen_size - ksn::vec2i{ 1, 1 });
		else
			top = ksn::vec2f(screen_size);

		v = ksn::clamp(v, { 0, 0 }, top);
		v += ksn::vec2f{ 0.5f, 0.5f };
		return v;
	};

	downleft = transform(downleft, false);
	topright = transform(topright, false);

	auto* buffer_data = this->m_screen_data.data();
	auto buffer_size = this->m_parent->get_actual_size();

	for (int y = int(downleft[1]); y < int(topright[1]); ++y)
	{
		for (int x = int(downleft[0]); x < int(topright[0]); ++x)
		{
			auto& dst = buffer_data[(buffer_size[1] - 1 - y) * buffer_size[0] + x];
			dst = alpha_mix(dst, color);
		}
	}
}

const ksn::color_bgr_t* framebuffer_t::get_data() const noexcept
{
	return this->m_screen_data.data();
}



void swapchain_t::create(uint16_t width, uint16_t height, size_t frames)
{
	this->m_frames.resize(frames);
	this->m_screen_size = { width, height };
	
	static constexpr size_t row_byte_alignment = 4;
	static constexpr size_t alignment = row_byte_alignment / std::gcd(row_byte_alignment, sizeof(this->m_frames[0].get_data()[0]));
	
	width = ksn::align_up<uint16_t>(width, alignment);

	this->m_size = { width, height };
	size_t total_area = (size_t)width * height;

	for (auto& frame : this->m_frames)
	{
		frame.m_parent = this;
		frame.m_screen_data.resize(total_area);
		frame.m_semaphore.release();
	}

	this->frame_index = -1;
}

framebuffer_t* swapchain_t::acquire_frame() noexcept
{
	if (this->m_frames.size() == 0)
		return nullptr;

	if (++this->frame_index == this->m_frames.size())
		this->frame_index = 0;

	auto* result = &this->m_frames[this->frame_index];
	result->m_semaphore.acquire();
	return result;
}

swapchain_t* framebuffer_t::get_parent() const noexcept
{
	return this->m_parent;
}
ksn::vec2i swapchain_t::get_screen_size() const noexcept
{
	return this->m_screen_size;
}
ksn::vec2i swapchain_t::get_actual_size() const noexcept
{
	return this->m_size;
}


view_t::view_t() noexcept
	: m_origin{}, m_ratio(0)
{
}
view_t::view_t(ksn::vec2f origin, float ratio) noexcept
	: m_origin(origin), m_ratio(ratio)
{
}

view_t::view_t(ksn::vec2f center, float ratio, ksn::vec2i screen_size) noexcept
{
	this->view_t::view_t(center - (ksn::vec2f)screen_size * 0.5f / ratio, ratio);
}

view_t::view_t(ksn::vec2f origin, float view_dim, int screen_dim) noexcept
	: m_origin(origin), m_ratio(screen_dim / view_dim)
{
}

view_t::view_t(ksn::vec2f origin, ksn::vec2f view_min_size, ksn::vec2i screen_size) noexcept
	: m_origin(origin)
{
	float ratio_x = screen_size[0] / view_min_size[0];
	float ratio_y = screen_size[1] / view_min_size[1];

	this->m_ratio = std::max(ratio_x, ratio_y);
}

view_t::view_t(center_t, ksn::vec2f center, ksn::vec2f view_min_size, ksn::vec2i screen_size) noexcept
{
	float ratio_x = screen_size[0] / view_min_size[0];
	float ratio_y = screen_size[1] / view_min_size[1];

	this->m_ratio = std::max(ratio_x, ratio_y);
	this->m_origin = center - (ksn::vec2f)screen_size * 0.5f / this->m_ratio;
}

ksn::vec2f view_t::map_w2s(ksn::vec2f world_coordinates) const noexcept
{
	return (world_coordinates - this->m_origin) * this->m_ratio;
}

ksn::vec2f view_t::map_s2w(ksn::vec2f screen_coordinates) const noexcept
{
	return screen_coordinates / this->m_ratio + this->m_origin;
}

void view_t::zoom_in_s(ksn::vec2f screen_coordinates, float factor) noexcept
{
	this->zoom_in_w(this->map_s2w(screen_coordinates), factor);
}

void view_t::zoom_in_w(ksn::vec2f world_coordinates, float factor) noexcept
{
	ksn::vec2f shift = world_coordinates - this->m_origin;
	this->m_ratio /= factor;
	this->m_origin = world_coordinates - shift * factor;
}

void view_t::shiht_by_w(ksn::vec2f world_dpos) noexcept
{
	this->m_origin += world_dpos;
}

void view_t::shiht_by_s(ksn::vec2f screen_dpos) noexcept
{
	this->shiht_by_w(screen_dpos / this->m_ratio);
}
