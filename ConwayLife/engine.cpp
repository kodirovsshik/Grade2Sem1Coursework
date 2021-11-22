
#include "engine.hpp"
#include "auxillary.hpp"

#include <thread>

#include <wchar.h>

#include <Windows.h>



const wchar_t* engine_t::sm_default_error_caption = L"Ошибка";
const wchar_t* engine_t::sm_default_fatal_caption = L"Фатальная ошибка";

const wchar_t* engine_t::sm_default_error_postfix = L"";
const wchar_t* engine_t::sm_default_fatal_postfix = L"Данная ошибка является критической, приложение будет закрыто и должно быть перезапущено для продолжения работы";





enum engine_exit_code
{
	unknown_error = -1,
	ok = 0,
	out_of_memory,
	window_open_failure
};



bool engine_t::update(float dt)
{
	return true;
}
void engine_t::draw(framebuffer_t&)
{
}

void engine_t::on_init()
{
}
void engine_t::on_exit()
{
}

bool engine_t::on_close()
{
	return true;
}
void engine_t::on_focus_gain()
{
}
void engine_t::on_focus_loss()
{
}
void engine_t::on_scroll(on_scroll_data_t& scroll_data)
{
}


void engine_t::set_bufferization(uint8_t frames_count)
{
	if (frames_count >= 1 && frames_count <= engine_t::sm_max_frames && this->m_bufferization != frames_count)
		this->m_swapchain.create(this->m_window_size[0], this->m_window_size[1], this->m_bufferization = frames_count);
}
uint8_t engine_t::get_bufferization()
{
	return this->m_bufferization;
}

void engine_t::set_framerate_limit(uint32_t framerate)
{
	if (framerate)
		this->m_window.set_framerate(framerate);
}
uint32_t engine_t::get_framerate_limit()
{
	return this->m_window.get_framerate();
}


void engine_t::set_window_size(ksn::vec<2, uint16_t> size)
{
	auto old_size = this->m_window_size;

	this->m_window.set_client_size(size);
	auto new_size = this->m_window.get_client_size();

	if (new_size != old_size)
	{
		this->m_window_size = new_size;
		this->m_swapchain.create(size[0], size[1], this->m_bufferization);
	}
}
ksn::vec<2, uint16_t> engine_t::get_window_size() const noexcept
{
	return this->m_window_size;
}
void engine_t::set_window_size_constraint(ksn::vec<2, uint16_t> min_size, ksn::vec<2, uint16_t> max_size)
{
	auto old_size = this->m_window_size;

	this->m_window.set_size_constraint(min_size, max_size);
	auto new_size = this->m_window.get_client_size();

	if (new_size != old_size)
	{
		this->m_window_size = new_size;
		this->m_swapchain.create(new_size.first, new_size.second, this->m_bufferization);
	}
}
//ksn::vec<2, uint16_t> engine_t::get_window_size_constraint() const noexcept
//{
//	return this->m_window;
//}

void engine_t::set_window_title(const wchar_t* title)
{
	this->m_window.set_title(title);
}

ksn::vec2i engine_t::get_mouse_pos() const noexcept
{
	if (this->m_mouse_pos_cached)
		return this->m_mouse_pos;

	POINT pt;
	GetCursorPos(&pt);
	ScreenToClient(this->m_window.window_native_handle(), &pt);

	pt.y = this->m_window_size[1] - 1 - pt.y;

	this->m_mouse_pos = ksn::vec2i{ pt.x, pt.y };
	this->m_mouse_pos_cached = true;
	return this->m_mouse_pos;
}



engine_t::~engine_t()
{
}

