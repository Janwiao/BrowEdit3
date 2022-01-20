#pragma once

#include <glm/glm.hpp>
#include <browedit/gl/Shader.h>
#include <browedit/gl/Vertex.h>
#include <browedit/gl/VBO.h>
#include "Mesh.h"

#include <vector>

class Gadget
{
	class SimpleShader : public gl::Shader
	{
	public:
		SimpleShader() : gl::Shader("data/shaders/simple", Uniforms::End) { bindUniforms(); }
		struct Uniforms
		{
			enum
			{
				projectionMatrix,
				viewMatrix,
				modelMatrix,
				s_texture,
				textureFac,
				color,
				End
			};
		};
		void bindUniforms() override
		{
			bindUniform(Uniforms::projectionMatrix, "projectionMatrix");
			bindUniform(Uniforms::viewMatrix, "viewMatrix");
			bindUniform(Uniforms::modelMatrix, "modelMatrix");
			bindUniform(Uniforms::s_texture, "s_texture");
			bindUniform(Uniforms::textureFac, "textureFac");
			bindUniform(Uniforms::color, "color");
		}
	};
	class ArrowMesh : public Mesh
	{
	public:
		virtual std::vector<glm::vec3> buildVertices() override;
	};

	static inline SimpleShader* shader;
	static inline ArrowMesh arrowMesh;

	static inline glm::mat4 viewMatrix;
	static inline glm::mat4 projectionMatrix;
public:
	static void init();
	static void setMatrices(const glm::mat4& projectionMatrix, const glm::mat4& viewMatrix);
	enum class Mode
	{
		Translate,
		Rotate,
		Scale
	};
	enum Axis
	{
		X = 1,
		Y = 2,
		Z = 4
	};
	static void setMode(Mode mode);
	
	static inline bool axisClicked = false;
	static inline bool axisReleased = false;
	static inline bool axisDragged = false;
	static inline int selectedAxis = 0;

	static void draw(const math::Ray& mouseRay, const glm::mat4& modelMatrix);
};