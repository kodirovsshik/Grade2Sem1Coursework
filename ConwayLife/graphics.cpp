
#include "graphics.hpp"



void framebuffer_t::fill(ksn::color_bgr_t filler)
{
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

void framebuffer_t::draw_rect(ksn::vec2f downleft, ksn::vec2f topright)
{
	const auto& size = this->m_parent->get_size();

	auto transform = [this]
	(ksn::vec2f v) -> ksn::vec2f
	{
		for (auto& x : v.data)
			x = roundf(x);
		return ksn::clamp(v, { 0, 0 }, ksn::vec2f(this->get_parent()->get_size() - ksn::vec2i{ 1, 1 }));
	};

	//TODO: finish
}

const ksn::color_bgr_t* framebuffer_t::get_data() const noexcept
{
	return this->m_screen_data.data();
}



void swapchain_t::create(uint16_t width, uint16_t height, size_t frames)
{
	this->m_frames.resize(frames);
	size_t total_screen_area = width * height;

	for (auto& frame : this->m_frames)
	{
		frame.m_parent = this;
		frame.m_screen_data.resize(total_screen_area);
		frame.m_semaphore.release();
	}

	this->m_size = { width, height };
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
ksn::vec2i swapchain_t::get_size() const noexcept
{
	return this->m_size;
}
