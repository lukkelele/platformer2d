#include <vector>

#include <catch2/catch_test_macros.hpp>

#include "core/object.h"
#include "core/log.h"

namespace Test::Class {

	class CShape : public platformer2d::LObject
	{
	public:
		virtual ~CShape() = default;
		[[nodiscard]] virtual float Area() const { return 0.0f; }

		LK_CLASS();
	};

	class CCircle : public CShape
	{
	public:
		[[nodiscard]] float Area() const override { return 3.14159f * Radius * Radius; }

	private:
		float Radius = 1.0f;

		LK_CLASS();
	};

	class CSquare : public CShape
	{
	public:
		[[nodiscard]] float Area() const override { return Side * Side; }

	private:
		float Side = 2.0f;

		LK_CLASS();
	};

	class CEllipse : public CCircle
	{
		LK_CLASS();
	};

}

using namespace platformer2d;
using namespace platformer2d::Core;
using namespace Test::Class;

template<typename T, typename TBase>
[[nodiscard]] static std::size_t CountExact(const std::vector<std::unique_ptr<TBase>>& Items)
{
	std::size_t Count = 0;
	for (const auto& Item : Items) {
		if (Item->template IsClass<T>()) {
			Count++;
		}
	}
	return Count;
}

TEST_CASE("Names derived at compile time", "[class][name]")
{
	LKLOG_PRINTLN("");
	LK_INFO_TAG("Class", "LObject  -> '{}'", ClassInfoOf<LObject>().Name);
	LK_INFO_TAG("Class", "CShape   -> '{}'", ClassInfoOf<CShape>().Name);
	LK_INFO_TAG("Class", "CCircle  -> '{}'", ClassInfoOf<CCircle>().Name);
	LK_INFO_TAG("Class", "CSquare  -> '{}'", ClassInfoOf<CSquare>().Name);
	LK_INFO_TAG("Class", "CEllipse -> '{}'", ClassInfoOf<CEllipse>().Name);

	REQUIRE(ClassInfoOf<LObject>().Name == "LObject");
	REQUIRE(ClassInfoOf<CShape>().Name == "CShape");
	REQUIRE(ClassInfoOf<CCircle>().Name == "CCircle");
	REQUIRE(ClassInfoOf<CSquare>().Name == "CSquare");
	REQUIRE(ClassInfoOf<CEllipse>().Name == "CEllipse");
}

TEST_CASE("Class descriptor is unique", "[class][identity]")
{
	REQUIRE(&ClassInfoOf<CCircle>() == &ClassInfoOf<CCircle>());
	REQUIRE(&ClassInfoOf<CCircle>() != &ClassInfoOf<CSquare>());
	REQUIRE(&ClassInfoOf<CCircle>() != &ClassInfoOf<CShape>());
	REQUIRE(&ClassInfoOf<CShape>() != &ClassInfoOf<LObject>());
}

TEST_CASE("IsClass matches only the exact dynamic type", "[class][identity]")
{
	const CCircle Circle;
	const CSquare Square;

	REQUIRE(Circle.IsClass<CCircle>());
	REQUIRE_FALSE(Circle.IsClass<CSquare>());
	REQUIRE_FALSE(Circle.IsClass<CShape>());
	REQUIRE_FALSE(Circle.IsClass<LObject>());

	REQUIRE(Square.IsClass<CSquare>());
	REQUIRE_FALSE(Square.IsClass<CCircle>());
}

TEST_CASE("IsClass resolves the dynamic type through base reference", "[class][identity]")
{
	const std::unique_ptr<CShape> Shape = std::make_unique<CCircle>();
	REQUIRE(Shape->IsClass<CCircle>());
	REQUIRE_FALSE(Shape->IsClass<CShape>());
	REQUIRE_FALSE(Shape->IsClass<CSquare>());

	const CShape& Ref = *Shape;
	REQUIRE(Ref.GetClass().Name == "CCircle");
}

TEST_CASE("IsClass is exact", "[class]")
{
	const std::unique_ptr<CShape> Shape = std::make_unique<CEllipse>();
	REQUIRE(Shape->IsClass<CEllipse>());
	REQUIRE_FALSE(Shape->IsClass<CCircle>());
	REQUIRE_FALSE(Shape->IsClass<CShape>());
}

TEST_CASE("Filtering over a container", "[class][filter]")
{
	std::vector<std::unique_ptr<CShape>> Shapes;
	Shapes.push_back(std::make_unique<CCircle>());
	Shapes.push_back(std::make_unique<CCircle>());
	Shapes.push_back(std::make_unique<CSquare>());
	Shapes.push_back(std::make_unique<CEllipse>());

	REQUIRE(CountExact<CCircle>(Shapes) == 2);
	REQUIRE(CountExact<CSquare>(Shapes) == 1);
	REQUIRE(CountExact<CEllipse>(Shapes) == 1);
	REQUIRE(CountExact<CShape>(Shapes) == 0);
}

TEST_CASE("SubclassOf is a compile-time predicate", "[class][subclassof]")
{
	STATIC_REQUIRE(SubclassOf<CCircle, CShape>);
	STATIC_REQUIRE(SubclassOf<CSquare, CShape>);
	STATIC_REQUIRE(SubclassOf<CEllipse, CCircle>);
	STATIC_REQUIRE(SubclassOf<CEllipse, CShape>);
	STATIC_REQUIRE(SubclassOf<CCircle, LObject>);

	STATIC_REQUIRE_FALSE(SubclassOf<CShape, CCircle>);
	STATIC_REQUIRE_FALSE(SubclassOf<CCircle, CSquare>);

	STATIC_REQUIRE(SubclassOf<CCircle, CCircle>);
}

TEST_CASE("A plain LObject reports itself", "[class][lobject]")
{
	const LObject Object;
	REQUIRE(Object.IsClass<LObject>());
	REQUIRE_FALSE(Object.IsClass<CShape>());
	REQUIRE(Object.GetClass().Name == "LObject");
}

