#include "../litetest/litetest.h"

#include "staticlist.h"

using namespace litetest;
using namespace illumina;

TEST_SUITE(StaticList);

TEST_CASE(StaticListConstructor) {
    StaticList<int, 5> int_list;

    EXPECT(int_list.empty()).to_be(true);
    EXPECT(int_list.full()).to_be(false);
    EXPECT(int_list.size()).to_be(0);
    EXPECT(int_list.capacity()).to_be(5);
}

TEST_CASE(StaticListPushBackAndSize) {
    StaticList<int, 5> int_list;

    int_list.push_back(1);
    int_list.push_back(2);
    int_list.push_back(3);

    EXPECT(int_list.empty()).to_be(false);
    EXPECT(int_list.full()).to_be(false);
    EXPECT(int_list.size()).to_be(3);
    EXPECT(int_list.capacity()).to_be(5);
}

TEST_CASE(StaticListAccessors) {
    StaticList<int, 5> int_list;

    int_list.push_back(1);
    int_list.push_back(2);
    int_list.push_back(3);


    EXPECT(int_list[0]).to_be(1);
    EXPECT(int_list.at(1)).to_be(2);
    EXPECT(int_list[2]).to_be(3);
}

TEST_CASE(StaticListPopBack) {
    StaticList<int, 5> int_list;

    int_list.push_back(1);
    int_list.push_back(2);
    int_list.push_back(3);

    int_list.pop_back();

    EXPECT(int_list.size()).to_be(2);
    EXPECT(int_list[0]).to_be(1);
    EXPECT(int_list[1]).to_be(2);
}

TEST_CASE(StaticListIterators) {
    StaticList<int, 5> int_list;

    int_list.push_back(1);
    int_list.push_back(2);
    int_list.push_back(3);

    int sum = 0;
    for (int& i: int_list) {
        sum += i;
    }

    EXPECT(sum).to_be(6);
}

TEST_CASE(StaticListCopyAndAssignment) {
    StaticList<int, 5> int_list;
    int_list.push_back(1);
    int_list.push_back(2);
    int_list.push_back(3);

    StaticList<int, 5> int_list_copy(int_list);
    EXPECT(int_list_copy.size()).to_be(3);
    EXPECT(int_list_copy[0]).to_be(1);
    EXPECT(int_list_copy[1]).to_be(2);
    EXPECT(int_list_copy[2]).to_be(3);

    StaticList<int, 5> int_list_assigned {};
    int_list_assigned = int_list;
    EXPECT(int_list_assigned.size()).to_be(3);
    EXPECT(int_list_assigned[0]).to_be(1);
    EXPECT(int_list_assigned[1]).to_be(2);
    EXPECT(int_list_assigned[2]).to_be(3);
}