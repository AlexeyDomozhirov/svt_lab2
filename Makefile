SRC = $(filter-out %_test.c, $(wildcard *.c))
OBJS = $(SRC:.c=.o)
DEPS = $(OBJS:.o=.d)

TEST_SRC = $(wildcard *_test.c)
TEST_OBJS = $(TEST_SRC:.c=.o) 
TEST_EXES = $(TEST_SRC:_test.c=_test)
RUN_TESTS = $(addprefix run_, $(TEST_EXES))
TEST_DEPS = $(TEST_SRC:.c=.d)

.DEFAULT_GOAL := all

%.o: %.c
	clang -MMD -g -c $< -o $@ -lm

%_test: %_test.o $(OBJS)
	clang $^ -o $@ -static -lm

-include $(DEPS) $(TEST_DEPS)

$(RUN_TESTS): run_%: %
	./$<

.PHONY: all run_tests clean

all: $(OBJS) $(TEST_EXES)

run_tests: $(RUN_TESTS)

.PHONY: format_all check_format_all

ALL_SOURCES = $(wildcard *.c *.h)

format_all:
	clang-format -i $(ALL_SOURCES)

check_format_all:
	clang-format --dry-run -Werror $(ALL_SOURCES)

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TEST_EXES) $(DEPS) $(TEST_DEPS)
