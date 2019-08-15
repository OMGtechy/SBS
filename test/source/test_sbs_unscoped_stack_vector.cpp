#include "test_sbs.hpp"

#include "sbs_unscoped_stack_vector.hpp"

#include <functional>

template <typename T> class USVTest : public ::testing::Test { };

using IntegerTypes = ::testing::Types<uint64_t, int64_t, int16_t, uint16_t>;
TYPED_TEST_CASE(USVTest, IntegerTypes);

using sbs::unscoped_stack_vector;

#ifdef WIN32
#define NEVER_INLINE __declspec(noinline)
#else
#define NEVER_INLINE __attribute__((noinline))
#endif

struct Base
{
    int32_t data;
    Base() = default;
    Base(uint8_t d) : data(d) {}

    // useful for testing emplace-like functions
    Base(uint8_t a, uint8_t b) : data(a + b) { }
};

struct NoCopy final : public Base
{
    using Base::Base;

    // copy not allowed
    NoCopy(const NoCopy&) = delete;
    NoCopy& operator=(const NoCopy&) = delete;

    // move allowed
    NoCopy(NoCopy&&) = default;
    NoCopy& operator=(NoCopy&&) = default;
};

struct NoMove final : public Base
{
    using Base::Base;

    // copy allowed
    NoMove(const NoMove&) = default;
    NoMove& operator=(const NoMove&) = default;

    // move not allowed
    NoMove(NoMove&&) = delete;
    NoMove& operator=(NoMove&&) = delete;
};

TYPED_TEST(USVTest, Capacity1PushBack) {
    // GIVEN: an instance with max_size 1 and no specified size
    unscoped_stack_vector<TypeParam> instance{1};

    // THEN: max_size() returns 1
    //       size() returns 0
    ASSERT_EQ(instance.max_size(), 1U);
    ASSERT_EQ(instance.size(), 0U);

    // WHEN: a value is push_back'd into the instance
    instance.push_back(42);

    // THEN: max_size() returns 1
    //       size() returns 1
    //       instance[0] returns the specified value
    ASSERT_EQ(instance.max_size(), 1U);
    ASSERT_EQ(instance.size(), 1U);
    ASSERT_EQ(instance[0], TypeParam{42});

    // WHEN: pop_back() is called
    instance.pop_back();

    // THEN: max_size() returns 1
    //       size() returns 0
    ASSERT_EQ(instance.max_size(), 1U);
    ASSERT_EQ(instance.size(), 0U);

    // WHEN: another value is push_back'd
    instance.push_back(1);

    // THEN: max_size() returns 1
    //       size() returns 1
    //       instance[0] returns the specified value
    ASSERT_EQ(instance.max_size(), 1U);
    ASSERT_EQ(instance.size(), 1U);
    ASSERT_EQ(instance[0], TypeParam{1});
}

TYPED_TEST(USVTest, Capacity10PushBack) {
    // GIVEN: an instance with max_size 10 and no specified size
    unscoped_stack_vector<TypeParam> instance{10};

    // THEN: max_size() returns 10
    //       size() returns 0
    ASSERT_EQ(instance.max_size(), 10U);
    ASSERT_EQ(instance.size(), 0U);

    // WHEN: 5 values are push_back'd
    instance.push_back(5);
    instance.push_back(4);
    instance.push_back(3);
    instance.push_back(2);
    instance.push_back(1);

    // THEN: max_size() returns 10
    //       size() returns 5
    //       instance[0 -> 4] returns the specified values
    ASSERT_EQ(instance.max_size(), 10U);
    ASSERT_EQ(instance.size(), 5U);
    ASSERT_EQ(instance[0], TypeParam{5});
    ASSERT_EQ(instance[1], TypeParam{4});
    ASSERT_EQ(instance[2], TypeParam{3});
    ASSERT_EQ(instance[3], TypeParam{2});
    ASSERT_EQ(instance[4], TypeParam{1});

    // WHEN: 3 values are pop_back'd
    instance.pop_back();
    instance.pop_back();
    instance.pop_back();

    // THEN: max_size() returns 10
    //       size() returns 2
    //       instance[0 -> 1] returns the specified values
    ASSERT_EQ(instance.max_size(), 10U);
    ASSERT_EQ(instance.size(), 2U);
    ASSERT_EQ(instance[0], TypeParam{5});
    ASSERT_EQ(instance[1], TypeParam{4});
}