engine_t::engine_t()
	: m_draw_semaphore(1)
{
	memset(this->engine_key_down, 0, sizeof(this->engine_key_down));
	memset(this->engine_key_pressed, 0, sizeof(this->engine_key_pressed));
	memset(this->engine_key_released, 0, sizeof(this->engine_key_released));

	this->engine_autoclear = true;
	this->engine_prevent_tearing = true;
	this->engine_use_async_displaying = true;

	this->engine_reset_keyboard_keys_on_focus_loss = true;
	this->engine_reset_mouse_keys_on_focus_loss = true;

	this->m_bufferization = 2;
	this->m_window_size = { 800, 600 };
}


static void handle_window_open_result(ksn::window_open_result_t open_result)
{
	switch (open_result)
	{
	case ksn::window_open_result::ok:
		break;

	case ksn::window_open_result::ok_but_direct_drawing_unsupported:
		throw exception_with_code(engine_exit_code::window_open_failure, L"\
Не удалось создать битовую карту изображения окна\n\
Попробуйте закрыть другие программы чтобы освободить оперативную память");
		break;

	default:
		throw exception_with_code(engine_exit_code::window_open_failure, L"\
Неизвестная ошибка\nНе удалось создать окно игры\nКод " + std::to_wstring(open_result));
	}
}

int engine_t::run(bool resizeable_window)
{
	ksn::window_style_t style = 
		ksn::window_style::border | 
		ksn::window_style::caption | 
		ksn::window_style::close_min_max | 
		ksn::window_style::hidden;

	if (resizeable_window)
		style |= ksn::window_style::resize;

	try
	{
		handle_window_open_result(this->m_window.open(this->m_window_size[0], this->m_window_size[1], L"", style));
		
		this->m_swapchain.create(this->m_window_size[0], this->m_window_size[1], this->m_bufferization);

		return this->main_loop();
	}
	catch (const std::bad_alloc&)
	{
		this->fatal_reporter(L"Программе не удалось выделить память под обязательные нужды\nПопробуйте закрыть другие программы чтобы освободить оперативную память\n");
		return engine_exit_code::ok;
	}
	catch (const exception_with_code& excp)
	{
		this->fatal_reporter(excp.what());
		return excp.code();
	}
	catch (...)
	{
		return engine_exit_code::unknown_error;
	}
}

int engine_t::main_loop()
{
	this->on_init();

	if (this->engine_autoclear)
	{
		for (size_t i = 0; i < this->m_bufferization; ++i)
		{
			auto* frame = this->m_swapchain.acquire_frame();
			frame->clear();
			frame->release();
		}
	}

	if (this->get_framerate_limit() == 0)
		this->m_update_clock.start();
	//else it would be 0 at restart() and will be replaced with 1/fps


	this->m_window.show();
	
	while (true)
	{
		this->basic_draw();
		if (!this->basic_update())
			break;
	}
	
	this->m_window.close();


	this->on_exit();
	return 0;
}


void engine_t::static_display(engine_t* engine, framebuffer_t* frame)
{
	bool use_lock = engine->engine_prevent_tearing;


	if (use_lock)
		engine->m_draw_semaphore.acquire();
	
	auto buffer_size = engine->m_swapchain.get_actual_size();
	engine->m_window.draw_pixels_bgr_front(frame->get_data(), 0, 0, buffer_size[0], buffer_size[1]);

	if (use_lock)
		engine->m_draw_semaphore.release();


	if (engine->engine_autoclear)
		frame->clear();


	engine->m_window.tick();
	frame->release();
}


