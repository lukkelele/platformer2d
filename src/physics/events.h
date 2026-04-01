#pragma once

#include "core/core.h"
#include "core/event/event.h"

namespace platformer2d {

	class CActor;

	/************************************
	 * Sensor Events
	 ************************************/
	class CSensorBeginEvent : public IEvent
	{
	public:
		CSensorBeginEvent(CActor* InSensor, CActor* InVisitor)
			: Sensor(InSensor)
			, Visitor(InVisitor)
		{
		}
		CSensorBeginEvent() = delete;
		~CSensorBeginEvent() = default;

		void Execute() override {}

	public:
		CActor* Sensor = nullptr;
		CActor* Visitor = nullptr;

	private:
		LK_EVENT(CSensorBeginEvent, SensorBegin);
	};

	class CSensorEndEvent : public IEvent
	{
	public:
		CSensorEndEvent(CActor* InSensor, CActor* InVisitor)
			: Sensor(InSensor)
			, Visitor(InVisitor)
		{
		}
		CSensorEndEvent() = delete;
		~CSensorEndEvent() = default;

		void Execute() override {}

	public:
		CActor* Sensor = nullptr;
		CActor* Visitor = nullptr;

	private:
		LK_EVENT(CSensorEndEvent, SensorEnd);
	};

	/************************************
	 * Contact Events
	 ************************************/
	class CContactBeginEvent : public IEvent
	{
	public:
		CContactBeginEvent(CActor* InA, CActor* InB)
			: A(InA)
			, B(InB)
		{
		}
		CContactBeginEvent() = delete;
		~CContactBeginEvent() = default;

		void Execute() override {}

	public:
		CActor* A = nullptr;
		CActor* B = nullptr;

	private:
		LK_EVENT(CContactBeginEvent, ContactBegin);
	};

	class CContactEndEvent : public IEvent
	{
	public:
		CContactEndEvent(CActor* InA, CActor* InB)
			: A(InA)
			, B(InB)
		{
		}
		CContactEndEvent() = delete;
		~CContactEndEvent() = default;

		void Execute() override {}

	public:
		CActor* A = nullptr;
		CActor* B = nullptr;

	private:
		LK_EVENT(CContactEndEvent, ContactEnd);
	};

}
