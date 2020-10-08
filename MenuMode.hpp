#pragma once

// Heavily based on https://github.com/15-466/15-466-f19-base6

#include "Mode.hpp"

#include <vector>
#include <functional>
#include <string>

// random number generation: http://www.cplusplus.com/reference/cstdlib/rand/
#include <stdlib.h>     /* srand, rand */
#include <time.h>       /* time */

// types of buttons
enum Button { Letter, Points, String, Enter, Reset };

struct MenuMode : Mode {
	struct Item;
	MenuMode(std::vector< Item > const& items);
	virtual ~MenuMode();

	//functions called by main loop:
	virtual bool handle_event(SDL_Event const&, glm::uvec2 const& window_size) override;
	virtual void update(float elapsed) override;
	virtual void draw(glm::uvec2 const& drawable_size) override;

	//----- menu state -----

	//Each menu item is an "Item":
	struct Item {
		Item(
			std::string const& name_,
			/*Sprite const* sprite_ = nullptr,*/
			float scale_ = 1.0f,
			glm::u8vec4 const& tint_ = glm::u8vec4(0xff),
			std::function< void(Item const&) > const& on_select_ = nullptr,
			glm::vec2 const& at_ = glm::vec2(0.0f)
		) : name(name_),/* sprite(sprite_),*/ scale(scale_), tint(tint_), selected_tint(tint_), on_select(on_select_), at(at_) {
		}
		std::string name;
		//Sprite const* sprite; //sprite drawn for item
		float scale; //scale for sprite
		glm::u8vec4 tint; //tint for sprite (unselected)
		glm::u8vec4 selected_tint; //tint for sprite (selected)
		std::function< void(Item const&) > on_select; //if set, item is selectable
		glm::vec2 at; //location to draw item
		Button type = Letter;
	};

	std::vector< Item > items;
	int num_selectable = 0;
	std::vector<char> letters;
	std::vector<char> all_letters = { 'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z' };
	std::vector<char> vowels = { 'a','e','i','o','u' };
	std::string current_str = "";
	void randomize_letters(std::vector<Item>& items);
	// game points counter
	int points = 0;
	const int max_points = 25;
	bool game_over = false;

	//call to arrange items in a centered list:
	void layout_items(float gap = 0.0f);

	////if set, used to highlight the current selection:
	//Sprite const* left_select = nullptr;
	//Sprite const* right_select = nullptr;

	glm::vec2 left_select_offset = glm::vec2(0.0f);
	glm::vec2 right_select_offset = glm::vec2(0.0f);

	glm::u8vec4 left_select_tint = glm::u8vec4(0xff);
	glm::u8vec4 right_select_tint = glm::u8vec4(0xff);

	//currently selected item:
	uint32_t selected = 0;

	//area to display; by default, menu lays items out in the [-1,1]^2 box:
	glm::vec2 view_min = glm::vec2(-1.0f, -1.0f);
	glm::vec2 view_max = glm::vec2(1.0f, 1.0f);

	//if not nullptr, background's functions are called as follows:
	// background->handle_event() is called at the end of handle_event() [if this doesn't handle the event]
	// background->update() is called at the end of update()
	// background->draw() is called at the start of draw()
	//IMPORTANT NOTE: this means that if background->draw() ends up deleting this (e.g., by removing
	//  the last shared_ptr that references it), then it will crash. Don't do that!
	std::shared_ptr< Mode > background;

};