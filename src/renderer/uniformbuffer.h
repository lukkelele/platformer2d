#pragma once

#include "core/core.h"

namespace platformer2d {

	class CShader;

	class CUniformBuffer
	{
	public:
		CUniformBuffer(std::uint64_t Size, std::string_view InName);
		CUniformBuffer() = delete;
		CUniformBuffer(CUniformBuffer&&) = delete;
		CUniformBuffer(const CUniformBuffer&) = delete;
		~CUniformBuffer();

		void Destroy();

		void Bind() const;
		void Unbind() const;

		void SetData(const void* Data, std::uint64_t Size, std::uint64_t Offset = 0) const;
		void SetBinding(std::shared_ptr<CShader> Shader, std::string_view UBName, std::uint32_t BlockIndex);

		CUniformBuffer& operator=(CUniformBuffer&&) = delete;
		CUniformBuffer& operator=(const CUniformBuffer&) = delete;

	private:
		LRendererID ID = 0;
		std::string Name;
	};

}
