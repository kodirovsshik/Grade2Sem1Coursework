
#ifndef _LOCAL_GRAPHICS_HPP_
#define _LOCAL_GRAPHICS_HPP_



#include <vector>

#include "auxillary.hpp"

#include <ksn/color.hpp>
#include <ksn/math_vec.hpp>
#include <ksn/window.hpp>



class view_t
{
	ksn::vec2f m_origin;
	float ratio = 0;


public:

	ksn::vec2f map_w2s(ksn::vec2f world_coordinates) const noexcept
	{
		return (world_coordinates - this->m_origin) * ratio;
	}
	ksn::vec2f map_s2w(ksn::vec2f world_coordinates) const noexcept
	{
		return world_coordinates / ratio + this->m_origin;
	}

	void zoom_in_s(ksn::vec2f screen_coordinates, float factor) noexcept
	{
		this->zoom_in_w(this->map_s2w(screen_coordinates), factor);
	}
	void zoom_in_w(ksn::vec2f world_coordinates, float factor) noexcept
	{
		this->m_origin += world_coordinates * (1 - factor);
		this->ratio /= factor;
	}
};



class swapchain_t;

class framebuffer_t
{
	friend class swapchain_t;

	std::vector<ksn::color_bgr_t> m_screen_data;
	swapchain_t* m_parent = nullptr;
	semaphore_storage_t m_semaphore;



public:

	void fill(ksn::color_bgr_t filler);
	void clear();

	void release();

	void draw_rect(ksn::vec2f downleft, ksn::vec2f topright, ksn::color_bgr_t color, const view_t* view = nullptr);

	swapchain_t* get_parent() const noexcept;
	const ksn::color_bgr_t* get_data() const noexcept;

};



class swapchain_t
{
	std::vector<framebuffer_t> m_frames;
	ksn::vec2i m_size;
	ksn::vec2i m_screen_size;
	size_t frame_index = -1;


public:

	void create(uint16_t width, uint16_t height, size_t frames = 1);

	framebuffer_t* acquire_frame() noexcept;

	ksn::vec2i get_screen_size() const noexcept;
	ksn::vec2i get_actual_size() const noexcept;
};



#endif //!_LOCAL_GRAPHICS_HPP_
