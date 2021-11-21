
#include "engine.hpp"


class game_t : public engine_t
{
private:

public:
	virtual bool update(float dt)
	{
		return !this->engine_key_down[(int)ksn::keyboard_button_t::esc];
	}
	virtual void draw(framebuffer_t& frame)
	{
	}

	virtual void on_init()
	{
		this->set_window_title(L"あ");
		this->set_framerate_limit(60);

		this->engine_use_async_displaying = false;
	}
	virtual void on_exit()
	{

	}

};


int main()
{
	return game_t().run();
}
