
#include "DrawWords.hpp"

//for loading:
#include "Load.hpp"

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

	GL_ERRORS(); //PARANOIA: make sure nothing strange happened during setup

	});

DrawWords::DrawWords(glm::mat4 const& world_to_clip_) : world_to_clip(world_to_clip_)
{}




// to do: have the textures stored in some permanent array than having them
// generated on each time draw_text is called
void DrawWords::draw_text(std::string const& text, const int x, const int y, glm::u8vec4 const& color)
{
	std::vector<Character> characters;
	auto it = words.find(text);
	if (it != words.end())
	{
		characters = it->second;
	}
	else
	{
		characters.clear();
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

		hb_buffer_add_utf8(buf, text.c_str(), -1, 0, -1);

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
			x_offset = face->glyph->bitmap_left;
			y_offset = face->glyph->bitmap_top;
			x_advance = face->glyph->advance.x;
			y_advance = face->glyph->advance.y;

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
		words.insert(std::pair < std::string, std::vector<Character>>(text, characters));
		// clean up face
		FT_Done_Face(face);
		//// Tidy up. 
		hb_buffer_destroy(buf);
		hb_font_destroy(font);
	}

	//// DRAW
	//glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	//glClear(GL_COLOR_BUFFER_BIT);
	// Draw
	// start position
	double cursor_x = x;
	double cursor_y = y;
	for (size_t i = 0; i < characters.size(); ++i) {
		Character c = characters[i];
		// heavily following: https://learnopengl.com/In-Practice/Text-Rendering
		glUseProgram(color_texture_program->program);
		glUniform3f(glGetUniformLocation(color_texture_program->program, "textColor"), color.x, color.y, color.z);
		glUniformMatrix4fv(color_texture_program->OBJECT_TO_CLIP_mat4, 1, GL_FALSE, glm::value_ptr(world_to_clip));
		glBindVertexArray(VAO);
		float xpos = (float)cursor_x + (float)c.offset_x;
		float ypos = (float)cursor_y - (c.height - (float)c.offset_y);
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
		cursor_x += c.advance_x/64.0;
		cursor_y += c.advance_y/64.0;
	}
	// close out
	glBindVertexArray(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	glUseProgram(0);
}

DrawWords::~DrawWords()
{
}
