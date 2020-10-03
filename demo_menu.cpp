// based on https://github.com/Dmcdominic/15-466-f20-game4/blob/menu-mode/MenuMode.cpp
// and https://github.com/15-466/15-466-f19-base6

#include "demo_menu.hpp"
#include "Load.hpp"
//#include "Sprite.hpp"
#include "data_path.hpp"
#include "PlayMode.hpp"
#include <iostream>
#include <vector>

//#include "PlantMode.hpp"
//#include "DemoLightingMultipassMode.hpp"
//#include "DemoLightingForwardMode.hpp"
//#include "DemoLightingDeferredMode.hpp"

//Load< SpriteAtlas > trade_font_atlas(LoadTagDefault, []() -> SpriteAtlas const* {
//	return new SpriteAtlas(data_path("trade-font"));
//	});

std::shared_ptr< MenuMode > demo_menu;

Load< void > load_demo_menu(LoadTagDefault, []() {
	std::vector< MenuMode::Item > items;
	items.emplace_back("[[ DEMO MENU ]]");
	items.emplace_back("Option 1");
	items.back().on_select = [](MenuMode::Item const&) {
		Mode::set_current(std::make_shared< PlayMode >());
	};
	items.emplace_back("Option 2");
	items.back().on_select = [](MenuMode::Item const&) {
		std::cout << "2" << std::endl;
	};
	items.emplace_back("Option 3");
	items.back().on_select = [](MenuMode::Item const&) {
		std::cout << "3" << std::endl;
	};

	demo_menu = std::make_shared< MenuMode >(items);
	demo_menu->selected = 1;
	//demo_menu->atlas = trade_font_atlas;
	demo_menu->view_min = glm::vec2(0.0f, 0.0f);
	demo_menu->view_max = glm::vec2(320.0f, 200.0f);
	demo_menu->layout_items(2.0f);
	//demo_menu->left_select = &trade_font_atlas->lookup(">");
	demo_menu->left_select_offset = glm::vec2(-5.0f - 3.0f, 0.0f);
	//demo_menu->right_select = &trade_font_atlas->lookup("<");
	demo_menu->right_select_offset = glm::vec2(0.0f + 3.0f, 0.0f);
	//demo_menu->select_bounce_amount = 5.0f;
});