NEVER_INLINE
void allocate_one(const size_t size) {
    unscoped_stack_vector<uint64_t> instance(size);
}

TEST(USVTest, StackOverflow) {
    // the memory allocated by this type should be free'd when the stack frame ends,
    // if it doesn't you will hopefully get a stack overflow here,
    // this behaviour was verified on at least one machine
    for(size_t i = 0; i < 10000000; ++i) {
        allocate_one(1000);
    }
}

TEST(USVTest, MoveOnlyType) {
    // GIVEN: an instance for move-only type with max_size 1
    unscoped_stack_vector<NoCopy> instance(1);

    // THEN: max_size() returns 1
    //       size() returns 0
    ASSERT_EQ(instance.max_size(), 1U);
    ASSERT_EQ(instance.size(), 0U);

    // WHEN: a prvalue is push_back'd
    instance.push_back(NoCopy{100});

    // THEN: max_size() returns 1
    //       size() returns 1
    //       instance[0] returns the specified value
    ASSERT_EQ(instance.max_size(), 1U);
    ASSERT_EQ(instance.size(), 1U);
    ASSERT_EQ(instance[0].data, 100);

    // WHEN: pop_back() is called
    instance.pop_back();

    // THEN: max_size() returns 1
    //       size() returns 0
    ASSERT_EQ(instance.max_size(), 1U);
    ASSERT_EQ(instance.size(), 0U);

    // WHEN: an xvalue is push_back'd
    auto lvalue = NoCopy{50};
    instance.push_back(std::move(lvalue));

    // THEN: max_size() returns 1
    //       size() returns 1
    //       instance[0] returns the specified value
    ASSERT_EQ(instance.max_size(), 1U);
    ASSERT_EQ(instance.size(), 1U);
    ASSERT_EQ(instance[0].data, 50);

    // WHEN: pop_back() is called
    instance.pop_back();

    // THEN: max_size() returns 1
    //       size() returns 0
    ASSERT_EQ(instance.max_size(), 1U);
    ASSERT_EQ(instance.size(), 0U);

    // WHEN: emplace_back() is called
    instance.emplace_back(1, 2);

    // THEN: max_size() returns 1
    //       size() returns 1
    //       instance[0] returns the specified value
    ASSERT_EQ(instance.max_size(), 1U);
    ASSERT_EQ(instance.size(), 1U);
    ASSERT_EQ(instance[0].data, 3); // 1 + 2

    // WHEN: pop_back() is called
    instance.pop_back();

    // THEN: max_size() returns 1
    //       size() returns 0
    ASSERT_EQ(instance.max_size(), 1U);
    ASSERT_EQ(instance.size(), 0U);
}

TEST(USVTest, NoMoveType) {
    // GIVEN: an instance for unmoveable types with max_size 1
    unscoped_stack_vector<NoMove> usv(1);

    // THEN: max_size() returns 1
    //       size() returns 0
    ASSERT_EQ(usv.max_size(), 1U);
    ASSERT_EQ(usv.size(), 0U);

    // WHEN: push_back is called
    usv.push_back(NoMove{42});

    // THEN: max_size() returns 1
    //       size() returns 1
    //       usv[0] returns the specified value
    ASSERT_EQ(usv.max_size(), 1U);
    ASSERT_EQ(usv.size(), 1U);
    ASSERT_EQ(usv[0].data, 42);

    // WHEN: pop_back() is called
    usv.pop_back();

    // THEN: max_size() returns 1
    //       size() returns 0
    ASSERT_EQ(usv.max_size(), 1U);
    ASSERT_EQ(usv.size(), 0U);

    // WHEN: an lvalue is push_back'd
    NoMove namedInstance{12};
    usv.push_back(namedInstance);

    // THEN: max_size() returns 1
    //       size() returns 1
    //       usv[0] returns the specified value
    ASSERT_EQ(usv.max_size(), 1U);
    ASSERT_EQ(usv.size(), 1U);
    ASSERT_EQ(usv[0].data, 12);

    // WHEN: pop_back is called
    usv.pop_back();

    // THEN: max_size() returns 1
    //       size() returns 0
    ASSERT_EQ(usv.max_size(), 1U);
    ASSERT_EQ(usv.size(), 0U);

    // WHEN: emplace_back() is called
    usv.emplace_back(1, 1);

    // THEN: max_size() returns 1
    //       size() returns 1
    //       usv[0] returns the expected value
    ASSERT_EQ(usv.max_size(), 1U);
    ASSERT_EQ(usv.size(), 1U);
    ASSERT_EQ(usv[0].data, 2); // ctor sums the params

    // WHEN: pop_back() is called
    usv.pop_back();

    // THEN: max_size() returns 1
    //       size() returns 0
    ASSERT_EQ(usv.max_size(), 1U);
    ASSERT_EQ(usv.size(), 0U);
}

