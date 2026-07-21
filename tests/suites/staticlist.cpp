#include <doctest/doctest.h>

#include "staticlist.h"

using namespace illumina;

TEST_SUITE_BEGIN("StaticList");

TEST_CASE("StaticListConstructor") {
    StaticList<int, 5> int_list;

    REQUIRE_EQ(int_list.empty(), true);
    REQUIRE_EQ(int_list.full(), false);
    REQUIRE_EQ(int_list.size(), 0);
    REQUIRE_EQ(int_list.capacity(), 5);
}

TEST_CASE("StaticListPushBackAndSize") {
    StaticList<int, 5> int_list;

    int_list.push_back(1);
    int_list.push_back(2);
    int_list.push_back(3);

    REQUIRE_EQ(int_list.empty(), false);
    REQUIRE_EQ(int_list.full(), false);
    REQUIRE_EQ(int_list.size(), 3);
    REQUIRE_EQ(int_list.capacity(), 5);
}

TEST_CASE("StaticListAccessors") {
    StaticList<int, 5> int_list;

    int_list.push_back(1);
    int_list.push_back(2);
    int_list.push_back(3);


    REQUIRE_EQ(int_list[0], 1);
    REQUIRE_EQ(int_list.at(1), 2);
    REQUIRE_EQ(int_list[2], 3);
}

TEST_CASE("StaticListPopBack") {
    StaticList<int, 5> int_list;

    int_list.push_back(1);
    int_list.push_back(2);
    int_list.push_back(3);

    int_list.pop_back();

    REQUIRE_EQ(int_list.size(), 2);
    REQUIRE_EQ(int_list[0], 1);
    REQUIRE_EQ(int_list[1], 2);
}

TEST_CASE("StaticListIterators") {
    StaticList<int, 5> int_list;

    int_list.push_back(1);
    int_list.push_back(2);
    int_list.push_back(3);

    int sum = 0;
    for (int& i: int_list) {
        sum += i;
    }

    REQUIRE_EQ(sum, 6);
}

TEST_CASE("StaticListCopyAndAssignment") {
    StaticList<int, 5> int_list;
    int_list.push_back(1);
    int_list.push_back(2);
    int_list.push_back(3);

    StaticList<int, 5> int_list_copy(int_list);
    REQUIRE_EQ(int_list_copy.size(), 3);
    REQUIRE_EQ(int_list_copy[0], 1);
    REQUIRE_EQ(int_list_copy[1], 2);
    REQUIRE_EQ(int_list_copy[2], 3);

    StaticList<int, 5> int_list_assigned {};
    int_list_assigned = int_list;
    REQUIRE_EQ(int_list_assigned.size(), 3);
    REQUIRE_EQ(int_list_assigned[0], 1);
    REQUIRE_EQ(int_list_assigned[1], 2);
    REQUIRE_EQ(int_list_assigned[2], 3);
}

TEST_SUITE_END;
