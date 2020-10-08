#pragma once


#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
//for the GL_ERRORS() macro:
#include "gl_errors.hpp"

#include <string>
#include <vector>
#include <functional>
#include <map>

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
	int texture_id; // ID handle of the glyph texture
	int width;
	int height;
	double offset_x;
	double offset_y;
	double advance_x;
	double advance_y;
	Character(int texture_id, int width, int height, double offset_x, double offset_y, double advance_x, double advance_y) :
		texture_id(texture_id), width(width), height(height), offset_x(offset_x), offset_y(offset_y), advance_x(advance_x), advance_y(advance_y)
	{}
};

struct DrawWords {
	//Start drawing; will remember world_to_clip matrix:
	DrawWords(glm::mat4 const& world_to_clip);

	//draw wireframe text, start at anchor, move in x direction, mat gives x and y directions for text drawing:
	// (default character box is 1 unit high)
	void draw_text(std::string const& text, const int x, const int y, glm::u8vec4 const& color = glm::u8vec4(0xff));

	//Finish drawing (push attribs to GPU):
	~DrawWords();

	glm::mat4 world_to_clip;

	std::map<std::string,std::vector<Character>> words;
};