TEST(USVTest, DtorCalled) {
    using func_t = std::function<void()>;
    func_t onDtor = nullptr;

    struct TypeWithDtor final
    {
        const func_t& m_onDtor;

        TypeWithDtor(func_t& toCall) : m_onDtor(toCall) {};
        TypeWithDtor(const TypeWithDtor& other) : m_onDtor(other.m_onDtor) {}
        TypeWithDtor(TypeWithDtor&&) = delete;
        TypeWithDtor& operator=(const TypeWithDtor&) = delete;
        TypeWithDtor& operator=(TypeWithDtor&&) = delete;

        ~TypeWithDtor() { if(m_onDtor) { m_onDtor(); } }
    };

    // GIVEN: an instance with max_size 1
    unscoped_stack_vector<TypeWithDtor> instance(1);

    // WHEN: push_back() is called
    instance.push_back(TypeWithDtor(onDtor));

    // dtor only assigned here so the destruction of intermediates doesn't affect the result
    bool dtorCalled = false;
    onDtor = [&dtorCalled](){ dtorCalled = true; };

    // WHEN: pop_back() is called
    instance.pop_back();

    // THEN: the destructor is called
    ASSERT_TRUE(dtorCalled);
}

TEST(USVTest, InitialSizeRespected) {
    struct TestType final {
        uint32_t data;
        TestType() : data(0xFEEDU) {}
    };

    // GIVEN: an instance with max_size 5 and initial size 3
    unscoped_stack_vector<TestType> instance(5, 3);

    // THEN: max_size() returns 5
    //       size() returns 3
    //       instance[0 -> 2] returns default constructed data
    ASSERT_EQ(instance.max_size(), 5U);
    ASSERT_EQ(instance.size(), 3U);
    ASSERT_EQ(instance[0].data, 0xFEEDU);
    ASSERT_EQ(instance[1].data, 0xFEEDU);
    ASSERT_EQ(instance[2].data, 0xFEEDU);
}

TYPED_TEST(USVTest, ElementAccess) {
    // GIVEN: an instance with max_size 5
    unscoped_stack_vector<char> instance(5);

    // WHEN: 5 elements are push_back'd
    instance.push_back('a');
    instance.push_back('b');
    instance.push_back('c');
    instance.push_back('d');
    instance.push_back('e');

    // THEN: max_size() should return 5
    //       size() should return 5
    ASSERT_EQ(instance.max_size(), 5U);
    ASSERT_EQ(instance.size(), 5U);

    // THEN: instance[0 -> 4] should return the specified values
    ASSERT_EQ(instance[0], 'a');
    ASSERT_EQ(instance[1], 'b');
    ASSERT_EQ(instance[2], 'c');
    ASSERT_EQ(instance[3], 'd');
    ASSERT_EQ(instance[4], 'e');

    // THEN: at(0 -> 4) should return the specified values
    ASSERT_EQ(instance.at(0), 'a');
    ASSERT_EQ(instance.at(1), 'b');
    ASSERT_EQ(instance.at(2), 'c');
    ASSERT_EQ(instance.at(3), 'd');
    ASSERT_EQ(instance.at(4), 'e');

    // THEN: front() should return the first value
    ASSERT_EQ(instance.front(), 'a');

    // THEN: back() should return the last value
    ASSERT_EQ(instance.back(), 'e');

    // THEN: data() should return a pointer to the first element
    ASSERT_EQ(&instance[0], instance.data());
}
