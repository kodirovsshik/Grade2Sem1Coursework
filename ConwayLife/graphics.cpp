
#include "graphics.hpp"



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

void framebuffer_t::draw_rect(ksn::vec2f downleft, ksn::vec2f topright, ksn::color_bgr_t color, const view_t* view)
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

	downleft = transform(downleft, true);
	topright = transform(topright, false);

	auto* buffer_data = this->m_screen_data.data();
	auto buffer_size = this->m_parent->get_actual_size();

	for (int y = int(downleft[1]); y < int(topright[1]); ++y)
	{
		for (int x = int(downleft[0]); x < int(topright[0]); ++x)
		{
			buffer_data[(buffer_size[1] - 1 - y) * buffer_size[0] + x] = color;
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
	size_t total_area = width * height;

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
