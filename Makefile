.PHONY: format fuzz

format:
	find include src tests -iname "*.cpp" -o -iname "*.hpp" | xargs clang-format -i

fuzz:
	CXX=clang++ cmake -Bbuild -S fuzz -DFUZZTEST_FUZZING_MODE=on
	CXX=clang++ cmake --build build
	./build/fuzz -fuzz=fuzz_log_to_string.test_log_to_string
