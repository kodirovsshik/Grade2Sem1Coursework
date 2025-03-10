﻿
#include "engine.hpp"

#include <unordered_map>
#include <unordered_set>

#include <ksn/image.hpp>

#include <Windows.h>


class game_t : public engine_t
{
private:

	using row_t = std::unordered_set<int>;
	using field_t = std::unordered_map<int, row_t>;
	
	field_t m_field1, m_field2, m_field_cache;
	field_t* p_field = &m_field1, *p_aux_field = &m_field2;

	view_t m_view;

	bool m_is_paused : 1 = true;
	bool m_pause_skip : 1 = false;
	bool m_held_for_movement : 1 = false;
	bool m_should_exit : 1 = false;
	bool m_is_menu_opened : 1 = false;
	bool m_is_about_opened : 1 = false;
	bool mouse_on_button : 1 = false;
	bool mouse_on_menu_button : 1 = false;

	ksn::vec2i m_mouse_last_pos;

	float m_update_time = 0, m_update_perod = INFINITY;
	float m_gc_time = 0, m_gc_perod = 1;

	const ksn::vec2i menu_button_pos = { 10, 560 };

	ksn::image_bgra_t m_sprite_space_paused;
	ksn::image_bgra_t m_sprite_space_unpaused;
	ksn::image_bgra_t m_sprite_menu;
	ksn::image_bgra_t m_sprite_menu_button;
	ksn::image_bgra_t m_sprite_menu_button_light;
	ksn::image_bgra_t m_sprite_about;


	static constexpr ksn::color_bgr_t s_cell_alive_color = 0x808080;
	static constexpr ksn::color_bgr_t s_cell_alive_cursor_color = 0x606060;
	static constexpr ksn::color_bgr_t s_cell_dead_color = 0x000000;
	static constexpr ksn::color_bgr_t s_cell_dead_cursor_color = 0x202020;



	bool is_populated(int x, int y)
	{
		return this->p_field->count(y) == 1 && this->p_field->at(y).count(x) == 1;
	}
	bool is_populated(ksn::vec2i pos)
	{
		return this->is_populated(pos[0], pos[1]);
	}

	void set_populated(int x, int y, bool populated = true)
	{
		if (populated)
			(*this->p_field)[y].insert(x);
		else
		{
			if (this->p_field->contains(y))
				this->p_field->at(y).erase(x);
		}
	}
	void set_populated(ksn::vec2i pos, bool populated = true)
	{
		return this->set_populated(pos[0], pos[1], populated);
	}


	void aux_set_populated(int x, int y, bool populated = true)
	{
		if (populated)
			(*this->p_aux_field)[y].insert(x);
		else
		{
			if (this->p_aux_field->contains(y))
				this->p_aux_field->at(y).erase(x);
		}
	}
	void aux_conditionaly_populate(int x, int y)
	{
		if (this->m_field_cache.contains(y) && this->m_field_cache[y].contains(x))
			return;

		int count = 0;
		do
		{
			count += is_populated(x - 1, y - 1);
			count += is_populated(x - 0, y - 1);
			count += is_populated(x + 1, y - 1);
			count += is_populated(x + 1, y + 0);
			count += is_populated(x - 1, y + 0);
			if (count >= 4) break;
			
			count += is_populated(x - 1, y + 1);
			if (count >= 4) break;

			count += is_populated(x + 0, y + 1);
			if (count >= 4) break;
			
			count += is_populated(x + 1, y + 1);
		} while (false);

		this->aux_set_populated(x, y, count == 3 || count == 2 && is_populated(x, y));
		this->m_field_cache[y].insert(x);
	}


