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

	class IEvent
	{
	protected:
		IEvent() = default;

	public:
		virtual ~IEvent() = default;
		IEvent(IEvent&&) = default;
		IEvent(const IEvent&) = delete;

		IEvent& operator=(IEvent&&) = default;
		IEvent& operator=(const IEvent&) = delete;

		virtual void Execute() = 0;
		virtual EEventType GetType() const = 0;
		virtual const char* GetName() const = 0;
		virtual std::string ToString() const { return GetName(); }
	};

	namespace Enum {
		constexpr const char* ToString(const EEventType Type)
		{
			const char* S = "";
#define _(EnumValue)                                  \
	case EEventType::EnumValue: S = #EnumValue; break
			switch (Type) {
				_(None);
				_(SensorBegin);
				_(SensorEnd);
				_(COUNT);
				default:
					return nullptr;
			}
#undef _
			return S;
		}
	}

/**
 * @def LK_EVENT
 * Implements basic event functionality.
 */
#define LK_EVENT(EventName, EventType, ...)                     \
public:                                                         \
	using Class = EventName;                                    \
	static ::platformer2d::EEventType GetStaticType()           \
	{                                                           \
		return EEventType::EventType;                           \
	}                                                           \
	virtual ::platformer2d::EEventType GetType() const override \
	{                                                           \
		return GetStaticType();                                 \
	}                                                           \
	virtual const char* GetName() const override                \
	{                                                           \
		return #EventType;                                      \
	}

}
