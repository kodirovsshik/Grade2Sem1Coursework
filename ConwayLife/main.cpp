
#include "engine.hpp"


class game_t : public engine_t
{
private:
	size_t count = 3;

public:
	virtual bool update(float dt)
	{
		return this->count-- != 0;
	}
	virtual void draw(framebuffer_t& frame)
	{

	}

	virtual void on_init()
	{
		this->set_window_size({ 300, 300 });
		this->set_window_size_constraint({ 200, 400 }, {250, 500});
		this->set_window_title(L"あ");

		this->engine_use_async_displaying = false;
		this->engine_autoclear_color = ksn::color_bgr_t(0, 0, 255);
	}
	virtual void on_exit()
	{

	}

};


//4 byte screen alignment
int main()
{
	return game_t().run();
}
