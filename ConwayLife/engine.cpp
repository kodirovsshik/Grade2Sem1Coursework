
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

void engine_t::set_window_title(const wchar_t* title)
{
	this->m_window.set_title(title);
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

	this->engine_autoclear_color = ksn::color_bgr_t(0, 0, 0);

	this->engine_autoclear = true;
	this->engine_prevent_tearing = true;
	this->engine_use_async_displaying = true;

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
			frame->fill(this->engine_autoclear_color);
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
			return 0;
	}


	this->on_exit();
}


void engine_t::static_display(engine_t* engine, framebuffer_t* frame)
{
	bool use_lock = engine->engine_prevent_tearing;


	if (use_lock)
		engine->m_draw_semaphore.acquire();

	engine->m_window.draw_pixels_bgr_front(frame->get_data());

	if (use_lock)
		engine->m_draw_semaphore.release();


	if (engine->engine_autoclear)
		frame->fill(engine->engine_autoclear_color);


	frame->release();
	engine->m_window.tick();
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
