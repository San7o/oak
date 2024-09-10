format:
	find include src tests -iname "*.cpp" -o -iname "*.hpp" | xargs clang-format -i
