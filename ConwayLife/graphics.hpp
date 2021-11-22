
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
	float m_ratio = 0;


public:

	struct center_t {};

	view_t() noexcept;
	view_t(ksn::vec2f origin, float ratio) noexcept;
	view_t(ksn::vec2f center, float ratio, ksn::vec2i screen_size) noexcept;
	view_t(ksn::vec2f origin, float view_dim, int screen_dim) noexcept;
	view_t(ksn::vec2f origin, ksn::vec2f view_min_size, ksn::vec2i screen_size) noexcept;
	view_t(center_t, ksn::vec2f center, ksn::vec2f view_min_size, ksn::vec2i screen_size) noexcept;


	ksn::vec2f map_w2s(ksn::vec2f world_coordinates) const noexcept;
	ksn::vec2f map_s2w(ksn::vec2f screen_coordinates) const noexcept;

	void zoom_in_s(ksn::vec2f screen_coordinates, float factor) noexcept;
	void zoom_in_w(ksn::vec2f world_coordinates, float factor) noexcept;

	void shiht_by_w(ksn::vec2f world_dpos) noexcept;
	void shiht_by_s(ksn::vec2f screen_dpos) noexcept;
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