	void layer_game_update_internal(float dt)
	{
		if (this->m_should_exit)
			return;

		if (this->m_pause_skip)
		{
			this->m_is_paused = false;
			this->m_update_time += this->m_update_perod;
			this->m_gc_time += this->m_gc_perod;
		}
		
		if (this->m_is_paused) {}
		else
		{
			if (!this->m_pause_skip)
			{
				this->m_update_time += dt;
				this->m_gc_time += dt;
			}

			while (this->m_update_time >= this->m_update_perod)
			{
				this->m_field_cache.clear();
				this->p_aux_field->clear();

				for (const auto& [y, row] : *this->p_field)
				{
					for (int x : row)
					{
						this->aux_conditionaly_populate(x - 1, y - 1);
						this->aux_conditionaly_populate(x - 0, y - 1);
						this->aux_conditionaly_populate(x + 1, y - 1);
						this->aux_conditionaly_populate(x - 1, y - 0);
						this->aux_conditionaly_populate(x - 0, y - 0);
						this->aux_conditionaly_populate(x + 1, y - 0);
						this->aux_conditionaly_populate(x - 1, y + 1);
						this->aux_conditionaly_populate(x - 0, y + 1);
						this->aux_conditionaly_populate(x + 1, y + 1);
					}
				}

				std::swap(this->p_field, this->p_aux_field);

				this->m_update_time -= this->m_update_perod;
			}

			if (this->m_gc_time >= this->m_gc_perod)
			{
				this->m_gc_time = 0;

				std::erase_if(this->m_field1, [](const auto& x) { return x.second.empty(); });
				std::erase_if(this->m_field2, [](const auto& x) { return x.second.empty(); });
				this->m_field_cache = {};

#define __shrink_map(m) { m = decltype(m)(std::make_move_iterator(m.begin()), std::make_move_iterator(m.end())); }
				__shrink_map(this->m_field1);
				__shrink_map(this->m_field2);
#undef __shrink_map
			}
		}

		if (this->m_pause_skip)
		{
			this->m_is_paused = true;
			this->m_pause_skip = false;
		}
	}
	void layer_game_update_input()
	{
		if (this->engine_key_pressed[(int)ksn::keyboard_button_t::space])
			this->m_is_paused = !this->m_is_paused;

		if (this->engine_key_pressed[(int)ksn::keyboard_button_t::arrow_right])
			this->m_pause_skip = true;

		if (this->engine_mouse_key_pressed[(int)ksn::mouse_button_t::left])
			this->m_mouse_last_pos = this->get_mouse_pos();

		auto mouse_screen_pos = this->get_mouse_pos();
		auto mouse_pos = (ksn::vec2i)floor(this->m_view.map_s2w(mouse_screen_pos));

		const bool left_down = this->engine_mouse_key_down[(int)ksn::mouse_button_t::left];
		const bool right_down = this->engine_mouse_key_down[(int)ksn::mouse_button_t::right];

		mouse_on_button = false;
		mouse_on_menu_button = false;

		if (!this->m_is_menu_opened && !this->m_is_about_opened)
		{
			ksn::image_bgra_t &img = this->m_sprite_menu_button;
			ksn::vec2i menu_button_topright = menu_button_pos + ksn::vec2i{ img.width, img.height };

			if (ksn::clamp<2, int>(mouse_screen_pos, menu_button_pos, menu_button_topright - ksn::vec2i{ 1, 1 }) == mouse_screen_pos)
			{
				mouse_on_button = true;
				mouse_on_menu_button = true;
			}
		}

		if (this->m_is_about_opened && this->engine_key_down[(int)ksn::keyboard_button_t::enter])
		{
			this->m_is_about_opened = false;
			this->m_is_menu_opened = true;
		}

		
		bool focus_on_field = !mouse_on_button && !this->m_is_menu_opened && !this->m_is_about_opened;
		bool menu_just_opened = false;

		if (this->mouse_on_menu_button && this->engine_mouse_key_released[(int)ksn::mouse_button_t::left])
		{
			this->m_is_menu_opened = true;
			menu_just_opened = true;
		}

		if (this->m_is_menu_opened && !menu_just_opened && 
			this->engine_mouse_key_released[(int)ksn::mouse_button_t::left] &&
			mouse_screen_pos[0] <= (int)this->m_sprite_menu.width)
		{
			int mouse_y_reflected = this->get_window_size()[1] - 1 - mouse_screen_pos[1];
			if (mouse_y_reflected <= 34)
			{
				this->m_is_menu_opened = false;
			}
			else if (mouse_y_reflected <= 67)
			{
				this->m_is_menu_opened = false;
				this->m_is_about_opened = true;
			}
			else if (mouse_y_reflected <= 100)
			{
				this->m_should_exit = true;
			}
		}

		if (focus_on_field && (left_down || right_down) && this->get_mouse_pos() != this->m_mouse_last_pos)
		{
			if (GetAsyncKeyState(VK_SHIFT) < 0)
			{
				const bool mouse_inside_window = mouse_screen_pos == ksn::clamp<2, int>(mouse_screen_pos, ksn::vec2i{ 0, 0 }, this->get_window_size() - ksn::vec2i{ 1, 1 });

				if (mouse_inside_window && this->m_is_paused)
				{
					if (left_down)
						this->set_populated(mouse_pos, true);
					if (right_down)
						this->set_populated(mouse_pos, false);
				}
			}
			else if (left_down && !right_down)
			{
				this->m_held_for_movement = true;
				this->m_view.shiht_by_s(this->m_mouse_last_pos - this->get_mouse_pos());
			}
			this->m_mouse_last_pos = this->get_mouse_pos();
		}

		if (focus_on_field && !this->m_held_for_movement && this->m_is_paused)
		{
			if (this->engine_mouse_key_released[(int)ksn::mouse_button_t::left])
				this->set_populated(mouse_pos, true);
			if (this->engine_mouse_key_released[(int)ksn::mouse_button_t::right])
				this->set_populated(mouse_pos, false);
		}

		if (this->engine_mouse_key_released[(int)ksn::mouse_button_t::left])
			this->m_held_for_movement = false;
	}
	void layer_game_draw(framebuffer_t& frame)
	{
		for (const auto& [y, row] : *p_field)
		{
			for (const auto& x : row)
			{
				frame.draw_rect({ x, y }, { x + 1, y + 1 }, s_cell_alive_color, &this->m_view);
			}
		}
		
		auto mouse_screen_pos = this->get_mouse_pos();

		if (this->m_is_paused && !this->mouse_on_button && !this->m_is_menu_opened && !this->m_is_about_opened)
		{
			auto mouse_pos = (ksn::vec2i)floor(this->m_view.map_s2w(mouse_screen_pos));
			const bool mouse_inside_window = ksn::clamp<2, int>(mouse_screen_pos, ksn::vec2i{ 0, 0 }, this->get_window_size() - ksn::vec2i{ 1, 1 }) == mouse_screen_pos;

			ksn::color_bgr_t at_cursor_color =
				this->is_populated(mouse_pos)
				? s_cell_alive_cursor_color
				: s_cell_dead_cursor_color;
			frame.draw_rect(mouse_pos, mouse_pos + ksn::vec2i{ 1,1 }, at_cursor_color, &this->m_view);
		}

		if (this->m_is_paused)
			frame.draw_image({ 0, 0}, this->m_sprite_space_paused);
		else
			frame.draw_image({ 0, 0}, this->m_sprite_space_unpaused);

		if (!this->m_is_menu_opened && !this->m_is_about_opened)
		{
			if (mouse_on_menu_button)
				frame.draw_image(menu_button_pos, this->m_sprite_menu_button_light);
			else
				frame.draw_image(menu_button_pos, this->m_sprite_menu_button);
		}
		else
		{
			if (this->m_is_menu_opened)
			{
				const auto &img = this->m_sprite_menu;
				frame.draw_rect({ 0, 0 }, { (float)img.width, (float)this->get_window_size()[1] }, { 0xC0202020, 0 });
				frame.draw_image({ 0, this->get_window_size()[1] - (int)img.height }, img);
			}
			if (this->m_is_about_opened)
			{
				frame.draw_rect({ 0, 0 }, this->get_window_size(), { 0xC0202020, 0 });
				frame.draw_image({ 0, 0 }, this->m_sprite_about);
			}
		}
	}


