#include "uniformbuffer.h"

#include "renderer/opengl.h"
#include "renderer/shader.h"

namespace platformer2d {

	CUniformBuffer::CUniformBuffer(const uint64_t Size)
	{
		LK_OpenGL_Verify(glCreateBuffers(1, &ID));
		LK_OpenGL_Verify(glNamedBufferData(ID, Size, nullptr, GL_DYNAMIC_DRAW)); 
		LK_OpenGL_Verify(glBindBufferBase(GL_UNIFORM_BUFFER, 0, ID));
	}

	CUniformBuffer::~CUniformBuffer()
	{
		LK_OpenGL_Verify(glDeleteBuffers(1, &ID));
	}

	void CUniformBuffer::Bind() const
	{
		LK_OpenGL_Verify(glBindBuffer(GL_UNIFORM_BUFFER, ID));
	}

	void CUniformBuffer::Unbind() const
	{
		LK_OpenGL_Verify(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	}

	void CUniformBuffer::SetData(const void* Data, const uint64_t Size, const uint64_t Offset) const
	{
		LK_OpenGL_Verify(glBindBuffer(GL_UNIFORM_BUFFER, ID));
		LK_OpenGL_Verify(glNamedBufferSubData(ID, Offset, Size, Data));
		LK_OpenGL_Verify(glBindBuffer(GL_UNIFORM_BUFFER, 0));
	}

	void CUniformBuffer::SetBinding(const std::shared_ptr<CShader> Shader, std::string_view UBName, const uint32_t BlockIndex)
	{
		uint32_t UBIndex;
		LK_OpenGL_Verify(UBIndex = glGetUniformBlockIndex(Shader->GetRendererID(), UBName.data()));
		if (UBIndex == BlockIndex)
		{
			LK_WARN_TAG("UniformBuffer", "Already bound to index {} in \"{}\"", BlockIndex, UBName);
			return;
		}

		LK_OpenGL_Verify(glUniformBlockBinding(Shader->GetRendererID(), UBIndex, BlockIndex));
		LK_DEBUG_TAG("UniformBuffer", "Set index {} for {}", BlockIndex, UBName);
	}

}