#pragma once

#include "core/core.h"
#include "core/event/event.h"

namespace platformer2d {

	class CActor;

	class CSensorBeginEvent : public CEvent
	{
	public:
		CSensorBeginEvent(CActor* InSensor, CActor* InVisitor)
			: Sensor(InSensor)
			, Visitor(InVisitor)
		{
		}
		CSensorBeginEvent() = delete;
		~CSensorBeginEvent() = default;

	public:
		CActor* Sensor = nullptr;
		CActor* Visitor = nullptr;
	private:
		LK_EVENT(CSensorBeginEvent, SensorBegin);
	};

	class CSensorEndEvent : public CEvent
	{
	public:
		CSensorEndEvent(CActor* InSensor, CActor* InVisitor)
			: Sensor(InSensor)
			, Visitor(InVisitor)
		{
		}
		CSensorEndEvent() = delete;
		~CSensorEndEvent() = default;

	public:
		CActor* Sensor = nullptr;
		CActor* Visitor = nullptr;
	private:
		LK_EVENT(CSensorEndEvent, SensorEnd);
	};

}
