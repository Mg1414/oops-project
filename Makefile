CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -pedantic -O2
APP := car_rental
SRC := as.cpp

all: $(APP)

$(APP): $(SRC)
	$(CXX) $(CXXFLAGS) $< -o $@

tests/unit_tests: tests/unit_tests.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -DCAR_RENTAL_LIBRARY tests/unit_tests.cpp -o $@

tests/integration_tests: tests/integration_tests.cpp $(SRC)
	$(CXX) $(CXXFLAGS) -DCAR_RENTAL_LIBRARY tests/integration_tests.cpp -o $@

.PHONY: test

test: tests/unit_tests tests/integration_tests
	./tests/unit_tests
	./tests/integration_tests

.PHONY: cppcheck
cppcheck:
	cppcheck --enable=warning,performance,style --std=c++20 --inline-suppr --error-exitcode=1 $(SRC) tests/unit_tests.cpp tests/integration_tests.cpp

.PHONY: clean
clean:
	rm -f $(APP) tests/unit_tests tests/integration_tests
