// based on https://github.com/15-466/15-466-f19-base6
// and https://github.com/Dmcdominic/15-466-f20-game4/blob/menu-mode/MenuMode.cpp#L140
#include "MenuMode.hpp"

//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

////for easy sprite drawing:
#include "DrawLines.hpp"

//for playing movement sounds:
#include "Sound.hpp"

//for loading:
#include "Load.hpp"
#include <random>
#include <string>
#include <set>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>


// harfbuzz, freetype
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>
#include "ColorTextureProgram.hpp"
#include <glm/gtc/type_ptr.hpp>

#include "DrawWords.hpp"
#include "data_path.hpp"



//Load< SpriteAtlas > trade_font_atlas(LoadTagDefault, []() -> SpriteAtlas const* {
//	return new SpriteAtlas(data_path("trade-font"));
//	});

// load txt file
Load<std::set<std::string>> common_words(LoadTagDefault, []()->std::set<std::string>* {
	// taken from: https://github.com/first20hours/google-10000-english
	std::set<std::string> *s  = new std::set<std::string>();
	std::ifstream file(data_path("20k.txt"));
	std::copy(std::istream_iterator<std::string>(file),
		std::istream_iterator<std::string>(),
		std::inserter(*s, (*s).end()));
	return s;
});


Load< Sound::Sample > sound_click(LoadTagDefault, []() -> Sound::Sample* {
	std::vector< float > data(size_t(48000 * 0.2f), 0.0f);
	for (uint32_t i = 0; i < data.size(); ++i) {
		float t = i / float(48000);
		//phase-modulated sine wave (creates some metal-like sound):
		data[i] = std::sin(3.1415926f * 2.0f * 440.0f * t + std::sin(3.1415926f * 2.0f * 450.0f * t));
		//quadratic falloff:
		data[i] *= 0.3f * std::pow(std::max(0.0f, (1.0f - t / 0.2f)), 2.0f);
	}
	return new Sound::Sample(data);
	});

Load< Sound::Sample > sound_clonk(LoadTagDefault, []() -> Sound::Sample* {
	std::vector< float > data(size_t(48000 * 0.2f), 0.0f);
	for (uint32_t i = 0; i < data.size(); ++i) {
		float t = i / float(48000);
		//phase-modulated sine wave (creates some metal-like sound):
		data[i] = std::sin(3.1415926f * 2.0f * 220.0f * t + std::sin(3.1415926f * 2.0f * 200.0f * t));
		//quadratic falloff:
		data[i] *= 0.3f * std::pow(std::max(0.0f, (1.0f - t / 0.2f)), 2.0f);
	}
	return new Sound::Sample(data);
	});



MenuMode::MenuMode(std::vector< Item > const& items_) : items(items_) {


	//select first item which can be selected:
	for (uint32_t i = 0; i < items.size(); ++i) {
		if (items[i].on_select) {
			selected = i;
			break;
		}
	}
	for (uint32_t i = 0; i < items.size(); ++i) {
		if (items[i].on_select) {
			num_selectable++;
		}
	}

	// last option is enter
	num_selectable -= 2;

	// randomize the letters once something is pressed
	randomize_letters(items);

	// initialize random seed
	srand((unsigned)time(0));

}

void MenuMode::randomize_letters(std::vector<Item>& items)
{
	// empty letters
	letters.clear();
	letters.reserve(num_selectable);

	/* initialize random seed: */
	// first letter is a vowel
	std::set<int> added_letters; // don't want the same letter multiple times
	added_letters.clear();
	int tmp = 0;
	tmp = rand() % 5;
	letters.push_back(vowels[tmp]);
	added_letters.insert(tmp);

	// initialize letters
	for (int i = 1; i < num_selectable; i++)
	{
		tmp = rand() % 26;
		while (added_letters.find(tmp) != added_letters.end())
		{
			tmp = rand() % 26;
		}
		added_letters.insert(tmp);
		letters.push_back(all_letters[tmp]);
	}
	// visualize
	int counter = 0;
	for (uint32_t i = 0; i < items.size(); i++) {
		if (items[i].on_select) {
			items[i].name = std::to_string(counter + 1) + ": " + std::string(1, letters[counter]);
			counter += 1;
			// don't change enter or reset
			if (counter == num_selectable)
			{
				break;
			}
		}
	}
}


MenuMode::~MenuMode() {
}

