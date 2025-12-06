#pragma once

#include "core/core.h"

namespace platformer2d {

	enum class EEventType : uint16_t
	{
		None = 0,
		SensorBegin,
		SensorEnd,
		ContactBegin,
		ContactEnd,
		COUNT
	};

	class CEvent
	{
	public:
		virtual ~CEvent() = default;

		virtual EEventType GetType() const = 0;
		virtual const char* GetName() const = 0;
		virtual std::string ToString() const { return GetName(); }

		[[nodiscard]] bool IsHandled() const { return bHandled; }

	public:
		bool bHandled = false;
	};

	/**
	 * @def LK_EVENT
	 * Implements basic event functionality.
	 */
	#define LK_EVENT(EventName, EventType, ...) \
		public: \
		using Class = EventName; \
		static ::platformer2d::EEventType GetStaticType() { return EEventType::EventType; } \
		virtual ::platformer2d::EEventType GetType() const override { return GetStaticType(); } \
		virtual const char* GetName() const override { return #EventType; }

	namespace Enum
	{
		inline const char* ToString(const EEventType Type)
		{
			const char* S = "";
		#define _(EnumValue) case EEventType::EnumValue: S = #EnumValue; break
			switch (Type)
			{
				_(None);
				_(SensorBegin);
				_(SensorEnd);
				_(COUNT);
				default:
					LK_THROW_ENUM_ERR(Type);
					break;
			}
		#undef _
			return S;
		}
	}

}
