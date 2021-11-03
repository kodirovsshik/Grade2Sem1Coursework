
#ifndef _LOCAL_GRAPHICS_HPP_
#define _LOCAL_GRAPHICS_HPP_



#include <vector>
#include <semaphore>
#include <future>

#include <ksn/color.hpp>
#include <ksn/math_vec.hpp>
#include <ksn/window.hpp>


class view_t
{
	ksn::vec2f m_origin;
	float ratio = 0;


	ksn::vec2f map_w2s(ksn::vec2f world_coordinates)
	{
		return (world_coordinates - this->m_origin) * ratio;
	}
	ksn::vec2f map_s2w(ksn::vec2f world_coordinates)
	{
		return world_coordinates / ratio + this->m_origin;
	}

	void zoom_in_s(ksn::vec2f screen_coordinates, float factor)
	{
		this->zoom_in_w(this->map_s2w(screen_coordinates), factor);
	}
	void zoom_in_w(ksn::vec2f world_coordinates, float factor)
	{
		this->m_origin += world_coordinates * (1 - factor);
		this->ratio /= factor;
	}
};


class swapchain_t
{
	class semaphore_storage_t
	{
		friend class swapchain_t;

		std::binary_semaphore m_semaphore;

	public:
		semaphore_storage_t() noexcept
			: m_semaphore(1)
		{}

		semaphore_storage_t(semaphore_storage_t&&) noexcept
			: m_semaphore(1)
		{}

		void acquire() noexcept
		{
			this->m_semaphore.acquire();
		}

		void release() noexcept
		{
			this->m_semaphore.release();
		}
	};


	
	class framebuffer_t
	{
		friend class swapchain_t;

		std::vector<ksn::color_bgr_t> m_screen_data;
		swapchain_t* m_parent = nullptr;
		semaphore_storage_t m_semaphore;
		
		size_t index()
		{
			return this - this->m_parent->m_frames.data();
		}


	public:

		void fill(ksn::color_bgr_t filler)
		{
			std::ranges::fill(this->m_screen_data, filler);
		}
		void clear()
		{
			memset(
				this->m_screen_data.data(),
				0,
				this->m_screen_data.size() * sizeof(this->m_screen_data[0])
			);
		}

		swapchain_t* get_parent() const noexcept
		{
			return this->m_parent;
		}

		void draw_rect(ksn::vec2f downleft, ksn::vec2f topright)
		{
			const auto& size = this->m_parent->m_size;
			
			auto transform = [this]
			(ksn::vec2f v) -> ksn::vec2f
			{
				for (auto& x : v.data)
					x = roundf(x);
				return ksn::clamp(v, { 0, 0 }, ksn::vec2f(this->get_parent()->get_size() - ksn::vec2i{1, 1}));
			};

			//TODO: finish
		}


		void display_and_release(ksn::window_t& target)
		{
			static constexpr bool async_presenting = false;

			if constexpr (async_presenting)
				std::thread(_display, &target, this).detach();
			else
				_display(&target, this);
		}

	private:
		static void _display(ksn::window_t* target, framebuffer_t* frame)
		{
			target->draw_pixels_bgr_front(frame->m_screen_data.data());
			target->tick();
			frame->m_semaphore.release();
		}
	};

	std::vector<framebuffer_t> m_frames;
	ksn::vec2i m_size;
	size_t frame_index = -1;


public:

	void create(uint16_t width, uint16_t height, size_t frames = 1)
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

	framebuffer_t* acquire_frame() noexcept
	{
		if (this->m_frames.size() == 0)
			return nullptr;

		if (++this->frame_index == this->m_frames.size())
			this->frame_index = 0;

		auto* result = &this->m_frames[this->frame_index];
		result->m_semaphore.acquire();
		return result;
	}

	ksn::vec2i get_size() const noexcept
	{
		return this->m_size;
	}
};



#endif //!_LOCAL_GRAPHICS_HPP_