	virtual void on_scroll(on_scroll_data_t& data)
	{
		static constexpr float zoom_speed = 1.2f;

		if (data.is_vertical)
			this->m_view.zoom_in_s({ data.x, data.y }, powf(zoom_speed, -data.delta));
	}

	static void init_image(ksn::image_bgra_t& image, const char* filename)
	{
		auto load_result = image.load_from_file(filename);
		if (load_result != ksn::image_bgra_t::load_result::ok)
		{
			std::wstring message;
			message.reserve(128);
			
			message += L"Не удалось загрузить ресурс: ";
			message.append(filename, filename + strlen(filename));
			message += L"\nКод ошибки: ";
			message += std::to_wstring(load_result);

			throw exception_with_code(load_result, std::move(message));
		}
	}


public:

	virtual bool update(float dt)
	{
		this->layer_game_update_input();
		this->layer_game_update_internal(dt);

		if (this->m_should_exit)
			return false;
		if (this->m_is_menu_opened || this->m_is_about_opened)
			return true;
		return !this->engine_key_down[(int)ksn::keyboard_button_t::esc];
	}
	virtual void draw(framebuffer_t& frame)
	{
		this->layer_game_draw(frame);
	}

	virtual void on_init()
	{
		this->set_window_title(L"John Conway's Game of Life");
		this->set_framerate_limit(60);
		this->m_update_perod = 1.f / 20;

		//this->engine_use_async_displaying = false;

		this->m_view = view_t(view_t::center_t{}, { 0, 0 }, {20, 20}, this->get_window_size());

		init_image(this->m_sprite_space_paused, "resources/spr_space_paused.png");
		init_image(this->m_sprite_space_unpaused, "resources/spr_space_unpaused.png");
		init_image(this->m_sprite_menu, "resources/spr_menu.png");
		init_image(this->m_sprite_menu_button, "resources/spr_menu_button.png");
		init_image(this->m_sprite_menu_button_light, "resources/spr_menu_button_light.png");
		init_image(this->m_sprite_about, "resources/spr_about.png");
	}
	virtual void on_exit()
	{

	}
	

};


int main()
{
	return game_t().run();
}