void engine_t::basic_draw()
{
	auto* frame = this->m_swapchain.acquire_frame();

	this->draw(*frame);

	if (this->engine_use_async_displaying)
	{
		std::thread(engine_t::static_display, this, frame).detach();
	}
	else
	{
		engine_t::static_display(this, frame);
	}
}
bool engine_t::basic_update()
{
	memset(this->engine_key_pressed, 0, sizeof(this->engine_key_pressed));
	memset(this->engine_key_released, 0, sizeof(this->engine_key_released));
	memset(this->engine_mouse_key_pressed, 0, sizeof(this->engine_mouse_key_pressed));
	memset(this->engine_mouse_key_released, 0, sizeof(this->engine_mouse_key_released));

	ksn::event_t ev;
	while (this->m_window.poll_event(ev))
	{
		switch (ev.type)
		{
		case ksn::event_type_t::close:
			if (this->on_close())
				return false;
			break;

		case ksn::event_type_t::keyboard_press:
			this->engine_key_down[(int)ev.keyboard_button_data.button] = true;
			this->engine_key_pressed[(int)ev.keyboard_button_data.button] = true;
			break;

		case ksn::event_type_t::keyboard_release:
			this->engine_key_down[(int)ev.keyboard_button_data.button] = false;
			this->engine_key_released[(int)ev.keyboard_button_data.button] = true;
			break;

		case ksn::event_type_t::mouse_press:
			this->engine_mouse_key_down[(int)ev.mouse_button_data.button] = true;
			this->engine_mouse_key_pressed[(int)ev.mouse_button_data.button] = true;
			break;

		case ksn::event_type_t::mouse_release:
			this->engine_mouse_key_down[(int)ev.mouse_button_data.button] = false;
			this->engine_mouse_key_released[(int)ev.mouse_button_data.button] = true;
			break;

		case ksn::event_type_t::focus_gained:
			this->on_focus_gain();
			break;

		case ksn::event_type_t::focus_lost:
			this->on_focus_loss();
			if (this->engine_reset_keyboard_keys_on_focus_loss)
			{
				for (size_t i = 0; i < std::size(this->engine_key_down); ++i)
					if (this->engine_key_down[i])
					{
						this->engine_key_down[i] = false;
						this->engine_key_released[i] = true;
					}
			}
			if (this->engine_reset_mouse_keys_on_focus_loss)
			{
				for (size_t i = 0; i < std::size(this->engine_mouse_key_down); ++i)
					if (this->engine_mouse_key_down[i])
					{
						this->engine_mouse_key_down[i] = false;
						this->engine_mouse_key_released[i] = true;
					}
			}
			break;

		case ksn::event_type_t::resize:
			this->m_window_size = { ev.window_resize_data.width_new, ev.window_resize_data.height_new };
			this->m_swapchain.create(ev.window_resize_data.width_new, ev.window_resize_data.height_new, this->m_bufferization);
			break;

		case ksn::event_type_t::maximized:
			ksn::nop();
			break;

		case ksn::event_type_t::minimized:
			ksn::nop();
			break;

		case ksn::event_type_t::mouse_move:
			this->m_mouse_pos = { ev.mouse_move_data.x, this->m_window_size[1] - 1 - ev.mouse_move_data.y };
			this->m_mouse_pos_cached = true;
			break;

		case ksn::event_type_t::mouse_scroll_vertical:
		case ksn::event_type_t::mouse_scroll_horizontal:
			ev.mouse_scroll_data.y = this->m_window_size[1] - 1 - ev.mouse_scroll_data.y;
			this->on_scroll(ev.mouse_scroll_data);
			break;
		}


	}

	float dt = this->m_update_clock.restart().as_float_sec();
	if (dt == 0)
		dt = 1.f / this->m_window.get_framerate();

	return this->update(dt);
}

void engine_t::fatal_reporter(const wchar_t* text, const wchar_t* caption, const wchar_t* postfix)
{
	this->m_window.close();

	if (caption == nullptr)
		caption = engine_t::sm_default_fatal_caption;

	this->error_reporter(text, caption, postfix);
}
void engine_t::error_reporter(const wchar_t* text, const wchar_t* caption, const wchar_t* postfix)
{
	static constexpr size_t total_size = 8192;
	static wchar_t error_buffer[total_size];

	swprintf(error_buffer, total_size, L"%ws%ws%ws", text, postfix[0] == 0 ? L"" : L"\n\n", postfix);

	if (caption == nullptr)
		caption = engine_t::sm_default_error_caption;
	
	MessageBoxW(GetConsoleWindow(), error_buffer, caption, MB_ICONERROR);
}
