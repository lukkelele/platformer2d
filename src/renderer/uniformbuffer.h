#pragma once

#include "core/core.h"

namespace platformer2d {

	class CShader;

	class CUniformBuffer
	{
	public:
		CUniformBuffer(uint64_t Size);
		CUniformBuffer() = delete;
		~CUniformBuffer();

		void Bind() const;
		void Unbind() const;

		void SetData(const void* Data, uint64_t Size, uint64_t Offset = 0) const;
		void SetBinding(std::shared_ptr<CShader> Shader, std::string_view UBName, uint32_t BlockIndex);

	private:
		LRendererID ID;
	};

}