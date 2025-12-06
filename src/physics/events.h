#pragma once

#include "core/core.h"
#include "core/event/event.h"

namespace platformer2d {

	class CActor;

	/************************************
	 * Sensor Events
	 ************************************/
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


	/************************************
	 * Contact Events
	 ************************************/
	class CContactBeginEvent : public CEvent
	{
	public:
		CContactBeginEvent(CActor* InA, CActor* InB)
			: A(InA)
			, B(InB)
		{
		}
		CContactBeginEvent() = delete;
		~CContactBeginEvent() = default;

	public:
		CActor* A = nullptr;
		CActor* B = nullptr;
	private:
		LK_EVENT(CContactBeginEvent, ContactBegin);
	};

	class CContactEndEvent : public CEvent
	{
	public:
		CContactEndEvent(CActor* InA, CActor* InB)
			: A(InA)
			, B(InB)
		{
		}
		CContactEndEvent() = delete;
		~CContactEndEvent() = default;

	public:
		CActor* A = nullptr;
		CActor* B = nullptr;
	private:
		LK_EVENT(CContactEndEvent, ContactEnd);
	};

}
