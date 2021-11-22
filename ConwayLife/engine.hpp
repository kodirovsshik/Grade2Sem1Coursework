
#ifndef _LOCAL_ENGINE_HPP_
#define _LOCAL_ENGINE_HPP_


#include "graphics.hpp"

#include <ksn/window.hpp>
#include <ksn/time.hpp>


class engine_t
{
public:
	
	using on_scroll_data_t = decltype(ksn::event_t::mouse_scroll_data);



private:

	static const wchar_t* sm_default_fatal_caption;
	static const wchar_t* sm_default_error_caption;
	static const wchar_t* sm_default_fatal_postfix;
	static const wchar_t* sm_default_error_postfix;

	static constexpr size_t sm_max_frames = 3;



	engine_t(const engine_t&) = delete;
	engine_t(engine_t&&) = delete;



	ksn::window_t m_window;
	ksn::vec<2, uint16_t> m_window_size;

	swapchain_t m_swapchain;
	std::binary_semaphore m_draw_semaphore;
	uint8_t m_bufferization;

	ksn::stopwatch m_update_clock;

	mutable ksn::vec2i m_mouse_pos;
	mutable bool m_mouse_pos_cached = false;

	static void static_display(engine_t*, framebuffer_t*);



	void basic_draw();
	bool basic_update();
	
	void fatal_reporter(const wchar_t*, const wchar_t* = engine_t::sm_default_fatal_caption, const wchar_t* = sm_default_fatal_postfix);
	void error_reporter(const wchar_t*, const wchar_t* = engine_t::sm_default_error_caption, const wchar_t* = sm_default_error_postfix);

	int main_loop();


protected:

	engine_t();


	bool engine_key_pressed[(int)ksn::keyboard_button_t::buttons_count];
	bool engine_key_released[(int)ksn::keyboard_button_t::buttons_count];
	bool engine_key_down[(int)ksn::keyboard_button_t::buttons_count];

	bool engine_mouse_key_pressed[(int)ksn::mouse_button_t::buttons_count];
	bool engine_mouse_key_released[(int)ksn::mouse_button_t::buttons_count];
	bool engine_mouse_key_down[(int)ksn::mouse_button_t::buttons_count];

	union
	{
		struct
		{
			bool engine_use_async_displaying : 1;
			bool engine_prevent_tearing : 1;
			bool engine_autoclear : 1;
			bool engine_reset_keyboard_keys_on_focus_loss : 1;
			bool engine_reset_mouse_keys_on_focus_loss : 1;
		};

		uint16_t engine_flags;
	};



	virtual bool update(float dt);
	virtual void draw(framebuffer_t&);

	virtual void on_init();
	virtual void on_exit();
	
	virtual bool on_close();
	virtual void on_focus_gain();
	virtual void on_focus_loss();
	virtual void on_scroll(on_scroll_data_t& scroll_data);
	

	void set_bufferization(uint8_t frames_count);
	uint8_t get_bufferization();

	void set_framerate_limit(uint32_t framerate);
	uint32_t get_framerate_limit();

	void set_window_size(ksn::vec<2, uint16_t> size);
	ksn::vec<2, uint16_t> get_window_size() const noexcept;

	void set_window_size_constraint(ksn::vec<2, uint16_t> min_size, ksn::vec<2, uint16_t> max_size);
	//ksn::vec<2, uint16_t> get_window_size_constraint() const noexcept;

	void set_window_title(const wchar_t*);

	ksn::vec2i get_mouse_pos() const noexcept;


public:

	int run(bool resizable_window = false);

	virtual ~engine_t() = 0;
};


#endif //!_LOCAL_ENGINE_HPP_
