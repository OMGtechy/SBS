#include "benchmark/benchmark.h"

#include "sbs_unscoped_stack_vector.hpp"

BENCHMARK_MAIN();

void BM_create_vector_reserve(benchmark::State& state) {
    const auto task = [arg = static_cast<size_t>(state.range(0))](){
        std::vector<int> instance;
        instance.reserve(arg);
        benchmark::DoNotOptimize(instance);
    };

    for(auto _ : state) {
        task();
    }
}

void BM_create_vector_initial_size(benchmark::State& state) {
    const auto task = [arg = static_cast<size_t>(state.range(0))](){
        std::vector<int> instance(arg);
        benchmark::DoNotOptimize(instance);
    };

    for(auto _ : state) {
        task();
    }
}

void BM_create_usv_reserve(benchmark::State& state) {
    const auto task = [arg = static_cast<size_t>(state.range(0))](){
        sbs::unscoped_stack_vector<int> instance(arg);
        benchmark::DoNotOptimize(instance);
    };

    for(auto _ : state) {
        task();
    }
}

void BM_create_usv_initial_size(benchmark::State& state) {
    const auto task = [arg = static_cast<size_t>(state.range(0))](){
        sbs::unscoped_stack_vector<int> instance{arg, arg};
        benchmark::DoNotOptimize(instance);
    };

    for(auto _ : state) {
        task();
    }
}

template <typename T>
void compute8(T& instance, const    benchmark::State& state) {
    instance.push_back(state.range(0));
    instance.push_back(state.range(1));
    instance.push_back(state.range(2));
    instance.push_back(state.range(3));
    instance.push_back(state.range(4));
    instance.push_back(state.range(5));
    instance.push_back(state.range(6));
    instance.push_back(state.range(7));

    instance[0] = instance[7] % (instance[2] == 0 ? 1 : instance[2]);
    instance[1] = instance[0] + (instance[0] + instance[1]) / 2;
    instance[2] = instance[7] * instance[3];
    instance[3] = instance[6] + instance[5] - instance[4] * instance[3];
    instance[4] = instance[1] & instance[3];
    instance[5] = instance[4] ^ instance[7];
    instance[6] = instance[1] | instance[2];
    instance[7] = ~instance[0];
}

void BM_compute_vector(benchmark::State& state) {
    const auto task = [statePtr = &state](){
        std::vector<int> instance;
        instance.reserve(8);
        compute8(instance, *statePtr);
        benchmark::DoNotOptimize(instance);
    };

    for(auto _ : state) {
        task();
    }
}

void BM_compute_usv(benchmark::State& state) {
    const auto task = [statePtr = &state](){
        sbs::unscoped_stack_vector<int> instance{8};
        compute8(instance, *statePtr);
        benchmark::DoNotOptimize(instance);
    };

    for(auto _ : state) {
        task();
    }
}

const auto mini = 32 + rand() % 32;
const auto maxi = 2048 + rand() % 2048;

BENCHMARK(BM_create_vector_reserve)->Range(1, 1024);
BENCHMARK(BM_create_vector_initial_size)->Range(1, 1024);
BENCHMARK(BM_create_usv_reserve)->Range(1, 1024);
BENCHMARK(BM_create_usv_initial_size)->Range(1, 1024);
BENCHMARK(BM_compute_vector)->Range(mini, maxi);
BENCHMARK(BM_compute_usv)->Range(mini, maxi);
