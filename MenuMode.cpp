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


// harfbuzz, freetype
#include <ft2build.h>
#include FT_FREETYPE_H
#include <hb.h>
#include <hb-ft.h>
#include "ColorTextureProgram.hpp"
#include <glm/gtc/type_ptr.hpp>

unsigned int VAO, VBO;
static Load< void > setup_buffers(LoadTagDefault, []() {
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
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

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
	const int texture_id; // ID handle of the glyph texture
	const int width;
	const int height;
	const double offset_x;
	const double offset_y;
	const double advance_x;
	const double advance_y;
	Character(int texture_id, int width, int height, double offset_x, double offset_y, double advance_x, double advance_y) :
		texture_id(texture_id), width(width), height(height), offset_x(offset_x), offset_y(offset_y), advance_x(advance_x), advance_y(advance_y)
	{}
};


MenuMode::MenuMode(std::vector< Item > const& items_) : items(items_) {

	//select first item which can be selected:
	for (uint32_t i = 0; i < items.size(); ++i) {
		if (items[i].on_select) {
			selected = i;
			break;
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

	//select_bounce_acc = select_bounce_acc + elapsed / 0.7f;
	//select_bounce_acc -= std::floor(select_bounce_acc);

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

	// Derived from:
	// https://www.freetype.org/freetype2/docs/tutorial/step1.html
	// https://harfbuzz.github.io/ch03s03.html
	FT_Library library;
	FT_Face face;
	auto error = FT_Init_FreeType(&library);
	if (error)
	{
		std::cout << "Libary not loaded." << std::endl;
	}

	// https://harfbuzz.github.io/ch03s03.html
	// Create a buffer and put your text in it. 
	hb_buffer_t* buf = hb_buffer_create();
	std::string s = "AB";
	hb_buffer_add_utf8(buf, s.c_str(), -1, 0, -1);

	// Set the script, language and direction of the buffer. 
	hb_buffer_set_direction(buf, HB_DIRECTION_LTR);
	hb_buffer_set_script(buf, HB_SCRIPT_LATIN);
	hb_buffer_set_language(buf, hb_language_from_string("en", -1));

	// Create a face and a font, using FreeType for now. // just do once?
	error = FT_New_Face(library, "C:/Windows/Fonts/arial.ttf", 0, &face); // considering loading font file to memory
	if (error)
	{
		std::cout << "Failed to create face." << std::endl;
	}

	FT_Set_Char_Size(face, 0, 1000, 0, 0);
	hb_font_t* font = hb_ft_font_create(face, NULL);

	// Shape
	hb_shape(font, buf, NULL, 0);

	// Get the glyph and position information.
	unsigned int glyph_count;
	hb_glyph_info_t* glyph_info = hb_buffer_get_glyph_infos(buf, &glyph_count);
	hb_glyph_position_t* glyph_pos = hb_buffer_get_glyph_positions(buf, &glyph_count);

	// handle named slot that points to the face object's glyph slot
	FT_GlyphSlot  slot = face->glyph;

	static bool loaded = false;
	static std::vector<Character> characters;

	if (!loaded)
	{
		glEnable(GL_CULL_FACE);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glDisable(GL_DEPTH_TEST);

		// load textures
		for (size_t i = 0; i < glyph_count; ++i) {

			// glyph constants
			auto glyphid = glyph_info[i].codepoint; // the id specifies the specific letter
			double x_offset = glyph_pos[i].x_offset / 64.0;
			double y_offset = glyph_pos[i].y_offset / 64.0;
			double x_advance = glyph_pos[i].x_advance / 64.0;
			double y_advance = glyph_pos[i].y_advance / 64.0;

			// load glyph image
			error = FT_Load_Glyph(face, glyphid, FT_LOAD_DEFAULT);
			if (error)
			{
				std::cout << "Load glyph error" << std::endl;
			}

			// convert to bitmap
			error = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
			if (error)
			{
				std::cout << "Render error" << std::endl;
			}
			int w = face->glyph->bitmap.width;
			int h = face->glyph->bitmap.rows;

			// heavily following: https://learnopengl.com/In-Practice/Text-Rendering
			glUseProgram(color_texture_program->program);

			// 1 color
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			// store into texture
			unsigned int texture;
			glGenTextures(1, &texture);

			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
			);
			// set texture options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);

			// create character and push back
			characters.push_back({ (int)texture,w,h, x_offset, y_offset,x_advance,y_advance });

		}
		// configure VAO/VBO for texture quads
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
		loaded = true;
	}

	// DRAW
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Draw
	// start position
	double cursor_x = 0;
	double cursor_y = 0;
	for (size_t i = 0; i < glyph_count; ++i) {

		Character c = characters[i];
		// heavily following: https://learnopengl.com/In-Practice/Text-Rendering
		glm::mat4 to_clip = glm::mat4( //n.b. column major(!)
			1 * 2.0f / float(drawable_size.x), 0.0f, 0.0f, 0.0f,
			0.0f, 1 * 2.0f / float(drawable_size.y), 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			2.0f / float(drawable_size.x), 2.0f / float(drawable_size.y), 0.0f, 1.0f
		);
		glUseProgram(color_texture_program->program);
		glUniform3f(glGetUniformLocation(color_texture_program->program, "textColor"), 1.0f, 0.0f, 1.0f);
		glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(to_clip));
		glBindVertexArray(VAO);

		float xpos = (float)cursor_x + (float)c.offset_x;
		float ypos = (float)cursor_y + (float)c.offset_y;
		int w = c.width;
		int h = c.height;

		// update VBO for each character
		// two triangles!!!
		float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos,     ypos,       0.0f, 1.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos,     ypos + h,   0.0f, 0.0f },
			{ xpos + w, ypos,       1.0f, 1.0f },
			{ xpos + w, ypos + h,   1.0f, 0.0f }
		};

		// render glyph texture over quad
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, c.texture_id);

		// update content of VBO memory
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		// render quad (run opengl pipeline)
		glDrawArrays(GL_TRIANGLES, 0, 6);

		// update start
		cursor_x += c.advance_x;
		cursor_y += c.advance_y;
	}

	// clean up face
	FT_Done_Face(face);

	// close out
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);

	//// Tidy up. 
	hb_buffer_destroy(buf);
	hb_font_destroy(font);

	////use alpha blending:
	////glEnable(GL_BLEND);
	////glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	////don't use the depth test:
	//glDisable(GL_DEPTH_TEST);

	//float bounce = (0.25f - (select_bounce_acc - 0.5f) * (select_bounce_acc - 0.5f)) / 0.25f * select_bounce_amount;

	//{ //draw the menu using DrawSprites:
	//	/*assert(atlas && "it is an error to try to draw a menu without an atlas");
	//	DrawSprites draw_sprites(*atlas, view_min, view_max, drawable_size, DrawSprites::AlignPixelPerfect);*/
	//	float y_offset = 0.0f;
	//	for (auto const& item : items) {
	//		bool is_selected = (&item == &items[0] + selected);
	//		float aspect = float(drawable_size.x) / float(drawable_size.y);
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