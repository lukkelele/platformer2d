#pragma once

#include "core/core.h"
#include "yaml.h"

namespace platformer2d {

	enum class ESerializable
	{
		File,
		Yaml,
		All /* File and Yaml. */
	};

	/**
	 * @class ISerializable
	 * @brief Serializer interface.
	 */
	template<ESerializable SerializerType>
	class ISerializable;

	template<>
	class ISerializable<ESerializable::Yaml>
	{
	public:
		using TSink = YAML::Emitter;
	public:
		virtual ~ISerializable() = default;

		virtual bool Serialize(TSink& Sink) const = 0;
		/**
		 * Deserialize is not used for ESerializable::Yaml.
		 * There are deserializers in the 'Serialization' namespace for this.
		 */
	};

	template<>
	class ISerializable<ESerializable::File>
	{
	public:
		using TSink = std::filesystem::path;
	public:
		virtual ~ISerializable() = default;

		virtual bool Serialize(const TSink& OutFile) const = 0;
		virtual bool Deserialize(const TSink& InFile) = 0;
	};

	template<>
	class ISerializable<ESerializable::All>
	{
	public:
		using TYamlSink = YAML::Emitter;
		using TFileSink = std::filesystem::path;

	public:
		virtual ~ISerializable() = default;

		virtual bool Serialize(TYamlSink& Out) = 0;
		/**
		 * Deserialize is not used for ESerializable::Yaml.
		 * There are deserializers in the 'Serialization' namespace for this.
		 */

		virtual bool Serialize(const TFileSink& OutFile) const = 0;
		virtual bool Deserialize(const TFileSink& InFile) = 0;
	};

}