bool MenuMode::handle_event(SDL_Event const& evt, glm::uvec2 const& window_size) {
	if (evt.type == SDL_KEYDOWN) {
		if (evt.key.keysym.sym == SDLK_UP) {
			//skip non-selectable items:
			for (uint32_t i = selected - 1; i < items.size(); --i) {
				if (items[i].on_select) {
					selected = i;
					Sound::play(*sound_click);
					break;
				}
			}
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_DOWN) {
			//note: skips non-selectable items:
			for (uint32_t i = selected + 1; i < items.size(); ++i) {
				if (items[i].on_select) {
					selected = i;
					Sound::play(*sound_click);
					break;
				}
			}
			return true;
		}
		else if (evt.key.keysym.sym == SDLK_RETURN) {
			if (selected < items.size() && items[selected].on_select) {
				Sound::play(*sound_clonk);
				items[selected].on_select(items[selected]);
				// pressed enter
				if (items[selected].type == 3)
				{
					// processing code here
					if (common_words->find(current_str) != common_words->end())
					{
						points +=(int) current_str.size() * (int) current_str.size();
						if (points == max_points)
						{
							game_over = true;
						}
					}
					else
					{
						points -= std::max(0, points - 1);
					}
					current_str.clear();
				}
				// pressed reset
				else if (items[selected].type == 4)
				{
					int len = (int)current_str.size();
					len = std::max(1, len);
					points = std::max(0, points - 1);
					current_str.clear();
				}
				else
				{
					current_str += items[selected].name[3];
				}
				randomize_letters(items);
				return true;
			}
		}
	}
	if (background) {
		return background->handle_event(evt, window_size);
	}
	else {
		return false;
	}
}

void MenuMode::update(float elapsed) {
	if (background) {
		background->update(elapsed);
	}
}

void MenuMode::draw(glm::uvec2 const& drawable_size) {
	if (background) {
		std::shared_ptr< Mode > hold_me = shared_from_this();
		background->draw(drawable_size);
		//it is an error to remove the last reference to this object in background->draw():
		assert(hold_me.use_count() > 1);
	}
	else {
		glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	//// DRAW
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	///////////////////////////////////////////////////////
	static float scale = (float)std::min(drawable_size.x, drawable_size.y);
	static glm::mat4 to_clip = glm::mat4( //n.b. column major(!)
		2.0 /drawable_size.x, 0.0f, 0.0f, 0.0f,
		0.0f, 2.0 /drawable_size.y , 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		2.0f / float(drawable_size.x), 2.0f / float(drawable_size.y), 0.0f, 1.0f
	);	

	// key!!!
	static DrawWords draw_words(to_clip);

	int y_offset = 150;
	for (auto const& item : items) {

		// regular items
		if (item.type == 0 || item.type == 3 || item.type == 4)
		{
			bool is_selected = (&item == &items[0] + selected);
			/*float aspect = float(drawable_size.x) / float(drawable_size.y);*/
			glm::u8vec4 color = (is_selected ? glm::u8vec4(0xff, 0x00, 0x00, 0x00) : glm::u8vec4(0xff, 0xff, 0xff, 0x00));
			draw_words.draw_text(item.name, -50, y_offset, color);
			y_offset -= 50;
		}
		else if (item.type == 1)
		{
			// remaining items
			std::string s = item.name + ": " + std::to_string(points);
			draw_words.draw_text(s, -600, -300, glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}
		// drawing string
		else if (item.type == 2)
		{
			draw_words.draw_text(current_str, -600, 300, glm::u8vec4(0xff, 0xff, 0xff, 0x00));
		}

	}
	///////////////////////////////////////////////////////
	// older code that draws menus with draw lines... kept for maybe future use
	//use alpha blending:
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//don't use the depth test:
	//glDisable(GL_DEPTH_TEST);
	//{ //draw the menu using DrawSprites:
	//	/*assert(atlas && "it is an error to try to draw a menu without an atlas");
	//	DrawSprites draw_sprites(*atlas, view_min, view_max, drawable_size, DrawSprites::AlignPixelPerfect);*/
	//	float y_offset = 0.0f;
	//	for (auto const& item : items) {
	//		bool is_selected = (&item == &items[0] + selected);
	//		/*float aspect = float(drawable_size.x) / float(drawable_size.y);*/
	//		DrawLines lines(glm::mat4(
	//			1.0f / aspect, 0.0f, 0.0f, 0.0f,
	//			0.0f, 1.0f, 0.0f, 0.0f,
	//			0.0f, 0.0f, 1.0f, 0.0f,
	//			0.0f, 0.0f, 0.0f, 1.0f
	//		));
	//		constexpr float H = 0.2f;
	//		glm::u8vec4 color = (is_selected ? glm::u8vec4(0xff, 0x00, 0xff, 0x00) : glm::u8vec4(0x00, 0x00, 0x00, 0x00));
	//		lines.draw_text(item.name,
	//			glm::vec3(-aspect + 0.1f * H, 1.0f - 1.1f * H + y_offset, 0.0),
	//			glm::vec3(H, 0.0f, 0.0f), glm::vec3(0.0f, H, 0.0f),
	//			color
	//		);
	//		y_offset -= 0.5f;
	//	}
	//} //<-- gets drawn here!
	///////////////////////////////////////////////////////

	GL_ERRORS(); //PARANOIA: print errors just in case we did something wrong.
}

void MenuMode::layout_items(float gap) {
	float y = view_max.y;
	for (auto& item : items) {
		glm::vec2 min(0, 0);
		glm::vec2 max(0, 0);
		item.at.y = y - max.y;
		item.at.x = 0.5f * (view_max.x + view_min.x) - 0.5f * (max.x + min.x);
		y = y - (max.y - min.y) - gap;
	}
	float ofs = -0.5f * y;
	for (auto& item : items) {
		item.at.y += ofs;
	}
}
