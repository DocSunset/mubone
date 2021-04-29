#ifdef RUNTESTS
#include "catch.hpp"
#include "../Source/list.hpp"
#include <vector>

using namespace mubone;

TEST_CASE("Lists can be instantiated by variadic template")
{
    SECTION("Types are reflected as expected")
    {
        auto l = List<float, int, bool>{};
        static_assert(std::is_same_v<decltype(l.head), float>);
        static_assert(std::is_same_v<decltype(l.tail.head), int>);
        static_assert(std::is_same_v<decltype(l.tail.tail.head), bool>);
        static_assert(std::is_same_v<decltype(l.tail.tail.tail), Nil>);
    }

    SECTION("Values can be initialized with a braced-init list")
    {
        auto l = List<float, float, float>{0.1f, 0.2f, 0.3f};
        REQUIRE(l.head == Approx(0.1));
        REQUIRE(l.tail.head == Approx(0.2));
        REQUIRE(l.tail.tail.head == Approx(0.3));
    }

    SECTION("Types can be inferred from initializer list")
    {
        auto l = List{1, true, 5.0};
    }

}

TEST_CASE("List for each")
{
    auto l = List<int, int, int>{0, 1, 2};
    SECTION("can be used to do something with elements of list")
    {
        std::vector<int> v;
        for_each(l, [&v](const auto& elem){v.push_back(elem);});
        REQUIRE(v.size() == 3);
        REQUIRE(v[0] == 0);
        REQUIRE(v[1] == 1);
        REQUIRE(v[2] == 2);
    }
    SECTION("can be used to mutate elements of list")
    {
        for_each(l, [](auto& elem){elem = elem + 1;});
        REQUIRE(l.head == 1);
        REQUIRE(l.tail.head == 2);
        REQUIRE(l.tail.tail.head == 3);
    }
}

TEST_CASE("List get")
{
    auto l = List<int, bool, float>{10, true, 1.0f};
    REQUIRE(l.head == 10);

    int& got = get<int>(l);
    REQUIRE(got == 10);
    got = 20;
    REQUIRE(l.head == 20);
}

TEST_CASE("List map")
{
    auto l = List{0, 1, 2};
    auto ret = map(l, [](const auto& elem){return elem + 1;});
    static_assert(ret.length == 3);
    REQUIRE(ret.head == 1);
    REQUIRE(ret.tail.head == 2);
    REQUIRE(ret.tail.tail.head == 3);
}

TEST_CASE("List zip")
{
    auto l1 = List<int, int, int>{1, 2, 3};
    auto l2 = List<int, int, int>{4, 5, 6};
    auto ret = zip(l1, l2);
    static_assert(ret.length == 3);
    auto [a, b] = ret.head;
    auto [c, d] = ret.tail.head;
    auto [e, f] = ret.tail.tail.head;
    
    // some calculations which won't compile if there's an issue with zip
    int i = a + b;
    int j = c + d;
    int k = e + f;

    REQUIRE(a == 1);
    REQUIRE(b == 4);
    REQUIRE(c == 2);
    REQUIRE(d == 5);
    REQUIRE(e == 3);
    REQUIRE(f == 6);
}


#endif
